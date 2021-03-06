/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2019 Joël Krähemann
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

#include <ags/audio/recall/ags_route_dssi_audio_run.h>

#include <ags/audio/ags_recall_id.h>
#include <ags/audio/ags_recall_container.h>
#include <ags/audio/ags_recall_dssi.h>
#include <ags/audio/ags_recall_dssi_run.h>

#include <ags/audio/recall/ags_route_dssi_audio.h>
#include <ags/audio/recall/ags_delay_audio.h>
#include <ags/audio/recall/ags_delay_audio_run.h>

#include <alsa/seq_midi_event.h>

#include <ags/i18n.h>

void ags_route_dssi_audio_run_class_init(AgsRouteDssiAudioRunClass *route_dssi_audio_run);
void ags_route_dssi_audio_run_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_route_dssi_audio_run_init(AgsRouteDssiAudioRun *route_dssi_audio_run);
void ags_route_dssi_audio_run_set_property(GObject *gobject,
					   guint prop_id,
					   const GValue *value,
					   GParamSpec *param_spec);
void ags_route_dssi_audio_run_get_property(GObject *gobject,
					   guint prop_id,
					   GValue *value,
					   GParamSpec *param_spec);
void ags_route_dssi_audio_run_dispose(GObject *gobject);
void ags_route_dssi_audio_run_finalize(GObject *gobject);

void ags_route_dssi_audio_run_connect(AgsConnectable *connectable);
void ags_route_dssi_audio_run_disconnect(AgsConnectable *connectable);
void ags_route_dssi_audio_run_connect_connection(AgsConnectable *connectable,
						 GObject *connection);
void ags_route_dssi_audio_run_disconnect_connection(AgsConnectable *connectable,
						    GObject *connection);

void ags_route_dssi_audio_run_resolve_dependency(AgsRecall *recall);
void ags_route_dssi_audio_run_run_post(AgsRecall *recall);

void ags_route_dssi_audio_run_alloc_input_callback_feed_note(AgsRouteDssiAudioRun *route_dssi_audio_run,
							     AgsNotation *notation,
							     guint audio_start_mapping, guint audio_end_mapping,
							     guint64 notation_counter);

void ags_route_dssi_audio_run_alloc_input_callback(AgsDelayAudioRun *delay_audio_run,
						   guint nth_run,
						   gdouble delay, guint attack,
						   AgsRouteDssiAudioRun *route_dssi_audio_run);

void ags_route_dssi_audio_run_feed_midi(AgsRecall *recall,
					AgsNote *note);

/**
 * SECTION:ags_route_dssi_audio_run
 * @short_description: route MIDI
 * @title: AgsRouteDssiAudioRun
 * @section_id:
 * @include: ags/audio/recall/ags_route_dssi_audio_run.h
 *
 * The #AgsRouteDssiAudioRun routes MIDI to DSSI recall.
 */

enum{
  PROP_0,
  PROP_DELAY_AUDIO_RUN,
  PROP_COUNT_BEATS_AUDIO_RUN,
};

static gpointer ags_route_dssi_audio_run_parent_class = NULL;
static AgsConnectableInterface* ags_route_dssi_audio_run_parent_connectable_interface;

GType
ags_route_dssi_audio_run_get_type()
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_route_dssi_audio_run;

    static const GTypeInfo ags_route_dssi_audio_run_info = {
      sizeof (AgsRouteDssiAudioRunClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_route_dssi_audio_run_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsRouteDssiAudioRun),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_route_dssi_audio_run_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_route_dssi_audio_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_route_dssi_audio_run = g_type_register_static(AGS_TYPE_RECALL_AUDIO_RUN,
							   "AgsRouteDssiAudioRun",
							   &ags_route_dssi_audio_run_info,
							   0);
    
    g_type_add_interface_static(ags_type_route_dssi_audio_run,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_once_init_leave (&g_define_type_id__volatile, ags_type_route_dssi_audio_run);
  }

  return g_define_type_id__volatile;
}

void
ags_route_dssi_audio_run_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_route_dssi_audio_run_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_route_dssi_audio_run_connect;
  connectable->disconnect = ags_route_dssi_audio_run_disconnect;

  connectable->connect_connection = ags_route_dssi_audio_run_connect_connection;
  connectable->disconnect_connection = ags_route_dssi_audio_run_disconnect_connection;
}

