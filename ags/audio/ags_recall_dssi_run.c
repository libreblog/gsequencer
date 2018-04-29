/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2018 Joël Krähemann
 *
 * This file is part of GSequencer.
 *
 * GSequencer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GSequencer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GSequencer.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ags/audio/ags_recall_dssi.h>
#include <ags/audio/ags_recall_dssi_run.h>

#include <ags/libags.h>

#include <ags/plugin/ags_dssi_manager.h>

#include <ags/audio/ags_input.h>
#include <ags/audio/ags_port.h>
#include <ags/audio/ags_note.h>
#include <ags/audio/ags_recall_channel.h>
#include <ags/audio/ags_recall_channel_run.h>
#include <ags/audio/ags_recall_recycling.h>
#include <ags/audio/ags_recall_audio_signal.h>
#include <ags/audio/ags_audio_buffer_util.h>

#include <ags/audio/recall/ags_count_beats_audio_run.h>
#include <ags/audio/recall/ags_route_dssi_audio_run.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <ags/i18n.h>

void ags_recall_dssi_run_class_init(AgsRecallDssiRunClass *recall_dssi_run_class);
void ags_recall_dssi_run_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_recall_dssi_run_plugin_interface_init(AgsPluginInterface *plugin);
void ags_recall_dssi_run_init(AgsRecallDssiRun *recall_dssi_run);
void ags_recall_dssi_run_set_property(GObject *gobject,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *param_spec);
void ags_recall_dssi_run_get_property(GObject *gobject,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *param_spec);
void ags_recall_dssi_run_finalize(GObject *gobject);

void ags_recall_dssi_run_run_init_pre(AgsRecall *recall);
void ags_recall_dssi_run_feed_input_queue(AgsRecall *recall);

void ags_recall_dssi_run_load_ports(AgsRecallDssiRun *recall_dssi_run);

/**
 * SECTION:ags_recall_dssi_run
 * @Short_description: The object interfacing with DSSI
 * @Title: AgsRecallDssiRun
 *
 * #AgsRecallDssiRun provides DSSI support.
 */

enum{
  PROP_0,
  PROP_NOTE,
  PROP_ROUTE_DSSI_AUDIO_RUN,
};

static gpointer ags_recall_dssi_run_parent_class = NULL;
static AgsConnectableInterface* ags_recall_dssi_run_parent_connectable_interface;
static AgsPluginInterface* ags_recall_dssi_run_parent_plugin_interface;

