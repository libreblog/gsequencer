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

#include <ags/audio/ags_recall_channel_run.h>

#include <ags/libags.h>

#include <ags/audio/ags_audio.h>
#include <ags/audio/ags_channel.h>
#include <ags/audio/ags_output.h>
#include <ags/audio/ags_input.h>
#include <ags/audio/ags_recall_id.h>
#include <ags/audio/ags_recall_audio.h>
#include <ags/audio/ags_recall_audio_run.h>
#include <ags/audio/ags_recall_channel.h>
#include <ags/audio/ags_recall_container.h>
#include <ags/audio/ags_recall_recycling.h>

#include <ags/audio/task/ags_cancel_recall.h>

#include <ags/i18n.h>

void ags_recall_channel_run_class_init(AgsRecallChannelRunClass *recall_channel_run);
void ags_recall_channel_run_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_recall_channel_run_init(AgsRecallChannelRun *recall_channel_run);
void ags_recall_channel_run_set_property(GObject *gobject,
					 guint prop_id,
					 const GValue *value,
					 GParamSpec *param_spec);
void ags_recall_channel_run_get_property(GObject *gobject,
					 guint prop_id,
					 GValue *value,
					 GParamSpec *param_spec);
void ags_recall_channel_run_dispose(GObject *gobject);
void ags_recall_channel_run_finalize(GObject *gobject);

void ags_recall_channel_run_notify_recall_container_callback(GObject *gobject,
							     GParamSpec *pspec,
							     gpointer user_data);

void ags_recall_channel_run_connect(AgsConnectable *connectable);
void ags_recall_channel_run_disconnect(AgsConnectable *connectable);

void ags_recall_channel_run_remove(AgsRecall *recall);
AgsRecall* ags_recall_channel_run_duplicate(AgsRecall *recall,
					    AgsRecallID *recall_id,
					    guint *n_params, GParameter *parameter);

void ags_recall_channel_run_map_recall_recycling(AgsRecallChannelRun *recall_channel_run);

void ags_recall_channel_run_remap_child_source(AgsRecallChannelRun *recall_channel_run,
					       AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
					       AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region);
void ags_recall_channel_run_remap_child_destination(AgsRecallChannelRun *recall_channel_run,
						    AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
						    AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region);
void ags_recall_channel_run_source_recycling_changed_callback(AgsChannel *channel,
							      AgsRecycling *old_start_region, AgsRecycling *old_end_region,
							      AgsRecycling *new_start_region, AgsRecycling *new_end_region,
							      AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
							      AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region,
							      AgsRecallChannelRun *recall_channel_run);

void ags_recall_channel_run_destination_recycling_changed_callback(AgsChannel *channel,
								   AgsRecycling *old_start_region, AgsRecycling *old_end_region,
								   AgsRecycling *new_start_region, AgsRecycling *new_end_region,
								   AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
								   AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region,
								   AgsRecallChannelRun *recall_channel_run);

/**
 * SECTION:ags_recall_channel_run
 * @short_description: dynamic channel context of recall
 * @title: AgsRecallChannelRun
 * @section_id:
 * @include: ags/audio/ags_recall_channel_run.h
 *
 * #AgsRecallChannelRun acts as channel recall run.
 */

enum{
  PROP_0,
  PROP_RECALL_AUDIO,
  PROP_RECALL_AUDIO_RUN,
  PROP_RECALL_CHANNEL,
  PROP_DESTINATION,
  PROP_SOURCE,
};

static gpointer ags_recall_channel_run_parent_class = NULL;
static AgsConnectableInterface* ags_recall_channel_run_parent_connectable_interface;