void
ags_route_dssi_audio_run_class_init(AgsRouteDssiAudioRunClass *route_dssi_audio_run)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  GParamSpec *param_spec;

  ags_route_dssi_audio_run_parent_class = g_type_class_peek_parent(route_dssi_audio_run);

  /* GObjectClass */
  gobject = (GObjectClass *) route_dssi_audio_run;

  gobject->set_property = ags_route_dssi_audio_run_set_property;
  gobject->get_property = ags_route_dssi_audio_run_get_property;

  gobject->dispose = ags_route_dssi_audio_run_dispose;
  gobject->finalize = ags_route_dssi_audio_run_finalize;

  /* properties */
  /**
   * AgsRouteDssiAudioRun:delay-audio-run:
   * 
   * The delay audio run dependency.
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_object("delay-audio-run",
				   i18n_pspec("assigned AgsDelayAudioRun"),
				   i18n_pspec("the AgsDelayAudioRun which emits notation_alloc_input signal"),
				   AGS_TYPE_DELAY_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DELAY_AUDIO_RUN,
				  param_spec);

  /**
   * AgsRouteDssiAudioRun:count-beats-audio-run:
   * 
   * The count beats audio run dependency.
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_object("count-beats-audio-run",
				   i18n_pspec("assigned AgsCountBeatsAudioRun"),
				   i18n_pspec("the AgsCountBeatsAudioRun which just counts"),
				   AGS_TYPE_COUNT_BEATS_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_COUNT_BEATS_AUDIO_RUN,
				  param_spec);

  /* AgsRecallClass */
  recall = (AgsRecallClass *) route_dssi_audio_run;

  recall->resolve_dependency = ags_route_dssi_audio_run_resolve_dependency;
  recall->run_post = ags_route_dssi_audio_run_run_post;
}

void
ags_route_dssi_audio_run_init(AgsRouteDssiAudioRun *route_dssi_audio_run)
{
  ags_recall_set_ability_flags((AgsRecall *) route_dssi_audio_run, (AGS_SOUND_ABILITY_NOTATION));

  AGS_RECALL(route_dssi_audio_run)->name = "ags-route-dssi";
  AGS_RECALL(route_dssi_audio_run)->version = AGS_RECALL_DEFAULT_VERSION;
  AGS_RECALL(route_dssi_audio_run)->build_id = AGS_RECALL_DEFAULT_BUILD_ID;
  AGS_RECALL(route_dssi_audio_run)->xml_type = "ags-route-dssi-audio-run";
  AGS_RECALL(route_dssi_audio_run)->port = NULL;

  route_dssi_audio_run->delay_audio_run = NULL;
  route_dssi_audio_run->count_beats_audio_run = NULL;

  route_dssi_audio_run->notation = NULL;

  route_dssi_audio_run->timestamp = ags_timestamp_new();

  route_dssi_audio_run->timestamp->flags &= (~AGS_TIMESTAMP_UNIX);
  route_dssi_audio_run->timestamp->flags |= AGS_TIMESTAMP_OFFSET;

  route_dssi_audio_run->timestamp->timer.ags_offset.offset = 0;

  route_dssi_audio_run->sequencer = NULL;

  route_dssi_audio_run->feed_midi = NULL;
}