GType
ags_recall_dssi_run_get_type (void)
{
  static GType ags_type_recall_dssi_run = 0;

  if(!ags_type_recall_dssi_run){
    static const GTypeInfo ags_recall_dssi_run_info = {
      sizeof (AgsRecallDssiRunClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_recall_dssi_run_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsRecallDssiRun),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_recall_dssi_run_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_recall_dssi_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_recall_dssi_run_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_recall_dssi_run = g_type_register_static(AGS_TYPE_RECALL_AUDIO_SIGNAL,
						      "AgsRecallDssiRun",
						      &ags_recall_dssi_run_info,
						      0);

    g_type_add_interface_static(ags_type_recall_dssi_run,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_recall_dssi_run,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return(ags_type_recall_dssi_run);
}

void
ags_recall_dssi_run_class_init(AgsRecallDssiRunClass *recall_dssi_run)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;

  GParamSpec *param_spec;

  ags_recall_dssi_run_parent_class = g_type_class_peek_parent(recall_dssi_run);

  /* GObjectClass */
  gobject = (GObjectClass *) recall_dssi_run;

  gobject->set_property = ags_recall_dssi_run_set_property;
  gobject->get_property = ags_recall_dssi_run_get_property;

  gobject->finalize = ags_recall_dssi_run_finalize;

  /* properties */
  /**
   * AgsRecallDssiRun:route-dssi-audio-run:
   * 
   * The route dssi audio run dependency.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("route-dssi-audio-run",
				   i18n_pspec("assigned AgsRouteDssiAudioRun"),
				   i18n_pspec("the AgsRouteDssiAudioRun"),
				   AGS_TYPE_ROUTE_DSSI_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_ROUTE_DSSI_AUDIO_RUN,
				  param_spec);

  /**
   * AgsRecallDssiRun:note:
   *
   * The assigned #AgsNote.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_pointer("note",
				    i18n_pspec("assigned note"),
				    i18n_pspec("The note it is assigned with"),
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_NOTE,
				  param_spec);

  /* AgsRecallClass */
  recall = (AgsRecallClass *) recall_dssi_run;

  recall->run_init_pre = ags_recall_dssi_run_run_init_pre;
  recall->feed_input_queue = ags_recall_dssi_run_feed_input_queue;
}

void
ags_recall_dssi_run_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_recall_dssi_run_parent_connectable_interface = g_type_interface_peek_parent(connectable);
}

void
ags_recall_dssi_run_plugin_interface_init(AgsPluginInterface *plugin)
{
  ags_recall_dssi_run_parent_plugin_interface = g_type_interface_peek_parent(plugin);
}

void
ags_recall_dssi_run_init(AgsRecallDssiRun *recall_dssi_run)
{
  recall_dssi_run->ladspa_handle = NULL;

  recall_dssi_run->audio_channels = 0;

  recall_dssi_run->input = NULL;
  recall_dssi_run->output = NULL;

  recall_dssi_run->port_data = NULL;
  
  recall_dssi_run->delta_time = 0;
  
  recall_dssi_run->event_buffer = (snd_seq_event_t **) malloc(2 * sizeof(snd_seq_event_t *));

  recall_dssi_run->event_buffer[0] = (snd_seq_event_t *) malloc(sizeof(snd_seq_event_t));
  memset(recall_dssi_run->event_buffer[0], 0, sizeof(snd_seq_event_t));
  
  recall_dssi_run->event_buffer[1] = NULL;

  recall_dssi_run->event_count = (unsigned long *) malloc(2 * sizeof(unsigned long));

  recall_dssi_run->event_count[0] = 0;
  recall_dssi_run->event_count[1] = 0;

  recall_dssi_run->route_dssi_audio_run = NULL;
  recall_dssi_run->note = NULL;
}

void
ags_recall_dssi_run_set_property(GObject *gobject,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *param_spec)
{
  AgsRecallDssiRun *recall_dssi_run;

  pthread_mutex_t *recall_mutex;

  recall_dssi_run = AGS_RECALL_DSSI_RUN(gobject);

  /* get recall mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_mutex = AGS_RECALL(gobject)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  switch(prop_id){
  case PROP_ROUTE_DSSI_AUDIO_RUN:
    {
      AgsDelayAudioRun *route_dssi_audio_run;

      route_dssi_audio_run = g_value_get_object(value);

      pthread_mutex_lock(recall_mutex);

      if(route_dssi_audio_run == recall_dssi_run->route_dssi_audio_run){
	pthread_mutex_unlock(recall_mutex);
	
	return;
      }

      if(recall_dssi_run->route_dssi_audio_run != NULL){
	g_object_unref(G_OBJECT(recall_dssi_run->route_dssi_audio_run));
      }

      if(route_dssi_audio_run != NULL){
	g_object_ref(route_dssi_audio_run);
      }

      recall_dssi_run->route_dssi_audio_run = route_dssi_audio_run;

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_NOTE:
    {
      GObject *note;

      note = g_value_get_pointer(value);

      pthread_mutex_lock(recall_mutex);

      if(!AGS_IS_NOTE(note) ||
	 g_list_find(recall_dssi_run->note, note) != NULL){
	pthread_mutex_lock(recall_mutex);
	
	return;
      }

      recall_dssi_run->note = g_list_prepend(recall_dssi_run->note,
					     note);
      g_object_ref(note);

      pthread_mutex_lock(recall_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_recall_dssi_run_get_property(GObject *gobject,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *param_spec)
{
  AgsRecallDssiRun *recall_dssi_run;
  
  pthread_mutex_t *recall_mutex;

  recall_dssi_run = AGS_RECALL_DSSI_RUN(gobject);

  /* get recall mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_mutex = AGS_RECALL(gobject)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  switch(prop_id){
  case PROP_ROUTE_DSSI_AUDIO_RUN:
    {
      pthread_mutex_lock(recall_mutex);

      g_value_set_object(value, recall_dssi_run->route_dssi_audio_run);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_NOTE:
    {
      pthread_mutex_lock(recall_mutex);

      g_value_set_pointer(value,
			  g_list_copy(recall_dssi_run->note));

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_recall_dssi_run_finalize(GObject *gobject)
{
  AgsRecallDssi *recall_dssi;
  AgsRecallDssiRun *recall_dssi_run;
  
  recall_dssi_run = AGS_RECALL_DSSI_RUN(gobject);

  g_free(recall_dssi_run->ladspa_handle);

  g_free(recall_dssi_run->output);
  g_free(recall_dssi_run->input);

  if(recall_dssi_run->port_data != NULL){
    free(recall_dssi_run->port_data);
  }
  
  if(recall_dssi_run->event_buffer != NULL){
    if(recall_dssi_run->event_buffer[0] != NULL){
      free(recall_dssi_run->event_buffer[0]);
    }
    
    free(recall_dssi_run->event_buffer);
  }

  if(recall_dssi_run->event_count != NULL){
    free(recall_dssi_run->event_count);
  }

  if(recall_dssi_run->route_dssi_audio_run != NULL){
    g_object_unref(recall_dssi_run->route_dssi_audio_run);
  }
  
  g_list_free_full(recall_dssi_run->note,
		   g_object_unref);
  
  /* call parent */
  G_OBJECT_CLASS(ags_recall_dssi_run_parent_class)->finalize(gobject);
}

void
ags_recall_dssi_run_run_init_pre(AgsRecall *recall)
{
  AgsRecallDssi *recall_dssi;
  AgsRecallChannelRun *recall_channel_run;
  AgsRecallRecycling *recall_recycling;
  AgsRecallDssiRun *recall_dssi_run;
  AgsAudioSignal *audio_signal;

  LADSPA_Handle *ladspa_handle;

  LADSPA_Data *output, *input;
  LADSPA_Data *port_data;  
  
  guint output_lines, input_lines;
  guint samplerate;
  guint buffer_size;
  guint port_count;
  guint i, i_stop;

  void (*parent_class_run_init_pre)(AgsRecall *recall);

  LADSPA_Handle (*instantiate)(const struct _LADSPA_Descriptor * Descriptor,
                               unsigned long SampleRate);
  void (*activate)(LADSPA_Handle Instance);

  pthread_mutex_t *recall_dssi_mutex;
  
  /* get recall mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());

  parent_class_run_init_pre = AGS_RECALL_CLASS(ags_recall_dssi_run_parent_class)->run_init_pre;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  /* call parent */
  parent_class_run_init_pre(recall);

  recall_dssi_run = AGS_RECALL_DSSI_RUN(recall);

  g_object_get(recall,
	       "parent", &recall_recycling,
	       NULL);

  g_object_get(recall_recycling,
	       "parent", &recall_channel_run,
	       NULL);

  g_object_get(recall_channel_run,
	       "recall-channel", &recall_dssi,
	       NULL);
  
  /* set up buffer */
  g_object_get(recall_dssi_run,
	       "source", &audio_signal,
	       NULL);

  g_object_get(audio_signal,
	       "samplerate", &samplerate,
	       "buffer-size", &buffer_size,
	       NULL);

  /* get recall dssi mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_dssi_mutex = AGS_RECALL(recall_dssi)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  /* get some fields */
  pthread_mutex_lock(recall_dssi_mutex);

  output_lines = recall_dssi->output_lines;
  input_lines = recall_dssi->input_lines;
  
  port_count = recall_dssi->plugin_descriptor->LADSPA_Plugin->PortCount;

  instantiate = recall_dssi->plugin_descriptor->LADSPA_Plugin->instantiate;
  activate = recall_dssi->plugin_descriptor->LADSPA_Plugin->activate;
  
  pthread_mutex_unlock(recall_dssi_mutex);
  
  /* set up buffer */
  output = NULL;
  input = NULL;
  
  if(input_lines > 0){
    input = (LADSPA_Data *) malloc(input_lines *
				   buffer_size *
				   sizeof(LADSPA_Data));
  }

  output = (LADSPA_Data *) malloc(output_lines *
				  buffer_size *
				  sizeof(LADSPA_Data));

  recall_dssi_run->output = output;
  recall_dssi_run->input = input;

  if(input_lines < output_lines){
    i_stop = output_lines;
  }else{
    i_stop = input_lines;
  }

  ladspa_handle = NULL;

  if(i_stop > 0){
    ladspa_handle = (LADSPA_Handle *) malloc(i_stop *
					     sizeof(LADSPA_Handle));
  }
    
  recall_dssi_run->audio_channels = i_stop;
  
  /* instantiate dssi */
  pthread_mutex_lock(recall_dssi_mutex);

  for(i = 0; i < i_stop; i++){
    ladspa_handle[i] = instantiate(recall_dssi->plugin_descriptor->LADSPA_Plugin,
				   (unsigned long) samplerate);

#ifdef AGS_DEBUG
    g_message("instantiated DSSI handle %d %d",
	      recall_dssi->bank,
	      recall_dssi->program);
#endif
  }

  pthread_mutex_unlock(recall_dssi_mutex);

  if(port_count > 0){    
    port_data = (LADSPA_Data *) malloc(port_count * sizeof(LADSPA_Data));
  }

  /*  */  
  recall_dssi_run->ladspa_handle = ladspa_handle;

  recall_dssi_run->port_data = port_data;

  /*  */
  ags_recall_dssi_run_load_ports(recall_dssi_run);
 
  for(i = 0; i < i_stop; i++){
    if(activate != NULL){
      activate(recall_dssi_run->ladspa_handle[i]);
    }
    
#ifdef AGS_DEBUG
    g_message("activated DSSI handle");
#endif
  }
}

void
ags_recall_dssi_run_feed_input_queue(AgsRecall *recall)
{
  AgsRecallDssi *recall_dssi;
  AgsRecallChannelRun *recall_channel_run;
  AgsRecallRecycling *recall_recycling;
  AgsRecallDssiRun *recall_dssi_run;
  AgsAudioSignal *audio_signal;
  AgsPort *current_port;
  AgsRecallID *recall_id;
  AgsRecyclingContext *parent_recycling_context, *recycling_context;
  
  AgsCountBeatsAudioRun *count_beats_audio_run;
  AgsRouteDssiAudioRun *route_dssi_audio_run;
    
  GList *list_start, *list;
  GList *port;
  
  GList *note_start, *note;

  snd_seq_event_t **event_buffer;
  unsigned long *event_count;
    
  gchar *specifier, *current_specifier;
  
  LADSPA_Data port_data;

  guint output_lines, input_lines;
  guint notation_counter;
  guint x0, x1;
  guint port_count;
  
  guint copy_mode_in, copy_mode_out;
  guint buffer_size;
  guint i, i_stop;

  void (*parent_class_feed_input_queue)(AgsRecall *recall);

  void (*select_program)(LADSPA_Handle Instance,
			 unsigned long Bank,
			 unsigned long Program);
  void (*run_synth)(LADSPA_Handle Instance,
		    unsigned long SampleCount,
		    snd_seq_event_t *Events,
		    unsigned long EventCount);
  void (*run)(LADSPA_Handle Instance,
	      unsigned long SampleCount);
  void (*deactivate)(LADSPA_Handle Instance);
  void (*cleanup)(LADSPA_Handle Instance);
  
  pthread_mutex_t *recall_dssi_mutex;
  pthread_mutex_t *port_mutex;

  /* get parent class */
  pthread_mutex_lock(ags_recall_get_class_mutex());

  parent_class_feed_input_queue = AGS_RECALL_CLASS(ags_recall_dssi_run_parent_class)->feed_input_queue;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  /* call parent */
  parent_class_feed_input_queue(recall);

  g_object_get(recall,
	       "recall-id", &recall_id,
	       "source", &audio_signal,
	       NULL);

  g_object_get(recall_id,
	       "recycling-context", &recycling_context,
	       NULL);

  g_object_get(recycling_context,
	       "parent", &parent_recycling_context,
	       NULL);

  g_object_get(audio_signal,
	       "note", &note_start,
	       NULL);

  if(ags_recall_global_get_rt_safe() &&
     parent_recycling_context != NULL &&
     note_start == NULL){
    return;
  }

  g_list_free(note_start);
  
  g_object_get(recall,
	       "parent", &recall_recycling,
	       NULL);

  g_object_get(recall_recycling,
	       "parent", &recall_channel_run,
	       NULL);

  g_object_get(recall_channel_run,
	       "recall-channel", &recall_dssi,
	       NULL);

  recall_dssi_run = AGS_RECALL_DSSI_RUN(recall);

  g_object_get(recall_dssi_run,
	       "route-dssi-audio-run", &route_dssi_audio_run,
	       NULL);
  
  if(route_dssi_audio_run == NULL){
    return;
  }

  /* get recall dssi mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_dssi_mutex = AGS_RECALL(recall_dssi)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  g_object_get(route_dssi_audio_run,
	       "count-beats-audio-run", &count_beats_audio_run,
	       NULL);

  g_object_get(audio_signal,
	       "buffer-size", &buffer_size,
	       NULL);

  /* get some fields */
  pthread_mutex_lock(recall_dssi_mutex);

  output_lines = recall_dssi->output_lines;
  input_lines = recall_dssi->input_lines;

  pthread_mutex_unlock(recall_dssi_mutex);

  if(input_lines < output_lines){
    i_stop = output_lines;
  }else{
    i_stop = input_lines;
  }

  g_object_get(count_beats_audio_run,
	       "notation-counter", &notation_counter,
	       NULL);
  
  g_object_get(recall_dssi_run,
	       "note", &note_start,
	       NULL);
  
  if(ags_recall_global_get_rt_safe()){
    note = note_start;
    
    while(note != NULL){
      g_object_get(note->data,
		   "x0", &x0,
		   "x1", &x1,
		   NULL);
	
      if((x1 <= notation_counter &&
	  !ags_note_test_flags(note->data, AGS_NOTE_FEED)) ||
	 x0 > notation_counter){
	recall_dssi_run->note = g_list_remove(recall_dssi_run->note,
					      note->data);
	g_object_unref(note->data);
      }
    
      note = note->next;
    }
    
    if(note_start == NULL){
      memset(recall_dssi_run->event_buffer[0], 0, sizeof(snd_seq_event_t));
    }

    g_list_free(note_start);
  }else{
    g_object_get(note_start->data,
		 "x0", &x0,
		 "x1", &x1,
		 NULL);
    
    if(audio_signal->stream_current == NULL ||
       (x1 <= notation_counter &&
	!ags_note_test_flags(note_start->data, AGS_NOTE_FEED)) ||
       x0 > notation_counter){
      //    g_message("done");

      pthread_mutex_lock(recall_dssi_mutex);

      deactivate = recall_dssi->plugin_descriptor->LADSPA_Plugin->deactivate;
      cleanup = recall_dssi->plugin_descriptor->LADSPA_Plugin->cleanup;
      
      pthread_mutex_unlock(recall_dssi_mutex);
      
      for(i = 0; i < i_stop; i++){
	/* deactivate */
	//TODO:JK: fix-me
	if(deactivate != NULL){
	  deactivate(recall_dssi_run->ladspa_handle[i]);
	}
      
	cleanup(recall_dssi_run->ladspa_handle[i]);
      }
      
      ags_recall_done(recall);
      g_list_free(note_start);
      
      return;
    }
  }
  
  /* get copy mode and clear buffer */
  copy_mode_in = ags_audio_buffer_util_get_copy_mode(AGS_AUDIO_BUFFER_UTIL_FLOAT,
						     ags_audio_buffer_util_format_from_soundcard(audio_signal->format));

  copy_mode_out = ags_audio_buffer_util_get_copy_mode(ags_audio_buffer_util_format_from_soundcard(audio_signal->format),
						      AGS_AUDIO_BUFFER_UTIL_FLOAT);
  
  if(recall_dssi_run->output != NULL){
    ags_audio_buffer_util_clear_float(recall_dssi_run->output, output_lines,
				      buffer_size);
  }

  if(recall_dssi_run->input != NULL){
    ags_audio_buffer_util_clear_float(recall_dssi_run->input, input_lines,
				      buffer_size);
  }

  /* copy data  */
  if(recall_dssi_run->input != NULL){
    ags_audio_buffer_util_copy_buffer_to_buffer(recall_dssi_run->input, input_lines, 0,
						audio_signal->stream_current->data, 1, 0,
						buffer_size, copy_mode_in);
  }

  /* select program */
  pthread_mutex_lock(recall_dssi_mutex);

  port_count = recall_dssi->plugin_descriptor->LADSPA_Plugin->PortCount;

  select_program = recall_dssi->plugin_descriptor->select_program;
  
  pthread_mutex_unlock(recall_dssi_mutex);

  /* cache port data */
  g_object_get(recall_dssi,
	       "port", &list_start,
	       NULL);
  
  for(i = 0; i < port_count; i++){
    pthread_mutex_lock(recall_dssi_mutex);

    specifier = g_strdup(recall_dssi->plugin_descriptor->LADSPA_Plugin->PortNames[i]);

    pthread_mutex_unlock(recall_dssi_mutex);

    list = list_start;
    
    while(list != NULL){
      gboolean success;
      
      current_port = list->data;

      /* get port mutex */
      pthread_mutex_lock(ags_port_get_class_mutex());

      port_mutex = current_port->obj_mutex;
      
      pthread_mutex_unlock(ags_port_get_class_mutex());

      /* check specifier */
      pthread_mutex_lock(port_mutex);

      current_specifier = g_strdup(current_port->specifier);
      
      pthread_mutex_unlock(port_mutex);
      
      success = (!g_strcmp0(specifier,
			    current_specifier)) ? TRUE: FALSE;
      g_free(current_specifier);
	
      if(success){
	GValue value = {0,};
	  
	g_value_init(&value,
		     G_TYPE_FLOAT);
	ags_port_safe_read(current_port,
			   &value);
	
	recall_dssi_run->port_data[i] = g_value_get_float(&value);
	
	break;
      }

      list = list->next;
    }

    g_free(specifier);
  }
  
  if(select_program != NULL){    
    for(i = 0; i < i_stop; i++){
      select_program(recall_dssi_run->ladspa_handle[i],
		     (unsigned long) recall_dssi->bank,
		     (unsigned long) recall_dssi->program);

      //      g_message("b p %u %u", recall_dssi->bank, recall_dssi->program);
    }
  }

  /* reset port data */    
  for(i = 0; i < port_count; i++){
    pthread_mutex_lock(recall_dssi_mutex);

    specifier = g_strdup(recall_dssi->plugin_descriptor->LADSPA_Plugin->PortNames[i]);

    pthread_mutex_unlock(recall_dssi_mutex);

    list = list_start;
    current_port = NULL;
    
    while(list != NULL){
      gboolean success;
      
      current_port = list->data;

      /* get port mutex */
      pthread_mutex_lock(ags_port_get_class_mutex());

      port_mutex = current_port->obj_mutex;
      
      pthread_mutex_unlock(ags_port_get_class_mutex());

      /* check specifier */
      pthread_mutex_lock(port_mutex);

      current_specifier = g_strdup(current_port->specifier);
      
      pthread_mutex_unlock(port_mutex);
      
      success = (!g_strcmp0(specifier,
			    current_specifier)) ? TRUE: FALSE;
      g_free(current_specifier);

      if(success){
	break;
      }

      list = list->next;
    }

    g_free(specifier);

    if(list != NULL){
      GValue value = {0,};
      
      g_value_init(&value,
		   G_TYPE_FLOAT);
      port_data = recall_dssi_run->port_data[i];

      g_value_set_float(&value,
			port_data);
      ags_port_safe_write(current_port,
			  &value);
    }
  }

  g_list_free(list_start);
  
  /* process data */
  pthread_mutex_lock(recall_dssi_mutex);

  run_synth = recall_dssi->plugin_descriptor->run_synth;
  run = recall_dssi->plugin_descriptor->LADSPA_Plugin->run;
  
  pthread_mutex_unlock(recall_dssi_mutex);

  g_object_get(recall_dssi_run,
	       "note", &note_start,
	       NULL);
  
  note = note_start;

  while(note != NULL){    
    if(run_synth != NULL){
      if(recall_dssi_run->event_buffer != NULL){
	event_buffer = recall_dssi_run->event_buffer;
	event_count = recall_dssi_run->event_count;
      
	while(*event_buffer != NULL){
	  if(event_buffer[0]->type == SND_SEQ_EVENT_NOTEON){
	    run_synth(recall_dssi_run->ladspa_handle[0],
		      (unsigned long) (output_lines * buffer_size),
		      event_buffer[0],
		      event_count[0]);
	  }
	  
	  event_buffer++;
	  event_count++;
	}
      }
    }else if(run != NULL){
      run(recall_dssi_run->ladspa_handle[0],
	  (unsigned long) buffer_size);
    }

    note = note->next;
  }

  g_list_free(note_start);
  
  /* copy data */
  if(recall_dssi_run->output != NULL){
    ags_audio_buffer_util_clear_buffer(audio_signal->stream_current->data, 1,
				       buffer_size, ags_audio_buffer_util_format_from_soundcard(audio_signal->format));
    
    ags_audio_buffer_util_copy_buffer_to_buffer(audio_signal->stream_current->data, 1, 0,
						recall_dssi_run->output, output_lines, 0,
						buffer_size, copy_mode_out);
  }
}

/**
 * ags_recall_dssi_run_load_ports:
 * @recall_dssi_run: the #AgsRecallDssiRun
 *
 * Set up DSSI ports.
 *
 * Since: 2.0.0
 */
void
ags_recall_dssi_run_load_ports(AgsRecallDssiRun *recall_dssi_run)
{
  AgsRecallDssi *recall_dssi;
  AgsRecallChannelRun *recall_channel_run;
  AgsRecallRecycling *recall_recycling;
  AgsPort *current_port;

  AgsDssiPlugin *dssi_plugin;

  GList *list_start, *list;

  gchar *specifier, *current_specifier;
  
  guint output_lines, input_lines;
  guint port_count;
  guint i, j, j_stop;

  DSSI_Descriptor *plugin_descriptor;
  LADSPA_PortDescriptor *port_descriptor;
  LADSPA_PortDescriptor current_port_descriptor;
  
  void (*connect_port)(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * DataLocation);

  pthread_mutex_t *recall_dssi_mutex;
  pthread_mutex_t *port_mutex;

  if(!AGS_IS_RECALL_DSSI_RUN(recall_dssi_run)){
    return;
  }

  g_object_get(recall_dssi_run,
	       "parent", &recall_recycling,
	       NULL);

  g_object_get(recall_recycling,
	       "parent", &recall_channel_run,
	       NULL);

  g_object_get(recall_channel_run,
	       "recall-channel", &recall_dssi,
	       NULL);
  
  /* get recall dssi mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_dssi_mutex = AGS_RECALL(recall_dssi)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  /* get some fields */
  pthread_mutex_lock(recall_dssi_mutex);
  
  output_lines = recall_dssi->output_lines;
  input_lines = recall_dssi->input_lines;

  list_start = g_list_copy(AGS_RECALL(recall_dssi)->port);

  plugin_descriptor = recall_dssi->plugin_descriptor;

  port_count = plugin_descriptor->LADSPA_Plugin->PortCount;
  
  port_descriptor = plugin_descriptor->LADSPA_Plugin->PortDescriptors;

  connect_port = recall_dssi->plugin_descriptor->LADSPA_Plugin->connect_port;
  
  pthread_mutex_unlock(recall_dssi_mutex);

  /* match port */
  if(input_lines < output_lines){
    j_stop = output_lines;
  }else{
    j_stop = input_lines;
  }

  for(i = 0; i < port_count; i++){
    pthread_mutex_lock(recall_dssi_mutex);

    current_port_descriptor = port_descriptor[i];
    
    pthread_mutex_unlock(recall_dssi_mutex);

    if(LADSPA_IS_PORT_CONTROL(current_port_descriptor)){
      if(LADSPA_IS_PORT_INPUT(current_port_descriptor) ||
	 LADSPA_IS_PORT_OUTPUT(current_port_descriptor)){
	LADSPA_Data *port_pointer;
	
	pthread_mutex_lock(recall_dssi_mutex);

	specifier = g_strdup(plugin_descriptor->LADSPA_Plugin->PortNames[i]);

	pthread_mutex_unlock(recall_dssi_mutex);

	list = ags_port_find_specifier(list_start, specifier);
	g_free(specifier);

	if(list != NULL){
	  current_port = list->data;
	  
	  /* get port mutex */
	  pthread_mutex_lock(ags_port_get_class_mutex());

	  port_mutex = current_port->obj_mutex;
      
	  pthread_mutex_unlock(ags_port_get_class_mutex());

	  /* get port pointer */
	  pthread_mutex_lock(port_mutex);
	    
	  port_pointer = (LADSPA_Data *) &(current_port->port_value.ags_port_ladspa);

	  pthread_mutex_unlock(port_mutex);

	  for(j = 0; j < j_stop; j++){
#ifdef AGS_DEBUG
	    g_message("connecting port[%d]: %d/%d - %f", j, i, port_count, current->port_value.ags_port_ladspa);
#endif	  
	    connect_port(recall_dssi_run->ladspa_handle[j],
			 (unsigned long) i,
			 port_pointer);
	  }
	}
      }
    }
  }

  g_list_free(list_start);
  
  /* connect audio port */
  for(j = 0; j < input_lines; j++){
    connect_port(recall_dssi_run->ladspa_handle[j],
		 (unsigned long) (recall_dssi->input_port[j]),
		 &(recall_dssi_run->input[j]));
  }
  
  for(j = 0; j < recall_dssi->output_lines; j++){
    connect_port(recall_dssi_run->ladspa_handle[j],
		 (unsigned long) (recall_dssi->output_port[j]),
		 &(recall_dssi_run->output[j]));
  }
}

/**
 * ags_recall_dssi_run_new:
 * @source: the #AgsAudioSignal as source
 *
 * Creates a new instance of #AgsRecallDssiRun
 *
 * Returns: the new #AgsRecallDssiRun
 *
 * Since: 2.0.0
 */
AgsRecallDssiRun*
ags_recall_dssi_run_new(AgsAudioSignal *source)
{
  AgsRecallDssiRun *recall_dssi_run;

  recall_dssi_run = (AgsRecallDssiRun *) g_object_new(AGS_TYPE_RECALL_DSSI_RUN,
						      "source", source,
						      NULL);

  return(recall_dssi_run);
}
