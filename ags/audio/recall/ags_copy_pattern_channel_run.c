/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2015 Joël Krähemann
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

#include <ags/audio/recall/ags_copy_pattern_channel_run.h>

#include <ags/libags.h>

#include <ags/audio/ags_audio.h>
#include <ags/audio/ags_recycling.h>
#include <ags/audio/ags_audio_signal.h>
#include <ags/audio/ags_preset.h>
#include <ags/audio/ags_pattern.h>
#include <ags/audio/ags_recall_id.h>
#include <ags/audio/ags_recall_container.h>

#include <ags/audio/recall/ags_copy_pattern_audio.h>
#include <ags/audio/recall/ags_copy_pattern_audio_run.h>
#include <ags/audio/recall/ags_copy_pattern_channel.h>
#include <ags/audio/recall/ags_delay_audio_run.h>

#include <stdlib.h>

void ags_copy_pattern_channel_run_class_init(AgsCopyPatternChannelRunClass *copy_pattern_channel_run);
void ags_copy_pattern_channel_run_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_copy_pattern_channel_run_dynamic_connectable_interface_init(AgsDynamicConnectableInterface *dynamic_connectable);
void ags_copy_pattern_channel_run_plugin_interface_init(AgsPluginInterface *plugin);
void ags_copy_pattern_channel_run_init(AgsCopyPatternChannelRun *copy_pattern_channel_run);
void ags_copy_pattern_channel_run_connect(AgsConnectable *connectable);
void ags_copy_pattern_channel_run_disconnect(AgsConnectable *connectable);
void ags_copy_pattern_channel_run_connect_dynamic(AgsDynamicConnectable *dynamic_connectable);
void ags_copy_pattern_channel_run_disconnect_dynamic(AgsDynamicConnectable *dynamic_connectable);
void ags_copy_pattern_channel_run_finalize(GObject *gobject);

void ags_copy_pattern_channel_run_resolve_dependencies(AgsRecall *recall);
void ags_copy_pattern_channel_run_run_init_pre(AgsRecall *recall);
void ags_copy_pattern_channel_run_done(AgsRecall *recall);
void ags_copy_pattern_channel_run_cancel(AgsRecall *recall);
void ags_copy_pattern_channel_run_remove(AgsRecall *recall);
AgsRecall* ags_copy_pattern_channel_run_duplicate(AgsRecall *recall,
						  AgsRecallID *recall_id,
						  guint *n_params, GParameter *parameter);

void ags_copy_pattern_channel_run_sequencer_alloc_callback(AgsDelayAudioRun *delay_audio_run,
							   guint run_order,
							   gdouble delay, guint attack,
							   AgsCopyPatternChannelRun *copy_pattern_channel_run);

/**
 * SECTION:ags_copy_pattern_channel_run
 * @short_description: copys pattern
 * @title: AgsCopyPatternChannelRun
 * @section_id:
 * @include: ags/audio/recall/ags_copy_pattern_channel_run.h
 *
 * The #AgsCopyPatternChannelRun class copys pattern.
 */

static gpointer ags_copy_pattern_channel_run_parent_class = NULL;
static AgsConnectableInterface* ags_copy_pattern_channel_run_parent_connectable_interface;
static AgsDynamicConnectableInterface *ags_copy_pattern_channel_run_parent_dynamic_connectable_interface;
static AgsPluginInterface *ags_copy_pattern_channel_run_parent_plugin_interface;