void
ags_route_dssi_audio_run_set_property(GObject *gobject,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *param_spec)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  GRecMutex *recall_mutex;

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(gobject);

  /* get recall mutex */
  recall_mutex = AGS_RECALL_GET_OBJ_MUTEX(route_dssi_audio_run);

  switch(prop_id){
  case PROP_DELAY_AUDIO_RUN:
    {
      AgsDelayAudioRun *delay_audio_run, *old_delay_audio_run;

      gboolean is_template;

      delay_audio_run = g_value_get_object(value);
      old_delay_audio_run = NULL;
      
      g_rec_mutex_lock(recall_mutex);

      if(delay_audio_run == route_dssi_audio_run->delay_audio_run){
	g_rec_mutex_unlock(recall_mutex);

	return;
      }

      if(route_dssi_audio_run->delay_audio_run != NULL){
	old_delay_audio_run = route_dssi_audio_run->delay_audio_run;

	g_object_unref(G_OBJECT(route_dssi_audio_run->delay_audio_run));
      }

      if(delay_audio_run != NULL){
	g_object_ref(delay_audio_run);
      }

      g_rec_mutex_unlock(recall_mutex);

      /* check template */
      if(delay_audio_run != NULL &&
	 ags_recall_test_flags((AgsRecall *) route_dssi_audio_run, AGS_RECALL_TEMPLATE)){
	is_template = TRUE;
      }else{
	is_template = FALSE;
      }

      /* old - dependency/connection */
      if(is_template){
	if(old_delay_audio_run != NULL){
	  AgsRecallDependency *recall_dependency;

	  GList *list;
	  
	  recall_dependency = NULL;
	  list = ags_recall_dependency_find_dependency(AGS_RECALL(route_dssi_audio_run)->recall_dependency,
						       (GObject *) old_delay_audio_run);

	  if(list != NULL){
	    recall_dependency = list->data;
	  }
	  
	  ags_recall_remove_recall_dependency(AGS_RECALL(route_dssi_audio_run),
					      recall_dependency);
	}
      }else{
	if(ags_connectable_is_connected(AGS_CONNECTABLE(route_dssi_audio_run))){
	  ags_connectable_disconnect_connection(AGS_CONNECTABLE(route_dssi_audio_run),
						(GObject *) old_delay_audio_run);
	}
      }

      /* new - dependency/connection */
      g_rec_mutex_lock(recall_mutex);

      route_dssi_audio_run->delay_audio_run = delay_audio_run;

      g_rec_mutex_unlock(recall_mutex);

      if(delay_audio_run != NULL){
	if(is_template){
	  ags_recall_add_recall_dependency(AGS_RECALL(route_dssi_audio_run),
					   ags_recall_dependency_new((GObject *) delay_audio_run));
	}else{
	  if(ags_connectable_is_connected(AGS_CONNECTABLE(route_dssi_audio_run))){
	    ags_connectable_connect_connection(AGS_CONNECTABLE(route_dssi_audio_run),
					       (GObject *) delay_audio_run);
	  }
	}
      }
    }
    break;
  case PROP_COUNT_BEATS_AUDIO_RUN:
    {
      AgsCountBeatsAudioRun *count_beats_audio_run, *old_count_beats_audio_run;

      gboolean is_template;

      count_beats_audio_run = g_value_get_object(value);
      old_count_beats_audio_run = NULL;
      
      g_rec_mutex_lock(recall_mutex);

      if(count_beats_audio_run == route_dssi_audio_run->count_beats_audio_run){
	g_rec_mutex_unlock(recall_mutex);
	
	return;
      }

      if((AGS_RECALL_TEMPLATE & (AGS_RECALL(route_dssi_audio_run)->flags)) != 0){
	is_template = TRUE;
      }else{
	is_template = FALSE;
      }

      if(route_dssi_audio_run->count_beats_audio_run != NULL){
	old_count_beats_audio_run = route_dssi_audio_run->count_beats_audio_run;

	g_object_unref(G_OBJECT(route_dssi_audio_run->count_beats_audio_run));
      }

      if(count_beats_audio_run != NULL){
	g_object_ref(count_beats_audio_run);
      }

      route_dssi_audio_run->count_beats_audio_run = count_beats_audio_run;

      g_rec_mutex_unlock(recall_mutex);

      /* check template */
      if(count_beats_audio_run != NULL &&
	 ags_recall_test_flags((AgsRecall *) route_dssi_audio_run, AGS_RECALL_TEMPLATE)){
	is_template = TRUE;
      }else{
	is_template = FALSE;
      }

      /* dependency - remove */
      if(is_template){
	if(old_count_beats_audio_run != NULL){
	  AgsRecallDependency *recall_dependency;

	  GList *list;
	  
	  recall_dependency = NULL;
	  list = ags_recall_dependency_find_dependency(AGS_RECALL(route_dssi_audio_run)->recall_dependency,
						       (GObject *) old_count_beats_audio_run);

	  if(list != NULL){
	    recall_dependency = list->data;
	  }
	  
	  ags_recall_remove_recall_dependency(AGS_RECALL(route_dssi_audio_run),
					      recall_dependency);
	}
      }

      /* dependency - add */
      if(is_template &&
	 count_beats_audio_run != NULL){
	ags_recall_add_recall_dependency(AGS_RECALL(route_dssi_audio_run),
					 ags_recall_dependency_new((GObject *) count_beats_audio_run));
      }
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_route_dssi_audio_run_get_property(GObject *gobject,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *param_spec)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  GRecMutex *recall_mutex;
  
  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(gobject);

  /* get recall mutex */
  recall_mutex = AGS_RECALL_GET_OBJ_MUTEX(route_dssi_audio_run);

  switch(prop_id){
  case PROP_DELAY_AUDIO_RUN:
    {
      g_rec_mutex_lock(recall_mutex);

      g_value_set_object(value,
			 G_OBJECT(route_dssi_audio_run->delay_audio_run));

      g_rec_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_COUNT_BEATS_AUDIO_RUN:
    {
      g_rec_mutex_lock(recall_mutex);

      g_value_set_object(value,
			 route_dssi_audio_run->count_beats_audio_run);

      g_rec_mutex_unlock(recall_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_route_dssi_audio_run_dispose(GObject *gobject)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(gobject);

  /* delay audio run */
  if(route_dssi_audio_run->delay_audio_run != NULL){
    g_object_unref(G_OBJECT(route_dssi_audio_run->delay_audio_run));

    route_dssi_audio_run->delay_audio_run = NULL;
  }

  /* count beats audio run */
  if(route_dssi_audio_run->count_beats_audio_run != NULL){
    g_object_unref(G_OBJECT(route_dssi_audio_run->count_beats_audio_run));

    route_dssi_audio_run->count_beats_audio_run = NULL;
  }

  /* feed midi */
  g_list_free_full(route_dssi_audio_run->feed_midi,
		   g_object_unref);

  route_dssi_audio_run->feed_midi = NULL;

  /* call parent */
  G_OBJECT_CLASS(ags_route_dssi_audio_run_parent_class)->dispose(gobject);
}

void
ags_route_dssi_audio_run_finalize(GObject *gobject)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(gobject);

  /* delay audio run */
  if(route_dssi_audio_run->delay_audio_run != NULL){
    g_object_unref(G_OBJECT(route_dssi_audio_run->delay_audio_run));
  }

  /* count beats audio run */
  if(route_dssi_audio_run->count_beats_audio_run != NULL){
    g_object_unref(G_OBJECT(route_dssi_audio_run->count_beats_audio_run));
  }

  /* timestamp */
  if(route_dssi_audio_run->timestamp != NULL){
    g_object_unref(G_OBJECT(route_dssi_audio_run->timestamp));
  }

  g_list_free_full(route_dssi_audio_run->feed_midi,
		   g_object_unref);  

  /* call parent */
  G_OBJECT_CLASS(ags_route_dssi_audio_run_parent_class)->finalize(gobject);
}

void
ags_route_dssi_audio_run_connect(AgsConnectable *connectable)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;
  AgsDelayAudioRun *delay_audio_run;
  
  if(ags_connectable_is_connected(connectable)){
    return;
  }

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(connectable);
  
  g_object_get(route_dssi_audio_run,
	       "delay-audio-run", &delay_audio_run,
	       NULL);

  ags_connectable_connect_connection(connectable, (GObject *) delay_audio_run);
  
  /* call parent */
  ags_route_dssi_audio_run_parent_connectable_interface->connect(connectable);

  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }
}

void
ags_route_dssi_audio_run_disconnect(AgsConnectable *connectable)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;  
  AgsDelayAudioRun *delay_audio_run;
  
  if(!ags_connectable_is_connected(connectable)){
    return;
  }

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(connectable);
  
  g_object_get(route_dssi_audio_run,
	       "delay-audio-run", &delay_audio_run,
	       NULL);

  ags_connectable_disconnect_connection(connectable, (GObject *) delay_audio_run);

  /* call parent */
  ags_route_dssi_audio_run_parent_connectable_interface->disconnect(connectable);

  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }
}

