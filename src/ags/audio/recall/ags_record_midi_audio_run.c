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

#include <ags/audio/recall/ags_record_midi_audio_run.h>
#include <ags/audio/recall/ags_record_midi_audio.h>

#include <ags/util/ags_id_generator.h>

#include <ags/object/ags_config.h>
#include <ags/object/ags_connectable.h>
#include <ags/object/ags_dynamic_connectable.h>
#include <ags/object/ags_sequencer.h>
#include <ags/object/ags_plugin.h>

#include <ags/thread/ags_mutex_manager.h>

#include <ags/file/ags_file_stock.h>
#include <ags/file/ags_file_id_ref.h>
#include <ags/file/ags_file_lookup.h>

#include <ags/thread/ags_timestamp_thread.h>

#include <ags/audio/ags_recall_id.h>
#include <ags/audio/ags_recall_container.h>

#include <ags/audio/thread/ags_audio_loop.h>
#include <ags/audio/thread/ags_soundcard_thread.h>

#include <ags/audio/midi/ags_midi_parser.h>
#include <ags/audio/midi/ags_midi_file_writer.h>

void ags_record_midi_audio_run_class_init(AgsRecordMidiAudioRunClass *record_midi_audio_run);
void ags_record_midi_audio_run_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_record_midi_audio_run_dynamic_connectable_interface_init(AgsDynamicConnectableInterface *dynamic_connectable);
void ags_record_midi_audio_run_plugin_interface_init(AgsPluginInterface *plugin);
void ags_record_midi_audio_run_init(AgsRecordMidiAudioRun *record_midi_audio_run);
void ags_record_midi_audio_run_set_property(GObject *gobject,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *param_spec);
void ags_record_midi_audio_run_get_property(GObject *gobject,
					    guint prop_id,
					    GValue *value,
					    GParamSpec *param_spec);
void ags_record_midi_audio_run_finalize(GObject *gobject);
void ags_record_midi_audio_run_connect(AgsConnectable *connectable);
void ags_record_midi_audio_run_disconnect(AgsConnectable *connectable);
void ags_record_midi_audio_run_connect_dynamic(AgsDynamicConnectable *dynamic_connectable);
void ags_record_midi_audio_run_disconnect_dynamic(AgsDynamicConnectable *dynamic_connectable); 
void ags_record_midi_audio_run_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin);
xmlNode* ags_record_midi_audio_run_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin);

void ags_record_midi_audio_run_resolve_dependencies(AgsRecall *recall);
AgsRecall* ags_record_midi_audio_run_duplicate(AgsRecall *recall,
					       AgsRecallID *recall_id,
					       guint *n_params, GParameter *parameter);
void ags_record_midi_audio_run_run_init_pre(AgsRecall *recall);
void ags_record_midi_audio_run_run_pre(AgsRecall *recall);

void ags_record_midi_audio_run_write_resolve_dependency(AgsFileLookup *file_lookup,
							GObject *recall);
void ags_record_midi_audio_run_read_resolve_dependency(AgsFileLookup *file_lookup,
						       GObject *recall);

/**
 * SECTION:ags_record_midi_audio_run
 * @short_description: record midi
 * @title: AgsRecordMidiAudioRun
 * @section_id:
 * @include: ags/audio/recall/ags_record_midi_audio_run.h
 *
 * The #AgsRecordMidiAudioRun class record midi.
 */

enum{
  PROP_0,
  PROP_DELAY_AUDIO_RUN,
  PROP_COUNT_BEATS_AUDIO_RUN,
};

static gpointer ags_record_midi_audio_run_parent_class = NULL;
static AgsConnectableInterface* ags_record_midi_audio_run_parent_connectable_interface;
static AgsDynamicConnectableInterface *ags_record_midi_audio_run_parent_dynamic_connectable_interface;
static AgsPluginInterface *ags_record_midi_audio_run_parent_plugin_interface;