GType
ags_copy_pattern_channel_run_get_type()
{
  static GType ags_type_copy_pattern_channel_run = 0;

  if(!ags_type_copy_pattern_channel_run){
    static const GTypeInfo ags_copy_pattern_channel_run_info = {
      sizeof (AgsCopyPatternChannelRunClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_copy_pattern_channel_run_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsCopyPatternChannelRun),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_copy_pattern_channel_run_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_copy_pattern_channel_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_dynamic_connectable_interface_info = {
      (GInterfaceInitFunc) ags_copy_pattern_channel_run_dynamic_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_copy_pattern_channel_run_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };    

    ags_type_copy_pattern_channel_run = g_type_register_static(AGS_TYPE_RECALL_CHANNEL_RUN,
							       "AgsCopyPatternChannelRun",
							       &ags_copy_pattern_channel_run_info,
							       0);
    
    g_type_add_interface_static(ags_type_copy_pattern_channel_run,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
    
    g_type_add_interface_static(ags_type_copy_pattern_channel_run,
				AGS_TYPE_DYNAMIC_CONNECTABLE,
				&ags_dynamic_connectable_interface_info);

    g_type_add_interface_static(ags_type_copy_pattern_channel_run,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return(ags_type_copy_pattern_channel_run);
}

void
ags_copy_pattern_channel_run_class_init(AgsCopyPatternChannelRunClass *copy_pattern_channel_run)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  AgsRecallChannelRunClass *recall_channel_run;
  GParamSpec *param_spec;

  ags_copy_pattern_channel_run_parent_class = g_type_class_peek_parent(copy_pattern_channel_run);

  /* GObjectClass */
  gobject = (GObjectClass *) copy_pattern_channel_run;

  gobject->finalize = ags_copy_pattern_channel_run_finalize;

  /* AgsRecallClass */
  recall = (AgsRecallClass *) copy_pattern_channel_run;

  recall->resolve_dependencies = ags_copy_pattern_channel_run_resolve_dependencies;
  recall->run_init_pre = ags_copy_pattern_channel_run_run_init_pre;
  recall->done = ags_copy_pattern_channel_run_done;
  recall->cancel = ags_copy_pattern_channel_run_cancel;
  recall->remove = ags_copy_pattern_channel_run_remove;
  recall->duplicate = ags_copy_pattern_channel_run_duplicate;
}

void
ags_copy_pattern_channel_run_connectable_interface_init(AgsConnectableInterface *connectable)
{
  AgsConnectableInterface *ags_copy_pattern_channel_run_connectable_parent_interface;

  ags_copy_pattern_channel_run_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_copy_pattern_channel_run_connect;
  connectable->disconnect = ags_copy_pattern_channel_run_disconnect;
}

void
ags_copy_pattern_channel_run_dynamic_connectable_interface_init(AgsDynamicConnectableInterface *dynamic_connectable)
{
  ags_copy_pattern_channel_run_parent_dynamic_connectable_interface = g_type_interface_peek_parent(dynamic_connectable);

  dynamic_connectable->connect_dynamic = ags_copy_pattern_channel_run_connect_dynamic;
  dynamic_connectable->disconnect_dynamic = ags_copy_pattern_channel_run_disconnect_dynamic;
}

void
ags_copy_pattern_channel_run_plugin_interface_init(AgsPluginInterface *plugin)
{
  ags_copy_pattern_channel_run_parent_plugin_interface = g_type_interface_peek_parent(plugin);
}

void
ags_copy_pattern_channel_run_init(AgsCopyPatternChannelRun *copy_pattern_channel_run)
{
  AGS_RECALL(copy_pattern_channel_run)->name = "ags-copy-pattern";
  AGS_RECALL(copy_pattern_channel_run)->version = AGS_RECALL_DEFAULT_VERSION;
  AGS_RECALL(copy_pattern_channel_run)->build_id = AGS_RECALL_DEFAULT_BUILD_ID;
  AGS_RECALL(copy_pattern_channel_run)->xml_type = "ags-copy-pattern-channel-run";
  AGS_RECALL(copy_pattern_channel_run)->port = NULL;

  AGS_RECALL(copy_pattern_channel_run)->child_type = G_TYPE_NONE;

  copy_pattern_channel_run->note = NULL;
}

void
ags_copy_pattern_channel_run_connect(AgsConnectable *connectable)
{
  if((AGS_RECALL_CONNECTED & (AGS_RECALL(connectable)->flags)) != 0){
    return;
  }

  ags_copy_pattern_channel_run_parent_connectable_interface->connect(connectable);

  /* empty */
}

void
ags_copy_pattern_channel_run_disconnect(AgsConnectable *connectable)
{
  if((AGS_RECALL_CONNECTED & (AGS_RECALL(connectable)->flags)) == 0){
    return;
  }

  ags_copy_pattern_channel_run_parent_connectable_interface->disconnect(connectable);

  /* empty */
}

void
ags_copy_pattern_channel_run_connect_dynamic(AgsDynamicConnectable *dynamic_connectable)
{
  AgsCopyPatternAudioRun *copy_pattern_audio_run;
  AgsCopyPatternChannelRun *copy_pattern_channel_run;
  AgsDelayAudioRun *delay_audio_run;

  if((AGS_RECALL_DYNAMIC_CONNECTED & (AGS_RECALL(dynamic_connectable)->flags)) != 0){
    return;
  }

  /* call parent */
  ags_copy_pattern_channel_run_parent_dynamic_connectable_interface->connect_dynamic(dynamic_connectable);

  /* AgsCopyPatternChannelRun */
  copy_pattern_channel_run = AGS_COPY_PATTERN_CHANNEL_RUN(dynamic_connectable);

  /* get AgsCopyPatternAudioRun */
  copy_pattern_audio_run = AGS_COPY_PATTERN_AUDIO_RUN(AGS_RECALL_CHANNEL_RUN(copy_pattern_channel_run)->recall_audio_run);

  /* connect sequencer_alloc in AgsDelayAudioRun */
  delay_audio_run = copy_pattern_audio_run->delay_audio_run;

  copy_pattern_channel_run->sequencer_alloc_handler =
    g_signal_connect(G_OBJECT(delay_audio_run), "sequencer-alloc-input",
		     G_CALLBACK(ags_copy_pattern_channel_run_sequencer_alloc_callback), copy_pattern_channel_run);
}

void
ags_copy_pattern_channel_run_disconnect_dynamic(AgsDynamicConnectable *dynamic_connectable)
{
  AgsCopyPatternAudioRun *copy_pattern_audio_run;
  AgsCopyPatternChannelRun *copy_pattern_channel_run;
  AgsDelayAudioRun *delay_audio_run;

  if((AGS_RECALL_DYNAMIC_CONNECTED & (AGS_RECALL(dynamic_connectable)->flags)) == 0){
    return;
  }

  /* AgsCopyPatternChannelRun */
  copy_pattern_channel_run = AGS_COPY_PATTERN_CHANNEL_RUN(dynamic_connectable);

  /* get AgsCopyPatternAudioRun */
  copy_pattern_audio_run = AGS_COPY_PATTERN_AUDIO_RUN(AGS_RECALL_CHANNEL_RUN(copy_pattern_channel_run)->recall_audio_run);

  /* disconnect sequencer_alloc in AgsDelayAudioRun */
  delay_audio_run = copy_pattern_audio_run->delay_audio_run;

  g_signal_handler_disconnect(G_OBJECT(delay_audio_run),
			      copy_pattern_channel_run->sequencer_alloc_handler);

  /* call parent */
  ags_copy_pattern_channel_run_parent_dynamic_connectable_interface->disconnect_dynamic(dynamic_connectable);
}

void
ags_copy_pattern_channel_run_finalize(GObject *gobject)
{
  G_OBJECT_CLASS(ags_copy_pattern_channel_run_parent_class)->finalize(gobject);
}

void
ags_copy_pattern_channel_run_resolve_dependencies(AgsRecall *recall)
{
  //TODO:JK: implement me
}

void
ags_copy_pattern_channel_run_run_init_pre(AgsRecall *recall)
{
  AgsChannel *source;
  AgsPattern *pattern;
  AgsNote *note;
  
  AgsCopyPatternChannel *copy_pattern_channel;
  AgsCopyPatternChannelRun *copy_pattern_channel_run;

  AgsMutexManager *mutex_manager;

  guint pad;
  guint i, i_stop;
  
  GValue pattern_value = { 0, };  

  pthread_mutex_t *application_mutex;
  pthread_mutex_t *source_mutex;  
  pthread_mutex_t *pattern_mutex;
  
  mutex_manager = ags_mutex_manager_get_instance();
  application_mutex = ags_mutex_manager_get_application_mutex(mutex_manager);

  AGS_RECALL_CLASS(ags_copy_pattern_channel_run_parent_class)->run_init_pre(recall);

  /*  */
  copy_pattern_channel_run = AGS_COPY_PATTERN_CHANNEL_RUN(recall);

  /* get AgsCopyPatternChannel */
  copy_pattern_channel = AGS_COPY_PATTERN_CHANNEL(copy_pattern_channel_run->recall_channel_run.recall_channel);

  /* get source */
  source = AGS_RECALL_CHANNEL(copy_pattern_channel)->source;

  pthread_mutex_lock(application_mutex);
  
  source_mutex = ags_mutex_manager_lookup(mutex_manager,
					  (GObject *) source);
    
  pthread_mutex_unlock(application_mutex);

  /* get AgsPattern */
  g_value_init(&pattern_value, G_TYPE_POINTER);
  ags_port_safe_read(copy_pattern_channel->pattern,
		     &pattern_value);

  pattern = g_value_get_pointer(&pattern_value);

  pthread_mutex_lock(application_mutex);
  
  pattern_mutex = ags_mutex_manager_lookup(mutex_manager,
					   (GObject *) pattern);
  
  pthread_mutex_unlock(application_mutex);

  /* add note */
  pthread_mutex_lock(source_mutex);
  
  pad = source->pad;

  pthread_mutex_unlock(source_mutex);

  /* i stop */
  pthread_mutex_lock(pattern_mutex);

  i_stop = pattern->dim[2];

  pthread_mutex_unlock(pattern_mutex);
  
  for(i = 0; i < i_stop; i++){
    note = ags_note_new();

    note->x[0] = i;
    note->x[1] = i + 1;

    note->y = pad;

    copy_pattern_channel_run->note = g_list_prepend(copy_pattern_channel_run->note,
						    note);
    g_object_ref(note);
  }

  copy_pattern_channel_run->note = g_list_reverse(copy_pattern_channel_run->note);
}

void
ags_copy_pattern_channel_run_done(AgsRecall *recall)
{
  AgsCopyPatternChannelRun *copy_pattern_channel_run;
  AgsCopyPatternAudioRun *copy_pattern_audio_run;

  copy_pattern_channel_run = AGS_COPY_PATTERN_CHANNEL_RUN(recall);

  /* get AgsCopyPatternAudioRun */
  copy_pattern_audio_run = AGS_COPY_PATTERN_AUDIO_RUN(AGS_RECALL_CHANNEL_RUN(copy_pattern_channel_run)->recall_audio_run);

  /* denotify dependency */
  ags_recall_notify_dependency(AGS_RECALL(copy_pattern_audio_run->count_beats_audio_run),
 			       AGS_RECALL_NOTIFY_CHANNEL_RUN, -1);

  g_list_free_full(copy_pattern_channel_run->note,
		   g_object_unref);

  copy_pattern_channel_run->note = NULL;
  
  /* call parent */
  AGS_RECALL_CLASS(ags_copy_pattern_channel_run_parent_class)->done(recall);
}

void
ags_copy_pattern_channel_run_cancel(AgsRecall *recall)
{
  AGS_RECALL_CLASS(ags_copy_pattern_channel_run_parent_class)->cancel(recall);
}

void
ags_copy_pattern_channel_run_remove(AgsRecall *recall)
{

  AGS_RECALL_CLASS(ags_copy_pattern_channel_run_parent_class)->remove(recall);

  /* empty */
}

AgsRecall*
ags_copy_pattern_channel_run_duplicate(AgsRecall *recall,
				       AgsRecallID *recall_id,
				       guint *n_params, GParameter *parameter)
{
  AgsCopyPatternChannelRun *copy;

  copy = AGS_COPY_PATTERN_CHANNEL_RUN(AGS_RECALL_CLASS(ags_copy_pattern_channel_run_parent_class)->duplicate(recall,
													     recall_id,
													     n_params, parameter));

  /* empty */

  return((AgsRecall *) copy);
}

void
ags_copy_pattern_channel_run_sequencer_alloc_callback(AgsDelayAudioRun *delay_audio_run,
						      guint run_order,
						      gdouble delay, guint attack,
						      AgsCopyPatternChannelRun *copy_pattern_channel_run)
{
  AgsAudio *audio;
  AgsChannel *source;
  AgsPattern *pattern;
  AgsCopyPatternAudio *copy_pattern_audio;
  AgsCopyPatternAudioRun *copy_pattern_audio_run;
  AgsCopyPatternChannel *copy_pattern_channel;

  AgsMutexManager *mutex_manager;

  guint sequencer_counter;
  gboolean current_bit;

  GValue pattern_value = { 0, };  
  GValue i_value = { 0, };
  GValue j_value = { 0, };

  pthread_mutex_t *application_mutex;
  pthread_mutex_t *audio_mutex;
  pthread_mutex_t *source_mutex;
  pthread_mutex_t *pattern_mutex;
  
  if(delay != 0.0){
    return;
  }

  mutex_manager = ags_mutex_manager_get_instance();
  application_mutex = ags_mutex_manager_get_application_mutex(mutex_manager);

  /* get AgsCopyPatternAudio */
  copy_pattern_audio = AGS_COPY_PATTERN_AUDIO(AGS_RECALL_CHANNEL_RUN(copy_pattern_channel_run)->recall_audio_run->recall_audio);

  /* get AgsCopyPatternAudioRun */
  copy_pattern_audio_run = AGS_COPY_PATTERN_AUDIO_RUN(AGS_RECALL_CHANNEL_RUN(copy_pattern_channel_run)->recall_audio_run);

  /* get AgsCopyPatternChannel */
  copy_pattern_channel = AGS_COPY_PATTERN_CHANNEL(copy_pattern_channel_run->recall_channel_run.recall_channel);

  g_value_init(&i_value, G_TYPE_UINT64);
  ags_port_safe_read(copy_pattern_audio->bank_index_0, &i_value);

  g_value_init(&j_value, G_TYPE_UINT64);
  ags_port_safe_read(copy_pattern_audio->bank_index_1, &j_value);

  /* get AgsPattern */
  g_value_init(&pattern_value, G_TYPE_POINTER);
  ags_port_safe_read(copy_pattern_channel->pattern,
		     &pattern_value);

  pattern = g_value_get_pointer(&pattern_value);

  pthread_mutex_lock(application_mutex);
  
  pattern_mutex = ags_mutex_manager_lookup(mutex_manager,
					   (GObject *) pattern);
  
  pthread_mutex_unlock(application_mutex);

  /* write pattern port - current offset */
  ags_port_safe_set_property(copy_pattern_channel->pattern,
			     "first-index", &i_value);
  g_value_unset(&i_value);
  
  ags_port_safe_set_property(copy_pattern_channel->pattern,
			     "second-index", &j_value);
  g_value_unset(&j_value);

  /* get sequencer counter */
  //FIXME:JK: check thread safety
  sequencer_counter = copy_pattern_audio_run->count_beats_audio_run->sequencer_counter;
  
  /* read pattern port - current bit */
  pthread_mutex_lock(pattern_mutex);
  
  current_bit = ags_pattern_get_bit(pattern,
				    pattern->i,
				    pattern->j,
				    sequencer_counter);
  
  pthread_mutex_unlock(pattern_mutex);

  g_value_unset(&pattern_value);
  
  /*  */
  if(current_bit){
    AgsChannel *link;
    AgsRecycling *recycling;
    AgsRecycling *end_recycling;
    AgsAudioSignal *audio_signal;
    AgsNote *note;

    GList *preset;

    guint pad;
    guint audio_channel;
  
    pthread_mutex_t *link_mutex;
    
    //    g_message("ags_copy_pattern_channel_run_sequencer_alloc_callback - playing channel: %u; playing pattern: %u",
    //	      AGS_RECALL_CHANNEL(copy_pattern_channel)->source->line,
    //	      copy_pattern_audio_run->count_beats_audio_run->sequencer_counter);

    /* get audio */
    audio = AGS_RECALL_AUDIO(copy_pattern_audio)->audio;

    pthread_mutex_lock(application_mutex);
  
    audio_mutex = ags_mutex_manager_lookup(mutex_manager,
					    (GObject *) audio);
    
    pthread_mutex_unlock(application_mutex);
    
    /* get source */
    source = AGS_RECALL_CHANNEL(copy_pattern_channel)->source;

    pthread_mutex_lock(application_mutex);
  
    source_mutex = ags_mutex_manager_lookup(mutex_manager,
					    (GObject *) source);
    
    pthread_mutex_unlock(application_mutex);
    
    /* source fields */
    pthread_mutex_lock(source_mutex);

    link = source->link;
    
    recycling = source->first_recycling;

    if(recycling != NULL){
      end_recycling = source->last_recycling->next;
    }

    pad = source->pad;
    audio_channel = source->audio_channel;
    
    pthread_mutex_unlock(source_mutex);

    /* find preset scope envelope */
    pthread_mutex_lock(audio_mutex);

    preset = audio->preset;

    while((preset = ags_preset_find_scope(preset,
					  "ags-envelope")) != NULL){
      if(audio_channel >= AGS_PRESET(preset->data)->audio_channel_start &&
	 audio_channel < AGS_PRESET(preset->data)->audio_channel_end &&
	 pad >= AGS_PRESET(preset->data)->pad_start &&
	 pad < AGS_PRESET(preset->data)->pad_end &&
	 sequencer_counter >= AGS_PRESET(preset->data)->x_start &&
	 sequencer_counter < AGS_PRESET(preset->data)->x_end){
	break;
      }

      preset = preset->next;
    }
    
    pthread_mutex_unlock(audio_mutex);

    /* link */
    if(link != NULL){
      pthread_mutex_lock(application_mutex);
      
      link_mutex = ags_mutex_manager_lookup(mutex_manager,
					    (GObject *) link);
      
      pthread_mutex_unlock(application_mutex);
    }

    /* create audio signals */
    if(recycling != NULL){
      AgsRecallID *child_recall_id;
      
      while(recycling != end_recycling){
	child_recall_id = AGS_RECALL(copy_pattern_channel_run)->recall_id;

	/* apply preset */
	note = g_list_nth(copy_pattern_channel_run->note,
			  sequencer_counter)->data;
	
	if(preset != NULL){
	  AgsComplex *val;
	  
	  GValue value = {0,};

	  GError *error;
	  
	  note->flags |= AGS_NOTE_ENVELOPE;
	  
	  /* get attack */
	  g_value_init(&value,
		       AGS_TYPE_COMPLEX);

	  error = NULL;
	  ags_preset_get_parameter((AgsPreset *) preset->data,
				   "attack", &value,
				   &error);

	  if(error == NULL){
	    val = (AgsComplex *) g_value_get_boxed(&value);

	    note->attack[0] = val[0][0];
	    note->attack[1] = val[0][1];
	  }

	  /* get decay */
	  g_value_reset(&value);

	  error = NULL;
	  ags_preset_get_parameter((AgsPreset *) preset->data,
				   "decay", &value,
				   &error);

	  if(error == NULL){
	    val = (AgsComplex *) g_value_get_boxed(&value);

	    note->decay[0] = val[0][0];
	    note->decay[1] = val[0][1];
	  }

	  /* get sustain */
	  g_value_reset(&value);

	  error = NULL;
	  ags_preset_get_parameter((AgsPreset *) preset->data,
				   "sustain", &value,
				   &error);

	  if(error == NULL){
	    val = (AgsComplex *) g_value_get_boxed(&value);

	    note->sustain[0] = val[0][0];
	    note->sustain[1] = val[0][1];
	  }

	  /* get release */
	  g_value_reset(&value);

	  error = NULL;
	  ags_preset_get_parameter((AgsPreset *) preset->data,
				   "release", &value,
				   &error);

	  if(error == NULL){
	    val = (AgsComplex *) g_value_get_boxed(&value);

	    note->release[0] = val[0][0];
	    note->release[1] = val[0][1];
	  }

	  /* get ratio */
	  g_value_reset(&value);

	  error = NULL;
	  ags_preset_get_parameter((AgsPreset *) preset->data,
				   "ratio", &value,
				   &error);

	  if(error == NULL){
	    val = (AgsComplex *) g_value_get_boxed(&value);

	    note->ratio[0] = val[0][0];
	    note->ratio[1] = val[0][1];
	  }
	}
	
	if(!AGS_RECALL(copy_pattern_audio)->rt_safe){
	  /* create audio signal */
	  audio_signal = ags_audio_signal_new(AGS_RECALL(copy_pattern_audio)->soundcard,
					      (GObject *) recycling,
					      (GObject *) child_recall_id);
	  ags_recycling_create_audio_signal_with_defaults(recycling,
							  audio_signal,
							  0.0, attack);
	  audio_signal->flags &= (~AGS_AUDIO_SIGNAL_TEMPLATE);
	  audio_signal->stream_current = audio_signal->stream_beginning;

	  ags_connectable_connect(AGS_CONNECTABLE(audio_signal));

	  audio_signal->recall_id = (GObject *) child_recall_id;
	  ags_recycling_add_audio_signal(recycling,
					 audio_signal);

	  g_object_set(audio_signal,
		       "note", note,
		       NULL);
	}else{
	  GList *list;

	  audio_signal = NULL;
	  list = ags_audio_signal_get_by_recall_id(recycling->audio_signal,
						   child_recall_id);
	    
	  if(list != NULL){
	    audio_signal = list->data;

	    g_object_set(audio_signal,
			 "note", note,
			 NULL);
	  }
	    
	  note->rt_offset = 0;
	}
		
	/*
	 * emit add_audio_signal on AgsRecycling
	 */
#ifdef AGS_DEBUG
	g_message("play %x", AGS_RECALL(copy_pattern_channel_run)->recall_id);
#endif

	/*
	 * unref AgsAudioSignal because AgsCopyPatternChannelRun has no need for it
	 * if you need a valid reference to audio_signal you have to g_object_ref(audio_signal)
	 */
	//	g_object_unref(audio_signal);
		
	recycling = recycling->next;
      }
    }
  }
  
      //      g_message("%u\n", copy_pattern->shared_audio_run->bit);
      //      copy_pattern->shared_audio_run->bit++;
  //  }
}

/**
 * ags_copy_pattern_channel_run_new:
 *
 * Creates an #AgsCopyPatternChannelRun
 *
 * Returns: a new #AgsCopyPatternChannelRun
 *
 * Since: 1.0.0
 */
AgsCopyPatternChannelRun*
ags_copy_pattern_channel_run_new()
{
  AgsCopyPatternChannelRun *copy_pattern_channel_run;

  copy_pattern_channel_run = (AgsCopyPatternChannelRun *) g_object_new(AGS_TYPE_COPY_PATTERN_CHANNEL_RUN,
								       NULL);

  return(copy_pattern_channel_run);
}