void
ags_route_dssi_audio_run_connect_connection(AgsConnectable *connectable, GObject *connection)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;  
  AgsDelayAudioRun *delay_audio_run;

  if(connection == NULL){
    return;
  }

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(connectable);

  g_object_get(route_dssi_audio_run,
	       "delay-audio-run", &delay_audio_run,
	       NULL);

  if(connection == (GObject *) delay_audio_run){
    g_signal_connect(G_OBJECT(delay_audio_run), "notation-alloc-input",
		     G_CALLBACK(ags_route_dssi_audio_run_alloc_input_callback), route_dssi_audio_run);  
  }

  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }
}

void
ags_route_dssi_audio_run_disconnect_connection(AgsConnectable *connectable, GObject *connection)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;  
  AgsDelayAudioRun *delay_audio_run;

  if(connection == NULL){
    return;
  }
  
  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(connectable);

  g_object_get(route_dssi_audio_run,
	       "delay-audio-run", &delay_audio_run,
	       NULL);

  if(connection == (GObject *) delay_audio_run){
    g_object_disconnect(G_OBJECT(delay_audio_run),
			"any_signal::notation-alloc-input",
			G_CALLBACK(ags_route_dssi_audio_run_alloc_input_callback),
			route_dssi_audio_run,
			NULL);
  }

  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }
}