GType
ags_record_midi_audio_run_get_type()
{
  static GType ags_type_record_midi_audio_run = 0;

  if(!ags_type_record_midi_audio_run){
    static const GTypeInfo ags_record_midi_audio_run_info = {
      sizeof (AgsRecordMidiAudioRunClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_record_midi_audio_run_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsRecordMidiAudioRun),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_record_midi_audio_run_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_record_midi_audio_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_dynamic_connectable_interface_info = {
      (GInterfaceInitFunc) ags_record_midi_audio_run_dynamic_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_record_midi_audio_run_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_record_midi_audio_run = g_type_register_static(AGS_TYPE_RECALL_AUDIO_RUN,
							    "AgsRecordMidiAudioRun\0",
							    &ags_record_midi_audio_run_info,
							    0);

    g_type_add_interface_static(ags_type_record_midi_audio_run,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_record_midi_audio_run,
				AGS_TYPE_DYNAMIC_CONNECTABLE,
				&ags_dynamic_connectable_interface_info);

    g_type_add_interface_static(ags_type_record_midi_audio_run,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return (ags_type_record_midi_audio_run);
}

void
ags_record_midi_audio_run_class_init(AgsRecordMidiAudioRunClass *record_midi_audio_run)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  GParamSpec *param_spec;

  ags_record_midi_audio_run_parent_class = g_type_class_peek_parent(record_midi_audio_run);

  /* GObjectClass */
  gobject = (GObjectClass *) record_midi_audio_run;

  gobject->set_property = ags_record_midi_audio_run_set_property;
  gobject->get_property = ags_record_midi_audio_run_get_property;

  gobject->finalize = ags_record_midi_audio_run_finalize;

  /* properties */
  param_spec = g_param_spec_object("delay-audio-run\0",
				   "assigned AgsDelayAudioRun\0",
				   "the AgsDelayAudioRun which emits midi_alloc_input signal\0",
				   AGS_TYPE_DELAY_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DELAY_AUDIO_RUN,
				  param_spec);

  param_spec = g_param_spec_object("count-beats-audio-run\0",
				   "assigned AgsCountBeatsAudioRun\0",
				   "the AgsCountBeatsAudioRun which just counts\0",
				   AGS_TYPE_COUNT_BEATS_AUDIO_RUN,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_COUNT_BEATS_AUDIO_RUN,
				  param_spec);

  /* AgsRecallClass */
  recall = (AgsRecallClass *) record_midi_audio_run;

  recall->resolve_dependencies = ags_record_midi_audio_run_resolve_dependencies;
  recall->duplicate = ags_record_midi_audio_run_duplicate;
  recall->run_init_pre = ags_record_midi_audio_run_run_init_pre;
  recall->run_pre = ags_record_midi_audio_run_run_pre;
}

void
ags_record_midi_audio_run_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_record_midi_audio_run_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_record_midi_audio_run_connect;
  connectable->disconnect = ags_record_midi_audio_run_disconnect;
}

void
ags_record_midi_audio_run_dynamic_connectable_interface_init(AgsDynamicConnectableInterface *dynamic_connectable)
{
  ags_record_midi_audio_run_parent_dynamic_connectable_interface = g_type_interface_peek_parent(dynamic_connectable);

  dynamic_connectable->connect_dynamic = ags_record_midi_audio_run_connect_dynamic;
  dynamic_connectable->disconnect_dynamic = ags_record_midi_audio_run_disconnect_dynamic;
}

void
ags_record_midi_audio_run_plugin_interface_init(AgsPluginInterface *plugin)
{
  ags_record_midi_audio_run_parent_plugin_interface = g_type_interface_peek_parent(plugin);
  
  plugin->read = ags_record_midi_audio_run_read;
  plugin->write = ags_record_midi_audio_run_write;
}

void
ags_record_midi_audio_run_init(AgsRecordMidiAudioRun *record_midi_audio_run)
{
  AGS_RECALL(record_midi_audio_run)->name = "ags-record-midi\0";
  AGS_RECALL(record_midi_audio_run)->version = AGS_RECALL_DEFAULT_VERSION;
  AGS_RECALL(record_midi_audio_run)->build_id = AGS_RECALL_DEFAULT_BUILD_ID;
  AGS_RECALL(record_midi_audio_run)->xml_type = "ags-record-midi-audio-run\0";
  AGS_RECALL(record_midi_audio_run)->port = NULL;

  record_midi_audio_run->delay_audio_run = NULL;
  record_midi_audio_run->count_beats_audio_run = NULL;

  record_midi_audio_run->midi_file_writer = ags_midi_file_writer_new(NULL);
}

void
ags_record_midi_audio_run_set_property(GObject *gobject,
				       guint prop_id,
				       const GValue *value,
				       GParamSpec *param_spec)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(gobject);

  switch(prop_id){
  case PROP_DELAY_AUDIO_RUN:
    {
      AgsDelayAudioRun *delay_audio_run;
      gboolean is_template;

      delay_audio_run = g_value_get_object(value);

      if(delay_audio_run == record_midi_audio_run->delay_audio_run){
	return;
      }

      if(delay_audio_run != NULL &&
	 (AGS_RECALL_TEMPLATE & (AGS_RECALL(delay_audio_run)->flags)) != 0){
	is_template = TRUE;
      }else{
	is_template = FALSE;
      }

      if(record_midi_audio_run->delay_audio_run != NULL){
	if(is_template){
	  ags_recall_remove_dependency(AGS_RECALL(record_midi_audio_run),
				       (AgsRecall *) record_midi_audio_run->delay_audio_run);
	}
	
	g_object_unref(G_OBJECT(record_midi_audio_run->delay_audio_run));
      }

      if(delay_audio_run != NULL){
	g_object_ref(delay_audio_run);

	if(is_template){
	  ags_recall_add_dependency(AGS_RECALL(record_midi_audio_run),
				    ags_recall_dependency_new((GObject *) delay_audio_run));
	}
      }

      record_midi_audio_run->delay_audio_run = delay_audio_run;
    }
    break;
  case PROP_COUNT_BEATS_AUDIO_RUN:
    {
      AgsCountBeatsAudioRun *count_beats_audio_run;
      gboolean is_template;

      count_beats_audio_run = g_value_get_object(value);

      if(count_beats_audio_run == record_midi_audio_run->count_beats_audio_run){
	return;
      }

      if(count_beats_audio_run != NULL &&
	 (AGS_RECALL_TEMPLATE & (AGS_RECALL(count_beats_audio_run)->flags)) != 0){
	is_template = TRUE;
      }else{
	is_template = FALSE;
      }

      if(record_midi_audio_run->count_beats_audio_run != NULL){
	if(is_template){
	  ags_recall_remove_dependency(AGS_RECALL(record_midi_audio_run),
				       (AgsRecall *) record_midi_audio_run->count_beats_audio_run);
	}

	g_object_unref(G_OBJECT(record_midi_audio_run->count_beats_audio_run));
      }

      if(count_beats_audio_run != NULL){
	g_object_ref(count_beats_audio_run);

	if(is_template){
	  ags_recall_add_dependency(AGS_RECALL(record_midi_audio_run),
				    ags_recall_dependency_new((GObject *) count_beats_audio_run));
	}
      }

      record_midi_audio_run->count_beats_audio_run = count_beats_audio_run;
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_record_midi_audio_run_get_property(GObject *gobject,
				       guint prop_id,
				       GValue *value,
				       GParamSpec *param_spec)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;
  
  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(gobject);

  switch(prop_id){
  case PROP_DELAY_AUDIO_RUN:
    {
      g_value_set_object(value, G_OBJECT(record_midi_audio_run->delay_audio_run));
    }
    break;
  case PROP_COUNT_BEATS_AUDIO_RUN:
    {
      g_value_set_object(value, G_OBJECT(record_midi_audio_run->count_beats_audio_run));
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  };
}

void
ags_record_midi_audio_run_finalize(GObject *gobject)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(gobject);

  if(record_midi_audio_run->delay_audio_run != NULL){
    g_object_unref(G_OBJECT(record_midi_audio_run->delay_audio_run));
  }

  if(record_midi_audio_run->count_beats_audio_run != NULL){
    g_object_unref(G_OBJECT(record_midi_audio_run->count_beats_audio_run));
  }

  G_OBJECT_CLASS(ags_record_midi_audio_run_parent_class)->finalize(gobject);
}

void
ags_record_midi_audio_run_connect(AgsConnectable *connectable)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  if((AGS_RECALL_CONNECTED & (AGS_RECALL(connectable)->flags)) != 0){
    return;
  }

  /* call parent */
  ags_record_midi_audio_run_parent_connectable_interface->connect(connectable);

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(connectable);
}

void
ags_record_midi_audio_run_disconnect(AgsConnectable *connectable)
{
  /* call parent */
  ags_record_midi_audio_run_parent_connectable_interface->disconnect(connectable);
}

void
ags_record_midi_audio_run_connect_dynamic(AgsDynamicConnectable *dynamic_connectable)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  if((AGS_RECALL_DYNAMIC_CONNECTED & (AGS_RECALL(dynamic_connectable)->flags)) != 0){
    return;
  }

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(dynamic_connectable);

  /* call parent */
  ags_record_midi_audio_run_parent_dynamic_connectable_interface->connect_dynamic(dynamic_connectable);  
}

void
ags_record_midi_audio_run_disconnect_dynamic(AgsDynamicConnectable *dynamic_connectable)
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  /* call parent */
  ags_record_midi_audio_run_parent_dynamic_connectable_interface->disconnect_dynamic(dynamic_connectable);

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(dynamic_connectable);
}