GType
ags_recall_channel_run_get_type()
{
  static GType ags_type_recall_channel_run = 0;

  if(!ags_type_recall_channel_run){
    static const GTypeInfo ags_recall_channel_run_info = {
      sizeof (AgsRecallChannelRunClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_recall_channel_run_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsRecallChannelRun),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_recall_channel_run_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_recall_channel_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_recall_channel_run = g_type_register_static(AGS_TYPE_RECALL,
							 "AgsRecallChannelRun",
							 &ags_recall_channel_run_info,
							 0);
    
    g_type_add_interface_static(ags_type_recall_channel_run,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
  }

  return(ags_type_recall_channel_run);
}

void
ags_recall_channel_run_class_init(AgsRecallChannelRunClass *recall_channel_run)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  GParamSpec *param_spec;

  ags_recall_channel_run_parent_class = g_type_class_peek_parent(recall_channel_run);

  /* GObjectClass */
  gobject = (GObjectClass *) recall_channel_run;

  gobject->set_property = ags_recall_channel_run_set_property;
  gobject->get_property = ags_recall_channel_run_get_property;

  gobject->dispose = ags_recall_channel_run_dispose;
  gobject->finalize = ags_recall_channel_run_finalize;

  /* properties */
  /**
   * AgsRecallChannelRun:recall-audio:
   *
   * The audio context of this recall.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("recall-audio",
				   i18n_pspec("AgsRecallAudio of this recall"),
				   i18n_pspec("The AgsRecallAudio which this recall needs"),
				   AGS_TYPE_RECALL_AUDIO,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_RECALL_AUDIO,
				  param_spec);

  /**
   * AgsRecallChannelRun:recall-audio-run:
   *
   * The audio run context of this recall.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("recall-audio-run",
				   i18n_pspec("AgsRecallAudioRun of this recall"),
				   i18n_pspec("The AgsRecallAudioRun which this recall needs"),
				   AGS_TYPE_RECALL_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_RECALL_AUDIO_RUN,
				  param_spec);

  /**
   * AgsRecallChannelRun:recall-channel:
   *
   * The channel context of this recall.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("recall-channel",
				   i18n_pspec("AsgRecallChannel of this recall"),
				   i18n_pspec("The AgsRecallChannel which this recall needs"),
				   AGS_TYPE_RECALL_CHANNEL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_RECALL_CHANNEL,
				  param_spec);

  /**
   * AgsRecallChannelRun:destination:
   *
   * The channel to do output to.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("destination",
				   i18n_pspec("destination of output"),
				   i18n_pspec("The destination AgsChannel where it will output to"),
				   AGS_TYPE_CHANNEL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DESTINATION,
				  param_spec);

  /**
   * AgsRecallChannelRun:source:
   *
   * The channel to do input from.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("source",
				   i18n_pspec("source of input"),
				   i18n_pspec("The source AgsChannel where it will take the input from"),
				   AGS_TYPE_CHANNEL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_SOURCE,
				  param_spec);

  /* AgsRecallClass */
  recall = (AgsRecallClass *) recall_channel_run;

  recall->duplicate = ags_recall_channel_run_duplicate;
  recall->remove = ags_recall_channel_run_remove;
}

void
ags_recall_channel_run_connectable_interface_init(AgsConnectableInterface *connectable)
{
  AgsConnectableInterface *ags_recall_channel_run_connectable_parent_interface;

  ags_recall_channel_run_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_recall_channel_run_connect;
  connectable->disconnect = ags_recall_channel_run_disconnect;
}

void
ags_recall_channel_run_init(AgsRecallChannelRun *recall_channel_run)
{
  g_signal_connect_after(recall_channel_run, "notify::recall-container",
			 G_CALLBACK(ags_recall_channel_run_notify_recall_container_callback), NULL);

  recall_channel_run->recall_audio = NULL;
  recall_channel_run->recall_audio_run = NULL;
  recall_channel_run->recall_channel = NULL;

  recall_channel_run->source = NULL;
  recall_channel_run->destination = NULL;
}


void
ags_recall_channel_run_set_property(GObject *gobject,
				    guint prop_id,
				    const GValue *value,
				    GParamSpec *param_spec)
{
  AgsRecallChannelRun *recall_channel_run;

  pthread_mutex_t *recall_mutex;

  recall_channel_run = AGS_RECALL_CHANNEL_RUN(gobject);

  /* get recall mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_mutex = AGS_RECALL(recall_channel_run)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  switch(prop_id){
  case PROP_RECALL_AUDIO_RUN:
    {
      AgsRecallAudioRun *recall_audio_run;

      recall_audio_run = (AgsRecallAudioRun *) g_value_get_object(value);

      pthread_mutex_lock(recall_mutex);

      if(recall_channel_run->recall_audio_run == recall_audio_run){
	pthread_mutex_unlock(recall_mutex);

	return;
      }

      if(recall_channel_run->recall_audio_run != NULL){
	g_object_unref(recall_channel_run->recall_audio_run);
      }

      if(recall_audio_run != NULL){
	g_object_ref(G_OBJECT(recall_audio_run));
      }

      recall_channel_run->recall_audio_run = recall_audio_run;
      
      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_RECALL_CHANNEL:
    {
      AgsRecallChannel *recall_channel;

      recall_channel = (AgsRecallChannel *) g_value_get_object(value);

      pthread_mutex_lock(recall_mutex);

      if(recall_channel_run->recall_channel == recall_channel){
	pthread_mutex_unlock(recall_mutex);

	return;
      }

      if(recall_channel_run->recall_channel != NULL){
	g_object_unref(recall_channel_run->recall_channel);
      }

      if(recall_channel != NULL){
	g_object_ref(G_OBJECT(recall_channel));
      }

      recall_channel_run->recall_channel = recall_channel;
      
      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_DESTINATION:
    {
      AgsChannel *destination;
      AgsChannel *old_destination;

      destination = (AgsChannel *) g_value_get_object(value);

      pthread_mutex_lock(recall_mutex);

      if(recall_channel_run->destination == destination){
	pthread_mutex_unlock(recall_mutex);

	return;
      }

      old_destination = recall_channel_run->destination;

      if(destination != NULL){    
	g_object_ref(G_OBJECT(destination));
      }

      recall_channel_run->destination = destination;

      /* child destination */
      if(destination == recall_channel_run->source){
	g_warning("destination == recall_channel_run->source");
      }
      
      if(old_destination != NULL){
	g_object_unref(G_OBJECT(old_destination));
      }

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_SOURCE:
    {
      AgsChannel *source;
      AgsChannel *old_source;

      source = (AgsChannel *) g_value_get_object(value);

      pthread_mutex_lock(recall_mutex);

      if(recall_channel_run->source == source){
	pthread_mutex_unlock(recall_mutex);

	return;
      }

      old_source = recall_channel_run->source;

      if(source != NULL){
	g_object_ref(G_OBJECT(source));
      }

      recall_channel_run->source = source;

      if(source == recall_channel_run->destination){
	g_warning("destination == recall_channel_run->source");
      }
      
      if(old_source != NULL){
	g_object_unref(G_OBJECT(old_source));
      }

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_recall_channel_run_get_property(GObject *gobject,
				    guint prop_id,
				    GValue *value,
				    GParamSpec *param_spec)
{
  AgsRecallChannelRun *recall_channel_run;

  pthread_mutex_t *recall_mutex;

  recall_channel_run = AGS_RECALL_CHANNEL_RUN(gobject);

  /* get recall mutex */
  pthread_mutex_lock(ags_recall_get_class_mutex());
  
  recall_mutex = AGS_RECALL(recall_channel_run)->obj_mutex;
  
  pthread_mutex_unlock(ags_recall_get_class_mutex());

  switch(prop_id){
  case PROP_RECALL_AUDIO:
    {
      pthread_mutex_lock(recall_mutex);
      
      g_value_set_object(value, recall_channel_run->recall_audio);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_RECALL_AUDIO_RUN:
    {
      pthread_mutex_lock(recall_mutex);
      
      g_value_set_object(value, recall_channel_run->recall_audio_run);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_RECALL_CHANNEL:
    {
      pthread_mutex_lock(recall_mutex);
      
      g_value_set_object(value, recall_channel_run->recall_channel);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_DESTINATION:
    {
      pthread_mutex_lock(recall_mutex);
      
      g_value_set_object(value, recall_channel_run->destination);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  case PROP_SOURCE:
    {
      pthread_mutex_lock(recall_mutex);
      
      g_value_set_object(value, recall_channel_run->source);

      pthread_mutex_unlock(recall_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_recall_channel_run_dispose(GObject *gobject)
{
  AgsRecallChannelRun *recall_channel_run;

  recall_channel_run = AGS_RECALL_CHANNEL_RUN(gobject);
  
  /* recall audio run */
  if(recall_channel_run->recall_audio_run != NULL){
    g_object_unref(G_OBJECT(recall_channel_run->recall_audio_run));

    recall_channel_run->recall_audio_run = NULL;
  }
  
  /* recall channel */
  if(recall_channel_run->recall_channel != NULL){
    g_object_unref(G_OBJECT(recall_channel_run->recall_channel));

    recall_channel_run->recall_channel = NULL;
  }

  /* destination */
  if(recall_channel_run->destination != NULL){
    g_object_unref(recall_channel_run->destination);

    recall_channel_run->destination = NULL;
  }

  /* source */
  if(recall_channel_run->source != NULL){
    g_object_unref(recall_channel_run->source);

    recall_channel_run->source = NULL;
  }

  /* call parent */
  G_OBJECT_CLASS(ags_recall_channel_run_parent_class)->dispose(gobject);
}

void
ags_recall_channel_run_finalize(GObject *gobject)
{
  AgsRecallChannelRun *recall_channel_run;

  recall_channel_run = AGS_RECALL_CHANNEL_RUN(gobject);

  if(AGS_RECALL(gobject)->container != NULL){
    AgsRecallContainer *recall_container;

    recall_container = AGS_RECALL(gobject)->container;

    recall_container->recall_channel_run = g_list_remove(recall_container->recall_channel_run,
							 gobject);
    g_object_unref(AGS_RECALL(gobject)->container);
  }

  /* recall audio run */
  if(recall_channel_run->recall_audio_run != NULL){
    g_object_unref(G_OBJECT(recall_channel_run->recall_audio_run));
  }
  
  /* recall channel */
  if(recall_channel_run->recall_channel != NULL){
    g_object_unref(G_OBJECT(recall_channel_run->recall_channel));
  }

  /* destination */
  if(recall_channel_run->destination != NULL){
    g_object_unref(recall_channel_run->destination);
  }

  /* source */
  if(recall_channel_run->source != NULL){
    g_object_unref(recall_channel_run->source);
  }

  /* call parent */
  G_OBJECT_CLASS(ags_recall_channel_run_parent_class)->finalize(gobject);
}

void
ags_recall_channel_run_notify_recall_container_callback(GObject *gobject,
							GParamSpec *pspec,
							gpointer user_data)
{
  AgsChannel *source;
  AgsRecallContainer *recall_container;
  AgsRecallChannelRun *recall_channel_run;
  
  recall_channel_run = AGS_RECALL_CHANNEL_RUN(gobject);

  source = recall_channel_run->source;

  recall_container = AGS_RECALL(recall_channel_run)->recall_container;
  
  if(recall_container != NULL){
    AgsRecallAudio *recall_audio;
    AgsRecallID *recall_id;
    AgsRecyclingContext *recycling_context;
    
    GList *list_start, *list;

    guint recall_flags;

    recall_flags = AGS_RECALL(recall_channel_run)->flags;

    recall_id = AGS_RECALL(recall_channel_run)->recall_id;

    /* recall audio */
    g_object_get(recall_container,
		 "recall-audio", &recall_audio,
		 NULL);
    
    g_object_set(recall_channel_run,
		 "recall-audio", recall_audio,
		 NULL);
    
    /* recall audio run */
    g_object_get(recall_container,
		 "recall-audio-run", &list_start,
		 NULL);

    if(recall_id != NULL){
      recycling_context = recall_id->recycling_context;
      
      if((list = ags_recall_find_recycling_context(list_start,
						   (GObject *) recycling_context)) != NULL){
	g_object_set(recall_channel_run,
		     "recall-audio-run", list->data,
		     NULL);
      }
    }else if((AGS_RECALL_TEMPLATE & (recall_flags)) != 0){      
      if((list = ags_recall_find_template(list_start)) != NULL){	
	g_object_set(recall_channel_run,
		     "recall-audio-run", list->data,
		     NULL);
      }
    }

    g_list_free(list_start);

    /* recall channel */
    g_object_get(recall_container,
		 "recall-channel", &list_start,
		 NULL);
    
    if((list = ags_recall_find_provider(list_start,
					source)) != NULL){
      g_object_set(recall_channel_run,
		   "recall-channel", list->data,
		   NULL);
    }

    g_list_free(list_start);
  }else{
    g_object_set(recall_channel_run,
		 "recall-audio", NULL,
		 "recall-audio-run", NULL,
		 "recall-channel", NULL,
		 NULL);
  }
}

void
ags_recall_channel_run_connect(AgsConnectable *connectable)
{
  AgsRecallChannelRun *recall_channel_run;

  GObject *gobject;

  if(ags_connectable_is_connected(connectable)){
    return;
  }

  ags_recall_channel_run_parent_connectable_interface->connect(connectable);

  /* recall channel run */
  recall_channel_run = AGS_RECALL_CHANNEL_RUN(connectable);
  
  /* destination */
  if(recall_channel_run->destination != NULL){
    gobject = G_OBJECT(recall_channel_run->destination);
    
    /* recycling changed */
    g_signal_connect(gobject, "recycling-changed",
		     G_CALLBACK(ags_recall_channel_run_destination_recycling_changed_callback), recall_channel_run);
  }

  /* source */
  if(recall_channel_run->source != NULL){
    gobject = G_OBJECT(recall_channel_run->source);
    
    g_signal_connect(gobject, "recycling-changed",
		     G_CALLBACK(ags_recall_channel_run_source_recycling_changed_callback), recall_channel_run);
  }
}

void
ags_recall_channel_run_disconnect(AgsConnectable *connectable)
{
  AgsRecallChannelRun *recall_channel_run;

  GObject *gobject;

  if(!ags_connectable_is_connected(connectable)){
    return;
  }

  ags_recall_channel_run_parent_connectable_interface->disconnect(connectable);

  /* recall channel run */
  recall_channel_run = AGS_RECALL_CHANNEL_RUN(connectable);
  
  /* destination */
  if(recall_channel_run->destination != NULL){
    gobject = G_OBJECT(recall_channel_run->destination);

    g_object_disconnect(gobject,
			"any_signal::recycling-changed",
			G_CALLBACK(ags_recall_channel_run_destination_recycling_changed_callback),
			recall_channel_run,
			NULL);
  }

  /* source */
  if(recall_channel_run->source != NULL){
    gobject = G_OBJECT(recall_channel_run->source);

    g_object_disconnect(gobject,
			"any_signal::recycling-changed",
			G_CALLBACK(ags_recall_channel_run_source_recycling_changed_callback),
			recall_channel_run,
			NULL);
  }
}

void
ags_recall_channel_run_remove(AgsRecall *recall)
{
  if(AGS_RECALL_CHANNEL_RUN(recall)->source != NULL){
    ags_channel_remove_recall(AGS_RECALL_CHANNEL_RUN(recall)->source,
			      (GObject *) recall,
			      ((recall->recall_id->recycling_context->parent) ? TRUE: FALSE));
  }
 
  AGS_RECALL_CLASS(ags_recall_channel_run_parent_class)->remove(recall);
}

AgsRecall*
ags_recall_channel_run_duplicate(AgsRecall *recall,
				 AgsRecallID *recall_id,
				 guint *n_params, GParameter *parameter)
{
  AgsRecallChannelRun *recall_channel_run, *copy;

  recall_channel_run = AGS_RECALL_CHANNEL_RUN(recall);

  if(recall_channel_run->destination != NULL &&
     ags_recall_id_find_recycling_context(recall_channel_run->destination->recall_id,
					  recall_id->recycling_context->parent) == NULL){
    return(NULL);
  }
  
  parameter = ags_parameter_grow(G_OBJECT_TYPE(recall),
				 parameter, n_params,
				 "soundcard", AGS_RECALL(recall_channel_run)->soundcard,
				 "recall_channel", recall_channel_run->recall_channel,
				 "audio_channel", recall_channel_run->audio_channel,
				 "source", recall_channel_run->source,
				 "destination", recall_channel_run->destination,
				 NULL);
  copy = AGS_RECALL_CHANNEL_RUN(AGS_RECALL_CLASS(ags_recall_channel_run_parent_class)->duplicate(recall,
												 recall_id,
												 n_params, parameter));

  if(recall_channel_run->destination != NULL){
    ags_recall_channel_run_remap_child_destination(copy,
						   NULL, NULL,
						   copy->destination->first_recycling, copy->destination->last_recycling);
  }else{
    if(copy->source != NULL){
      ags_recall_channel_run_remap_child_source(copy,
						NULL, NULL,
						copy->source->first_recycling, copy->source->last_recycling);
    }
  }

  return((AgsRecall *) copy);
}

void
ags_recall_channel_run_map_recall_recycling(AgsRecallChannelRun *recall_channel_run)
{
  AgsRecallChannel *recall_channel;
  AgsRecycling *source_recycling;
  AgsRecycling *destination_recycling, *start_destination_recycling, *end_destination_recycling;

  if(recall_channel_run->source == NULL ||
     AGS_RECALL(recall_channel_run)->child_type == G_TYPE_NONE ||
     (AGS_RECALL_TEMPLATE & (AGS_RECALL(recall_channel_run)->flags)) != 0){
    return;
  }

  /* AgsRecallChannel */
  recall_channel = AGS_RECALL_CHANNEL(recall_channel_run->recall_channel);

  /* AgsRecycling - source */
  source_recycling = recall_channel->source->first_recycling;

  /* AgsRecycling - destination*/
  if(recall_channel_run->destination != NULL){
    start_destination_recycling = recall_channel_run->destination->first_recycling;
    end_destination_recycling = recall_channel_run->destination->last_recycling->next;
  }else{
    start_destination_recycling = NULL;
    end_destination_recycling = NULL;
  }

  if(source_recycling != NULL){
    AgsRecallRecycling *recall_recycling;

    while(source_recycling != recall_channel->source->last_recycling->next){
      destination_recycling = start_destination_recycling;

#ifdef AGS_DEBUG
      g_message("ags_recall_channel_run_map_recall_recycling");
#endif

      do{
	recall_recycling = g_object_new(AGS_RECALL(recall_channel_run)->child_type,
					"soundcard", AGS_RECALL(recall_channel_run)->soundcard,
					"recall_id", AGS_RECALL(recall_channel_run)->recall_id,
					"audio_channel", recall_channel_run->audio_channel,
					"source", source_recycling,
					"destination", destination_recycling,
					NULL);

	ags_recall_add_child(AGS_RECALL(recall_channel_run), AGS_RECALL(recall_recycling));
	
	if(destination_recycling != NULL){
	  destination_recycling = destination_recycling->next;
	}
      }while(destination_recycling != end_destination_recycling);

      source_recycling = source_recycling->next;
    }
  }
}

void
ags_recall_channel_run_remap_child_source(AgsRecallChannelRun *recall_channel_run,
					  AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
					  AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region)
{
  AgsRecycling *destination_recycling, *source_recycling;
  AgsRecallRecycling *recall_recycling;
  GList *list, *list_next, *list_start;

  if(recall_channel_run->source == NULL ||
     AGS_RECALL(recall_channel_run)->child_type == G_TYPE_NONE ||
     (AGS_RECALL_TEMPLATE & (AGS_RECALL(recall_channel_run)->flags)) != 0){
    return;
  }

  /* remove old */
  if(old_start_changed_region != NULL){
    AgsRecall *recall;
    AgsCancelRecall *cancel_recall;

    source_recycling = old_start_changed_region;

    while(source_recycling != old_end_changed_region->next){
      list_start = 
	list = ags_recall_get_children(AGS_RECALL(recall_channel_run));

      while(list != NULL){
	list_next = list->next;
	
	if(AGS_RECALL_RECYCLING(list->data)->source == source_recycling){
	  recall = AGS_RECALL(list->data);
	  //	  g_message("disconnect");
	  ags_dynamic_connectable_disconnect_dynamic(AGS_DYNAMIC_CONNECTABLE(recall));
  
	  ags_recall_remove(recall);
	  g_object_unref(recall);
	}

	list = list_next;
      }

      g_list_free(list_start);
      source_recycling = source_recycling->next;
    }
  }

  /* add new */
  if(new_start_changed_region != NULL){
    source_recycling = new_start_changed_region;
      
    while(source_recycling != new_end_changed_region->next){
      recall_recycling = g_object_new(AGS_RECALL(recall_channel_run)->child_type,
				      "soundcard", AGS_RECALL(recall_channel_run)->soundcard,
				      "recall_id", AGS_RECALL(recall_channel_run)->recall_id,
				      "audio_channel", recall_channel_run->audio_channel,
				      "source", source_recycling,
				      "destination", ((recall_channel_run->destination != NULL) ? recall_channel_run->destination->first_recycling: NULL),
				      NULL);
	
      ags_recall_add_child(AGS_RECALL(recall_channel_run), AGS_RECALL(recall_recycling));
      
      source_recycling = source_recycling->next;
    }
  }
}

void
ags_recall_channel_run_remap_child_destination(AgsRecallChannelRun *recall_channel_run,
					       AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
					       AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region)
{
  AgsRecycling *destination_recycling, *source_recycling;
  AgsRecallRecycling *recall_recycling;
  GList *list, *list_start;

  if(recall_channel_run->source == NULL ||
     AGS_RECALL(recall_channel_run)->child_type == G_TYPE_NONE ||
     (AGS_RECALL_TEMPLATE & (AGS_RECALL(recall_channel_run)->flags)) != 0){
    return;
  }

  /* remove old */
  if(old_start_changed_region != NULL){
    AgsRecall *recall;
    AgsCancelRecall *cancel_recall;

    destination_recycling = old_start_changed_region;
    
    while(destination_recycling != old_end_changed_region->next){
      list_start = 
	list = ags_recall_get_children(AGS_RECALL(recall_channel_run));

      while(list != NULL){
	if(AGS_RECALL_RECYCLING(list->data)->destination == destination_recycling){
	  recall = AGS_RECALL(list->data);

	  ags_recall_remove(recall);
	}

	list = list->next;
      }

      g_list_free(list_start);
      
      destination_recycling = destination_recycling->next;
    }
  }

  /* add new */
  if(new_start_changed_region != NULL){
    if(recall_channel_run->source != NULL){
      if(recall_channel_run->source->first_recycling == NULL){
	return;
      }
      
      destination_recycling = new_start_changed_region;
      
      while(destination_recycling != new_end_changed_region->next){
	source_recycling = recall_channel_run->source->first_recycling;
	
	while(source_recycling != recall_channel_run->source->last_recycling->next){
	  recall_recycling = g_object_new(AGS_RECALL(recall_channel_run)->child_type,
					  "soundcard", AGS_RECALL(recall_channel_run)->soundcard,
					  "recall_id", AGS_RECALL(recall_channel_run)->recall_id,
					  "audio_channel", recall_channel_run->audio_channel,
					  "source", source_recycling,
					  "destination", destination_recycling,
					  NULL);
	  
	  ags_recall_add_child(AGS_RECALL(recall_channel_run), AGS_RECALL(recall_recycling));

	  source_recycling = source_recycling->next;
	}
	
	destination_recycling = destination_recycling->next;
      }
    }
  }
}

void
ags_recall_channel_run_source_recycling_changed_callback(AgsChannel *channel,
							 AgsRecycling *old_start_region, AgsRecycling *old_end_region,
							 AgsRecycling *new_start_region, AgsRecycling *new_end_region,
							 AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
							 AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region,
							 AgsRecallChannelRun *recall_channel_run)
{  
  if(recall_channel_run->destination != NULL){
    ags_recall_channel_run_remap_child_source(recall_channel_run,
					      NULL, NULL,
					      NULL, NULL);
    ags_recall_channel_run_remap_child_destination(recall_channel_run,
						   NULL, NULL,
						   recall_channel_run->destination->first_recycling, recall_channel_run->destination->last_recycling);
  }else{
    ags_recall_channel_run_remap_child_source(recall_channel_run,
					      old_start_changed_region, old_end_changed_region,
					      new_start_changed_region, new_end_changed_region);
  }
}

void
ags_recall_channel_run_destination_recycling_changed_callback(AgsChannel *channel,
							      AgsRecycling *old_start_region, AgsRecycling *old_end_region,
							      AgsRecycling *new_start_region, AgsRecycling *new_end_region,
							      AgsRecycling *old_start_changed_region, AgsRecycling *old_end_changed_region,
							      AgsRecycling *new_start_changed_region, AgsRecycling *new_end_changed_region,
							      AgsRecallChannelRun *recall_channel_run)
{
  /* empty */
}

/**
 * ags_recall_channel_run_new:
 *
 * Creates an #AgsRecallChannelRun
 *
 * Returns: a new #AgsRecallChannelRun
 *
 * Since: 1.0.0
 */
AgsRecallChannelRun*
ags_recall_channel_run_new()
{
  AgsRecallChannelRun *recall_channel_run;

  recall_channel_run = (AgsRecallChannelRun *) g_object_new(AGS_TYPE_RECALL_CHANNEL_RUN, NULL);

  return(recall_channel_run);
}