void
ags_route_dssi_audio_run_resolve_dependency(AgsRecall *recall)
{
  AgsRecall *template;
  AgsRecallID *recall_id;
  AgsRecallContainer *recall_container;
  AgsRecallDependency *recall_dependency;
  AgsDelayAudioRun *delay_audio_run;
  AgsCountBeatsAudioRun *count_beats_audio_run;

  GList *list_start, *list;

  guint i, i_stop;

  g_object_get(recall,
	       "recall-id", &recall_id,
	       "recall-container", &recall_container,
	       NULL);

  g_object_get(recall_container,
	       "recall-audio-run", &list_start,
	       NULL);

  template = NULL;
  list = ags_recall_find_template(list_start);

  if(list != NULL){
    template = AGS_RECALL(list->data);
  }

  g_list_free_full(list_start,
		   g_object_unref);

  g_object_get(template,
	       "recall-dependency", &list_start,
	       NULL);

  list = list_start;

  delay_audio_run = NULL;
  count_beats_audio_run = NULL;

  i_stop = 2;

  for(i = 0; i < i_stop && list != NULL;){
    GObject *dependency;
    
    recall_dependency = AGS_RECALL_DEPENDENCY(list->data);
    
    g_object_get(recall_dependency,
		 "dependency", &dependency,
		 NULL);
    
    if(AGS_IS_DELAY_AUDIO_RUN(dependency)){
      delay_audio_run = (AgsDelayAudioRun *) ags_recall_dependency_resolve(recall_dependency,
									   recall_id);
      
      i++;
    }else if(AGS_IS_COUNT_BEATS_AUDIO_RUN(dependency)){
      count_beats_audio_run = (AgsCountBeatsAudioRun *) ags_recall_dependency_resolve(recall_dependency,
										      recall_id);

      i++;
    }

    g_object_unref(dependency);
    
    /* iterate */
    list = list->next;
  }

  g_list_free_full(list_start,
		   g_object_unref);
  
  g_object_set(G_OBJECT(recall),
	       "delay-audio-run", delay_audio_run,
	       "count-beats-audio-run", count_beats_audio_run,
	       NULL);

  /* unref */
  g_object_unref(recall_id);

  g_object_unref(recall_container);
}