void
ags_record_midi_audio_run_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin)
{
  AgsFileLookup *file_lookup;
  xmlNode *iter;

  /* read parent */
  ags_record_midi_audio_run_parent_plugin_interface->read(file, node, plugin);

  /* read depenendency */
  iter = node->children;

  while(iter != NULL){
    if(iter->type == XML_ELEMENT_NODE){
      if(!xmlStrncmp(iter->name,
		     "ags-dependency-list\0",
		     19)){
	xmlNode *dependency_node;

	dependency_node = iter->children;

	while(dependency_node != NULL){
	  if(dependency_node->type == XML_ELEMENT_NODE){
	    if(!xmlStrncmp(dependency_node->name,
			   "ags-dependency\0",
			   15)){
	      file_lookup = (AgsFileLookup *) g_object_new(AGS_TYPE_FILE_LOOKUP,
							   "file\0", file,
							   "node\0", dependency_node,
							   "reference\0", G_OBJECT(plugin),
							   NULL);
	      ags_file_add_lookup(file, (GObject *) file_lookup);
	      g_signal_connect(G_OBJECT(file_lookup), "resolve\0",
			       G_CALLBACK(ags_record_midi_audio_run_read_resolve_dependency), G_OBJECT(plugin));
	    }
	  }
	  
	  dependency_node = dependency_node->next;
	}
      }
    }

    iter = iter->next;
  }
}

xmlNode*
ags_record_midi_audio_run_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin)
{
  AgsFileLookup *file_lookup;
  xmlNode *node, *child;
  xmlNode *dependency_node;
  GList *list;
  gchar *id;

  /* write parent */
  node = ags_record_midi_audio_run_parent_plugin_interface->write(file, parent, plugin);

  /* write dependencies */
  child = xmlNewNode(NULL,
		     "ags-dependency-list\0");

  xmlNewProp(child,
	     AGS_FILE_ID_PROP,
	     ags_id_generator_create_uuid());

  xmlAddChild(node,
	      child);

  list = AGS_RECALL(plugin)->dependencies;

  while(list != NULL){
    id = ags_id_generator_create_uuid();

    dependency_node = xmlNewNode(NULL,
				 "ags-dependency\0");

    xmlNewProp(dependency_node,
	       AGS_FILE_ID_PROP,
	       id);

    xmlAddChild(child,
		dependency_node);

    file_lookup = (AgsFileLookup *) g_object_new(AGS_TYPE_FILE_LOOKUP,
						 "file\0", file,
						 "node\0", dependency_node,
						 "reference\0", list->data,
						 NULL);
    ags_file_add_lookup(file, (GObject *) file_lookup);
    g_signal_connect(G_OBJECT(file_lookup), "resolve\0",
		     G_CALLBACK(ags_record_midi_audio_run_write_resolve_dependency), G_OBJECT(plugin));

    list = list->next;
  }

  return(node);
}