void
ags_route_dssi_audio_run_feed_midi(AgsRecall *recall,
				   AgsNote *note)
{
  AgsAudio *audio;
  AgsChannel *start_output, *start_input;
  AgsChannel *channel, *selected_channel, *nth_channel;
  AgsRecallDssiRun *recall_dssi_run;
  AgsRecallID *recall_id;
  AgsRecallID *child_recall_id;
  AgsRecyclingContext *recycling_context;
  AgsRecyclingContext *child_recycling_context;
  
  AgsDelayAudioRun *delay_audio_run;
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  GList *start_generic_channel_recall, *generic_channel_recall;
  GList *start_generic_recycling_recall, *generic_recycling_recall;
  GList *start_dssi_run, *dssi_run;
  GList *start_list_recall, *list_recall;
  GList *start_list, *list;
  
  snd_seq_event_t *seq_event;

  gchar *str;

  guint audio_start_mapping;
  guint midi_start_mapping, midi_end_mapping;
  guint audio_channel;
  guint pads;
  guint output_pads, input_pads;
  guint selected_pad;
  guint start_frame, end_frame;
  guint note_y;

  GRecMutex *audio_mutex;
  
  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(recall);

  audio = NULL;

  recall_id = NULL;
  recycling_context = NULL;
  
  delay_audio_run = NULL;
  
  g_object_get(route_dssi_audio_run,
	       "audio", &audio,
	       "audio-channel", &audio_channel,
	       "recall-id", &recall_id,
	       "delay-audio-run", &delay_audio_run,
	       NULL);

  g_object_get(recall_id,
	       "recycling-context", &recycling_context,
	       NULL);
  
  /* audio mutex */
  audio_mutex = AGS_AUDIO_GET_OBJ_MUTEX(audio);

  /* get audio fields */
  g_rec_mutex_lock(audio_mutex);

  audio_start_mapping = audio->audio_start_mapping;

  midi_start_mapping = audio->midi_start_mapping;
  midi_end_mapping = audio->midi_end_mapping;

  output_pads = audio->output_pads;
  input_pads = audio->input_pads;

  start_output = audio->output;

  if(start_output != NULL){
    g_object_ref(start_output);
  }
  
  start_input = audio->input;

  if(start_input != NULL){
    g_object_ref(start_input);
  }

  g_rec_mutex_unlock(audio_mutex);

  /* note */
  g_object_get(note,
	       "y", &note_y,
	       NULL);  

  /* get channel */
  if(ags_audio_test_behaviour_flags(audio, AGS_SOUND_BEHAVIOUR_DEFAULTS_TO_INPUT)){
    selected_channel = ags_channel_nth(start_input,
				       audio_channel);

    pads = input_pads;
  }else{
    selected_channel = ags_channel_nth(start_output,
				       audio_channel);

    pads = output_pads;
  }

  if(ags_audio_test_behaviour_flags(audio, AGS_SOUND_BEHAVIOUR_REVERSE_MAPPING)){
    nth_channel = ags_channel_pad_nth(selected_channel,
				      audio_start_mapping + pads - note_y - 1);

    g_object_unref(selected_channel);
    
    selected_channel = nth_channel;
  }else{
    nth_channel = ags_channel_pad_nth(selected_channel,
				      audio_start_mapping + note_y);

    g_object_unref(selected_channel);
    
    selected_channel = nth_channel;
  }

  /* check within mapping */
  g_object_get(selected_channel,
	       "pad", &selected_pad,
	       NULL);

  if(selected_pad - audio_start_mapping + midi_start_mapping < midi_start_mapping ||
     selected_pad - audio_start_mapping + midi_start_mapping >= midi_end_mapping){
    g_object_unref(audio);
    
    g_object_unref(recall_id);

    g_object_unref(delay_audio_run);

    g_object_unref(recycling_context);

    if(start_output != NULL){
      g_object_unref(start_output);
    }

    if(start_input != NULL){
      g_object_unref(start_input);
    }

    if(selected_channel != NULL){
      g_object_unref(selected_channel);
    }

    return;
  }
  
  /* get recall id */
  if(selected_channel != NULL){
    g_object_get(selected_channel,
		 "recall-id", &start_list,
		 NULL);

    child_recall_id = NULL;
    list = start_list;
      
    while(list != NULL){
      AgsRecyclingContext *parent_current_recycling_context, *current_recycling_context;
	
      if(AGS_RECALL_ID(list->data)->recycling_context->parent == recycling_context){
	child_recall_id = (AgsRecallID *) list->data;

	break;
      }
	  
      list = list->next;
    }

    g_list_free_full(start_list,
		     g_object_unref);

    /* get dssi run */
    if(child_recall_id != NULL){
      g_object_get(child_recall_id,
		   "recycling-context", &child_recycling_context,
		   NULL);
      
      g_object_get(selected_channel,
		   "recall", &start_list_recall,
		   NULL);
     
      //FIXME:JK: use filename and effect to identify
      start_generic_channel_recall = NULL;

      list_recall = ags_recall_template_find_type(start_list_recall,
						  AGS_TYPE_RECALL_DSSI);

      if(list_recall != NULL){
	AgsRecallContainer *recall_container;

	g_object_get(list_recall->data,
		     "recall-container", &recall_container,
		     NULL);

	g_object_get(recall_container,
		     "recall-channel-run", &start_generic_channel_recall,
		     NULL);
	
	g_object_unref(recall_container);
      }

      g_list_free_full(start_list_recall,
		       g_object_unref);
      
      generic_channel_recall = start_generic_channel_recall;
      
      while(generic_channel_recall != NULL){
	AgsRecallID *current_recall_id;
	AgsRecyclingContext *current_recycling_context;

	g_object_get(generic_channel_recall->data,
		     "recall-id", &current_recall_id,
		     NULL);

	if(current_recall_id == NULL){
	  generic_channel_recall = generic_channel_recall->next;
	  
	  continue;
	}
	
	g_object_get(current_recall_id,
		     "recycling-context", &current_recycling_context,
		     NULL);
	
	if(current_recycling_context != child_recycling_context){
	  generic_channel_recall = generic_channel_recall->next;

	  g_object_unref(current_recall_id);
	  
	  continue;
	}

	g_object_get(generic_channel_recall->data,
		     "child", &start_generic_recycling_recall,
		     NULL);

	generic_recycling_recall = start_generic_recycling_recall;

	while(generic_recycling_recall != NULL){
	  g_object_get(generic_recycling_recall->data,
		       "child", &start_dssi_run,
		       NULL);

	  dssi_run = start_dssi_run;
	  
	  while(dssi_run != NULL){
	    recall_dssi_run = AGS_RECALL_DSSI_RUN(dssi_run->data);
	    
	    if(ags_recall_global_get_rt_safe() ||
	       recall_dssi_run->note == NULL){
	      /* prepend note */
	      //		route_dssi_audio_run->feed_midi = g_list_prepend(route_dssi_audio_run->feed_midi,
	      //						 note);

	      g_object_set(recall_dssi_run,
			   "route-dssi-audio-run", route_dssi_audio_run,
			   NULL);

	      /* key on */
	      seq_event = recall_dssi_run->event_buffer[0];
		
	      seq_event->type = SND_SEQ_EVENT_NOTEON;

	      seq_event->data.note.channel = 0;
	      seq_event->data.note.note = 0x7f & (selected_channel->pad - audio_start_mapping + midi_start_mapping);
	      seq_event->data.note.velocity = 127;

	      recall_dssi_run->event_count[0] = 1;
		  
	      g_object_set(recall_dssi_run,
			   "audio-channel", audio_channel,
			   NULL);
	      
	      recall_dssi_run->note = g_list_prepend(recall_dssi_run->note,
						     (GObject *) note);
	      g_object_ref(note);
	    }

	    //fixme:jk: remove notes

	    dssi_run = dssi_run->next;
	  }

	  /* unref */
	  g_list_free_full(start_dssi_run,
			   g_object_unref);

	  generic_recycling_recall = generic_recycling_recall->next;
	}

	/* unref */
	g_object_unref(current_recall_id);
	  
	g_object_unref(current_recycling_context);
	  
	g_list_free_full(start_generic_recycling_recall,
			 g_object_unref);
	
	generic_channel_recall = generic_channel_recall->next;
      }      

      g_object_unref(child_recycling_context);

      g_list_free_full(start_generic_channel_recall,
		       g_object_unref);
    }
  }

  /* unref */
  if(audio != NULL){
    g_object_unref(audio);
  }

  if(recall_id != NULL){
    g_object_unref(recall_id);
  }

  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }

  if(recycling_context != NULL){
    g_object_unref(recycling_context);
  }
  
  if(start_output != NULL){
    g_object_unref(start_output);
  }

  if(start_input != NULL){
    g_object_unref(start_input);
  }

  if(selected_channel != NULL){
    g_object_unref(selected_channel);
  }
}

void
ags_route_dssi_audio_run_alloc_input_callback_feed_note(AgsRouteDssiAudioRun *route_dssi_audio_run,
							AgsNotation *notation,
							guint audio_start_mapping, guint audio_end_mapping,
							guint64 notation_counter)
{
  AgsNote *current_note;
    
  GList *start_note, *note;
  GList *start_list, *list;

  guint note_y;
  guint note_x0;

  if(!AGS_IS_NOTATION(notation)){
    return;
  }

  start_list = NULL;    

  g_object_get(notation,
	       "note", &start_note,
	       NULL);

  note = start_note;
    
  while(note != NULL){
    current_note = AGS_NOTE(note->data);

    g_object_get(current_note,
		 "x0", &note_x0,
		 "y", &note_y,
		 NULL);
#if 0    
    g_message("--- %f %f ; %d %d",
	      current_note->stream_delay, delay,
	      note_x0, route_dssi_audio_run->count_beats_audio_run->notation_counter);
#endif
      
    //FIXME:JK: should consider delay
    if(note_y >= audio_start_mapping &&
       note_y < audio_end_mapping &&
       note_x0 == notation_counter){ // && floor(note->stream_delay) == floor(delay)
      start_list = g_list_prepend(start_list,
				  current_note);
      g_object_ref(current_note);
    }else if(note_x0 > notation_counter){
      break;
    }
    
    /* iterate */    
    note = note->next;
  }

  start_list = g_list_reverse(start_list);
    
  /* feed midi */
  list = start_list;
    
  while(list != NULL){
    ags_route_dssi_audio_run_feed_midi((AgsRecall *) route_dssi_audio_run,
				       list->data);
    g_object_unref(list->data);
      
    list = list->next;
  }

  g_list_free_full(start_note,
		   g_object_unref);
  g_list_free(start_list);
}

void
ags_route_dssi_audio_run_alloc_input_callback(AgsDelayAudioRun *delay_audio_run,
					      guint nth_run,
					      gdouble delay, guint attack,
					      AgsRouteDssiAudioRun *route_dssi_audio_run)
{
  AgsAudio *audio;
  AgsNotation *notation;
  AgsCountBeatsAudioRun *count_beats_audio_run;
  AgsRouteDssiAudio *route_dssi_audio;

  AgsTimestamp *timestamp;
  
  GObject *output_soundcard;

  GList *start_list, *list;

  guint audio_channel;
  guint64 notation_counter;
  guint audio_start_mapping, audio_end_mapping;
    
  if((guint) floor(delay) != 0){
    //    g_message("d %f", delay);
    return;
  }

  audio = NULL;

  route_dssi_audio = NULL;
  count_beats_audio_run = NULL;
  
  g_object_get(route_dssi_audio_run,
	       "audio", &audio,
	       "audio-channel", &audio_channel,
	       "recall-audio", &route_dssi_audio,
	       "count-beats-audio-run", &count_beats_audio_run,
	       NULL);

  /* feed note - first attempt */
  g_object_get(count_beats_audio_run,
	       "notation-counter", &notation_counter,
	       NULL);
  
  route_dssi_audio_run->timestamp->timer.ags_offset.offset = AGS_NOTATION_DEFAULT_OFFSET * floor(notation_counter / AGS_NOTATION_DEFAULT_OFFSET);

  g_object_get(audio,
	       "audio-start-mapping", &audio_start_mapping,
	       "audio-end-mapping", &audio_end_mapping,
	       "notation", &start_list,
	       NULL);

  notation = NULL;
  list = ags_notation_find_near_timestamp(start_list, audio_channel,
					  route_dssi_audio_run->timestamp);

  if(list != NULL){
    notation = list->data;
  }
  
  ags_route_dssi_audio_run_alloc_input_callback_feed_note(route_dssi_audio_run,
							  notation,
							  audio_start_mapping, audio_end_mapping,
							  notation_counter);
  
  /* feed note - second attempt */
  if(route_dssi_audio_run->timestamp->timer.ags_offset.offset != 0){
    route_dssi_audio_run->timestamp->timer.ags_offset.offset -= AGS_NOTATION_DEFAULT_OFFSET;

    notation = NULL;
    list = ags_notation_find_near_timestamp(start_list, audio_channel,
					    route_dssi_audio_run->timestamp);

    if(list != NULL){
      notation = list->data;
    }

    ags_route_dssi_audio_run_alloc_input_callback_feed_note(route_dssi_audio_run,
							    notation,
							    audio_start_mapping, audio_end_mapping,
							    notation_counter);
  }

  /* unref */
  if(audio != NULL){
    g_object_unref(audio);
  }

  if(route_dssi_audio != NULL){
    g_object_unref(route_dssi_audio);
  }
  
  if(count_beats_audio_run != NULL){
    g_object_unref(count_beats_audio_run);
  }
  
  g_list_free_full(start_list,
		   g_object_unref);
}