void
ags_record_midi_audio_run_resolve_dependencies(AgsRecall *recall)
{
  AgsRecall *template;
  AgsRecallContainer *recall_container;
  AgsRecordMidiAudioRun *record_midi_audio_run;
  AgsRecallDependency *recall_dependency;
  AgsDelayAudioRun *delay_audio_run;
  AgsCountBeatsAudioRun *count_beats_audio_run;
  GList *list;
  AgsRecallID *recall_id;
  guint i, i_stop;

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(recall);
  
  recall_container = AGS_RECALL_CONTAINER(recall->container);
  
  list = ags_recall_find_template(recall_container->recall_audio_run);

  if(list != NULL){
    template = AGS_RECALL(list->data);
  }else{
    g_warning("AgsRecallClass::resolve - missing dependency");
    return;
  }

  list = template->dependencies;
  delay_audio_run = NULL;
  count_beats_audio_run = NULL;
  i_stop = 2;

  for(i = 0; i < i_stop && list != NULL;){
    recall_dependency = AGS_RECALL_DEPENDENCY(list->data);

    if(AGS_IS_DELAY_AUDIO_RUN(recall_dependency->dependency)){
      if(((AGS_RECALL_INPUT_ORIENTATED & (recall->flags)) != 0 &&
	  (AGS_RECALL_INPUT_ORIENTATED & (AGS_RECALL(recall_dependency->dependency)->flags)) != 0) ||
	 ((AGS_RECALL_OUTPUT_ORIENTATED & (recall->flags)) != 0 &&
	  (AGS_RECALL_OUTPUT_ORIENTATED & (AGS_RECALL(recall_dependency->dependency)->flags)) != 0)){
	recall_id = recall->recall_id;
      }else{
	recall_id = (AgsRecallID *) recall->recall_id->recycling_context->parent->recall_id;
      }

      delay_audio_run = (AgsDelayAudioRun *) ags_recall_dependency_resolve(recall_dependency, recall_id);

      i++;
    }else if(AGS_IS_COUNT_BEATS_AUDIO_RUN(recall_dependency->dependency)){
      if(((AGS_RECALL_INPUT_ORIENTATED & (recall->flags)) != 0 &&
	  (AGS_RECALL_INPUT_ORIENTATED & (AGS_RECALL(recall_dependency->dependency)->flags)) != 0) ||
	 ((AGS_RECALL_OUTPUT_ORIENTATED & (recall->flags)) != 0 &&
	  (AGS_RECALL_OUTPUT_ORIENTATED & (AGS_RECALL(recall_dependency->dependency)->flags)) != 0)){
	recall_id = recall->recall_id;
      }else{
	recall_id = (AgsRecallID *) recall->recall_id->recycling_context->parent->recall_id;
      }

      count_beats_audio_run = (AgsCountBeatsAudioRun *) ags_recall_dependency_resolve(recall_dependency, recall_id);

      i++;
    }

    list = list->next;
  }

  g_object_set(G_OBJECT(recall),
	       "delay-audio-run\0", delay_audio_run,
	       "count-beats-audio-run\0", count_beats_audio_run,
	       NULL);
}

AgsRecall*
ags_record_midi_audio_run_duplicate(AgsRecall *recall,
				    AgsRecallID *recall_id,
				    guint *n_params, GParameter *parameter)
{
  AgsRecordMidiAudioRun *copy, *record_midi_audio_run;

  copy = AGS_RECORD_MIDI_AUDIO_RUN(AGS_RECALL_CLASS(ags_record_midi_audio_run_parent_class)->duplicate(recall,
												       recall_id,
												       n_params, parameter));

  return((AgsRecall *) copy);
}

void
ags_record_midi_audio_run_run_init_pre(AgsRecall *recall)
{
  AgsAudio *audio;
  AgsRecordMidiAudio *record_midi_audio;
  AgsRecordMidiAudioRun *record_midi_audio_run;
  
  AgsSequencer *sequencer;
  
  gboolean playback, record;

  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(recall);
  record_midi_audio = AGS_RECORD_MIDI_AUDIO(recall);

  audio = AGS_RECALL_AUDIO(record_midi_audio)->audio;
  sequencer = audio->sequencer;

  if(sequencer == NULL){
    return;
  }

  /* midi parser */
  if(record_midi_audio_run->midi_parser != NULL){
    g_object_unref(record_midi_audio_run->midi_parser);
  }

  record_midi_audio_run->midi_parser = ags_midi_parser_new(NULL);
  
  /* midi file writer */
  if(record_midi_audio_run->midi_file_writer != NULL){
    g_object_unref(record_midi_audio_run->midi_file_writer);
  }
  
  record_midi_audio_run->midi_file_writer = ags_midi_file_writer_new(NULL);
  
  AGS_RECALL_CLASS(ags_record_midi_audio_run_parent_class)->run_init_pre(recall);
}

void
ags_record_midi_audio_run_run_pre(AgsRecall *recall)
{
  AgsAudio *audio;
  
  AgsRecordMidiAudio *record_midi_audio;
  AgsRecordMidiAudioRun *record_midi_audio_run;
  AgsDelayAudioRun *delay_audio_run;
  AgsDelayAudioRun *count_beats_audio_run;

  AgsSequencer *sequencer;

  GList *feed_midi;
  
  unsigned char *midi_buffer;
  gboolean playback, record;
  guint audio_channel;
  guint buffer_length;
  
  GValue value = {0,};

  auto GList* ags_record_midi_audio_run_parse_node(xmlNode *node);

  GList* ags_record_midi_audio_run_parse_node(xmlNode *node){
    AgsNote *note;
    
    xmlNode *child;
    
    GList *list, *child_list;
    
    guint x, y;
    guint velocity, pressure;
    
    list = NULL;
    child_list = NULL;

    if(!xmlStrncmp(xmlGetProp(child,
			      "event\0"),
		   "note-on\0",
		   8)){
      x = count_beats_audio_run->notation_counter;
      y = (guint) g_ascii_strtoull(xmlGetProp(child,
					      "note\0"),
				   NULL,
				   10);
      velocity = (guint) g_ascii_strtoull(xmlGetProp(child,
						     "velocity\0"),
					  NULL,
					  10);
	  
      note = ags_note_new();
      note->x[0] = x;
      note->x[1] = x + 64;
      note->y = y;
      //TODO:JK: enhance me
      ags_complex_set(note->attack,
		      velocity);

      list = g_list_prepend(list,
			    note);
    }else if(!xmlStrncmp(xmlGetProp(child,
				    "event\0"),
			 "note-off\0",
			 9)){	  
      x = count_beats_audio_run->notation_counter;
      y = (guint) g_ascii_strtoull(xmlGetProp(child,
					      "note\0"),
				   NULL,
				   10);
      velocity = (guint) g_ascii_strtoull(xmlGetProp(child,
						     "velocity\0"),
					  NULL,
					  10);
	  

      note = ags_note_find_prev(list,
				x, y);
	    
      note->x[1] = x;
      note->y = y;
      //TODO:JK: enhance me
      ags_complex_set(note->release,
		      velocity);
    }else if(!xmlStrncmp(xmlGetProp(child,
				    "event\0"),
			 "polyphonic\0",
			 9)){	  
      x = count_beats_audio_run->notation_counter;
      y = (guint) g_ascii_strtoull(xmlGetProp(child,
					      "note\0"),
				   NULL,
				   10);
      pressure = (guint) g_ascii_strtoull(xmlGetProp(child,
						     "pressure\0"),
					  NULL,
					  10);
      
      note = ags_note_find_prev(list,
				x, y);
	    
      note->x[1] = x + 64;
      note->y = y;
      //TODO:JK: enhance me
      ags_complex_set(note->sustain,
		      pressure);
    }

    /* parse children */
    child = node->children;

    while(child != NULL){
      if(child->type == XML_ELEMENT_NODE){
	child_list = ags_record_midi_audio_run_parse_node(child);

	if(child_list != NULL){
	  if(list != NULL){
	    list = g_list_concat(list,
				 child_list);
	  }else{
	    list = child_list;
	  }
	}
      }

      child = child->next;
    }

    return(list);
  }
  
  record_midi_audio_run = AGS_RECORD_MIDI_AUDIO_RUN(recall);
  record_midi_audio = AGS_RECORD_MIDI_AUDIO(recall);

  delay_audio_run = record_midi_audio_run->delay_audio_run;
  count_beats_audio_run = record_midi_audio_run->count_beats_audio_run;
  
  audio = AGS_RECALL_AUDIO(record_midi_audio)->audio;
  sequencer = audio->sequencer;

  if(sequencer == NULL){
    return;
  }
  
  audio_channel = AGS_CHANNEL(AGS_RECYCLING(recall->recall_id->recycling)->channel)->audio_channel;
  
  /* get mode */
  g_value_init(&value,
	       G_TYPE_BOOLEAN);
  ags_port_safe_read(record_midi_audio->playback,
		     &value);

  playback = g_value_get_boolean(&value);

  g_value_reset(&value);
  ags_port_safe_read(record_midi_audio->record,
		     &value);

  record = g_value_get_boolean(&value);

  /* retrieve buffer */
  midi_buffer = ags_sequencer_get_buffer(AGS_SEQUENCER(sequencer),
					 &buffer_length);
  
  /* playback */
  if(midi_buffer != NULL){
    if(playback){
      xmlNode *node;

      GList *notation;
      GList *note;
      
      gdouble delay;
      gdouble attack;

      /* parse sequencer data */
      node = ags_midi_parser_parse_bytes(record_midi_audio_run->midi_parser,
					 midi_buffer,
					 buffer_length);
      
      xmlFreeNode(node);

      /* feed midi */
      delay = (gdouble) delay_audio_run->notation_counter;
      attack = 0;

      note = ags_record_midi_audio_run_parse_node(node);

      while(note != NULL){
	notation = audio->notation;

	while(notation != NULL){
	  ags_notation_add_note(notation->data,
				ags_note_duplicate(note->data),
				FALSE);

	  notation = notation->next;
	}
	
	note = note->next;
      }
    }

    /* record */
    if(record){
      ags_midi_file_writer_write_bytes(record_midi_audio_run->midi_file_writer,
				       midi_buffer,
				       buffer_length);
    }
  }

  AGS_RECALL_CLASS(ags_record_midi_audio_run_parent_class)->run_pre(recall);
}