void
ags_route_dssi_audio_run_run_post(AgsRecall *recall)
{
  AgsAudio *audio;
  AgsPort *port;
  
  AgsDelayAudio *delay_audio;
  AgsDelayAudioRun *delay_audio_run;
  AgsCountBeatsAudioRun *count_beats_audio_run;
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  GObject *output_soundcard;
 
  gdouble bpm;
  guint64 notation_counter;
  gdouble notation_delay;
  guint buffer_size;
  gdouble x;
  
  GValue value = {0,};

  route_dssi_audio_run = AGS_ROUTE_DSSI_AUDIO_RUN(recall);

  audio = NULL;
  output_soundcard = NULL;

  delay_audio = NULL;
  delay_audio_run = NULL;
  count_beats_audio_run = NULL;

  g_object_get(route_dssi_audio_run,
	       "audio", &audio,
	       "delay-audio-run", &delay_audio_run,
	       "count-beats-audio-run", &count_beats_audio_run,
	       NULL);

  g_object_get(delay_audio_run,
	       "recall-audio", &delay_audio,
	       NULL);

  g_object_get(audio,
	       "output-soundcard", &output_soundcard,
	       NULL);
  
  g_object_get(delay_audio,
	       "buffer-size", &buffer_size,
	       "notation-delay", &port,
	       NULL);

  g_object_get(count_beats_audio_run,
	       "notation-counter", &notation_counter,
	       NULL);

  /* get notation delay */
  g_value_init(&value, G_TYPE_DOUBLE);

  ags_port_safe_read(port,
		     &value);

  notation_delay = g_value_get_double(&value);

  g_value_unset(&value);

  g_object_unref(port);
  
  /* get bpm */
  g_object_get(delay_audio,
	       "bpm", &port,
	       NULL);
  
  g_value_init(&value, G_TYPE_DOUBLE);

  ags_port_safe_read(port,
		     &value);

  bpm = g_value_get_double(&value);

  g_value_unset(&value);

  g_object_unref(port);
  
  /*  */
  x = (((notation_counter * notation_delay) + notation_counter) * buffer_size);
  
  route_dssi_audio_run->delta_time = x / 16.0 / bpm * 60.0 / ((AGS_USEC_PER_SEC * bpm / 4.0) / (4.0 * bpm) / AGS_USEC_PER_SEC);

  /* unref */
  if(audio != NULL){
    g_object_unref(audio);
  }
  
  if(count_beats_audio_run != NULL){
    g_object_unref(count_beats_audio_run);
  }
  
  if(delay_audio_run != NULL){
    g_object_unref(delay_audio_run);
  }

  if(delay_audio != NULL){
    g_object_unref(delay_audio);
  }

  if(output_soundcard != NULL){
    g_object_unref(output_soundcard);
  }
}

/**
 * ags_route_dssi_audio_run_new:
 * @audio: the #AgsAudio
 *
 * Create a new instance of #AgsRouteDssiAudioRun
 *
 * Returns: the new #AgsRouteDssiAudioRun
 *
 * Since: 3.0.0
 */
AgsRouteDssiAudioRun*
ags_route_dssi_audio_run_new(AgsAudio *audio)
{
  AgsRouteDssiAudioRun *route_dssi_audio_run;

  route_dssi_audio_run = (AgsRouteDssiAudioRun *) g_object_new(AGS_TYPE_ROUTE_DSSI_AUDIO_RUN,
							       "audio", audio,
							       NULL);
  
  return(route_dssi_audio_run);
}