void
ags_record_midi_audio_run_write_resolve_dependency(AgsFileLookup *file_lookup,
						   GObject *recall)
{
  AgsFileIdRef *id_ref;
  gchar *id;

  id_ref = (AgsFileIdRef *) ags_file_find_id_ref_by_reference(file_lookup->file,
							      AGS_RECALL_DEPENDENCY(file_lookup->ref)->dependency);

  id = xmlGetProp(id_ref->node, AGS_FILE_ID_PROP);

  xmlNewProp(file_lookup->node,
	     "xpath\0",
  	     g_strdup_printf("xpath=//*[@id='%s']\0", id));
}

void
ags_record_midi_audio_run_read_resolve_dependency(AgsFileLookup *file_lookup,
						  GObject *recall)
{
  AgsFileIdRef *id_ref;
  gchar *xpath;

  xpath = (gchar *) xmlGetProp(file_lookup->node,
			       "xpath\0");

  id_ref = (AgsFileIdRef *) ags_file_find_id_ref_by_xpath(file_lookup->file, xpath);

  if(AGS_IS_DELAY_AUDIO_RUN(id_ref->ref)){
    g_object_set(G_OBJECT(recall),
		 "delay-audio-run\0", id_ref->ref,
		 NULL);
  }else if(AGS_IS_COUNT_BEATS_AUDIO_RUN(id_ref->ref)){
    g_object_set(G_OBJECT(recall),
		 "count-beats-audio-run\0", id_ref->ref,
		 NULL);
  }
}

/**
 * ags_record_midi_audio_run_new:
 *
 * Creates an #AgsRecordMidiAudioRun
 *
 * Returns: a new #AgsRecordMidiAudioRun
 *
 * Since: 0.7.0
 */
AgsRecordMidiAudioRun*
ags_record_midi_audio_run_new()
{
  AgsRecordMidiAudioRun *record_midi_audio_run;

  record_midi_audio_run = (AgsRecordMidiAudioRun *) g_object_new(AGS_TYPE_RECORD_MIDI_AUDIO_RUN,
								 NULL);

  return(record_midi_audio_run);
}
