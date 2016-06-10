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

#include <ags/X/machine/ags_lv2_bridge.h>
#include <ags/X/machine/ags_lv2_bridge_callbacks.h>

#include <ags/util/ags_id_generator.h>

#include <ags/lib/ags_string_util.h>

#include <ags/object/ags_application_context.h>
#include <ags/object/ags_marshal.h>
#include <ags/object/ags_connectable.h>
#include <ags/object/ags_config.h>
#include <ags/object/ags_soundcard.h>
#include <ags/object/ags_plugin.h>
#include <ags/object/ags_seekable.h>

#include <ags/file/ags_file.h>
#include <ags/file/ags_file_stock.h>
#include <ags/file/ags_file_id_ref.h>
#include <ags/file/ags_file_launch.h>

#include <ags/thread/ags_mutex_manager.h>
#include <ags/thread/ags_thread-posix.h>

#include <ags/plugin/ags_lv2_manager.h>
#include <ags/plugin/ags_lv2ui_manager.h>
#include <ags/plugin/ags_lv2_plugin.h>

#include <ags/audio/ags_input.h>
#include <ags/audio/ags_recall_factory.h>
#include <ags/audio/ags_recall.h>
#include <ags/audio/ags_recall_lv2.h>
#include <ags/audio/ags_recall_container.h>

#include <ags/audio/recall/ags_delay_audio.h>
#include <ags/audio/recall/ags_delay_audio_run.h>
#include <ags/audio/recall/ags_count_beats_audio.h>
#include <ags/audio/recall/ags_count_beats_audio_run.h>
#include <ags/audio/recall/ags_play_notation_audio.h>
#include <ags/audio/recall/ags_play_notation_audio_run.h>
#include <ags/audio/recall/ags_route_lv2_audio.h>
#include <ags/audio/recall/ags_route_lv2_audio_run.h>

#include <ags/widget/ags_dial.h>

#include <ags/X/ags_window.h>
#include <ags/X/ags_effect_bridge.h>
#include <ags/X/ags_effect_bulk.h>
#include <ags/X/ags_bulk_member.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void ags_lv2_bridge_class_init(AgsLv2BridgeClass *lv2_bridge);
void ags_lv2_bridge_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_lv2_bridge_plugin_interface_init(AgsPluginInterface *plugin);
void ags_lv2_bridge_init(AgsLv2Bridge *lv2_bridge);
void ags_lv2_bridge_set_property(GObject *gobject,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *param_spec);
void ags_lv2_bridge_get_property(GObject *gobject,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *param_spec);
void ags_lv2_bridge_connect(AgsConnectable *connectable);
void ags_lv2_bridge_disconnect(AgsConnectable *connectable);
gchar* ags_lv2_bridge_get_version(AgsPlugin *plugin);
void ags_lv2_bridge_set_version(AgsPlugin *plugin, gchar *version);
gchar* ags_lv2_bridge_get_build_id(AgsPlugin *plugin);
void ags_lv2_bridge_set_build_id(AgsPlugin *plugin, gchar *build_id);
gchar* ags_lv2_bridge_get_xml_type(AgsPlugin *plugin);
void ags_lv2_bridge_set_xml_type(AgsPlugin *plugin, gchar *xml_type);
void ags_lv2_bridge_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin);
void ags_lv2_bridge_launch_task(AgsFileLaunch *file_launch, AgsLv2Bridge *lv2_bridge);
xmlNode* ags_lv2_bridge_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin);

void ags_lv2_bridge_set_audio_channels(AgsAudio *audio,
				       guint audio_channels, guint audio_channels_old,
				       gpointer data);
void ags_lv2_bridge_set_pads(AgsAudio *audio, GType type,
			     guint pads, guint pads_old,
			     gpointer data);

void ags_lv2_bridge_map_recall(AgsMachine *machine);

/**
 * SECTION:ags_lv2_bridge
 * @short_description: A composite widget to visualize a bunch of #AgsChannel
 * @title: AgsLv2Bridge
 * @section_id:
 * @include: ags/X/ags_lv2_bridge.h
 *
 * #AgsLv2Bridge is a composite widget to visualize all #AgsChannel. It should be
 * packed by an #AgsMachine.
 */

enum{
  PROP_0,
  PROP_FILENAME,
  PROP_EFFECT,
  PROP_URI,
  PROP_INDEX,
  PROP_HAS_MIDI,
  PROP_HAS_GUI,
  PROP_GUI_FILENAME,
  PROP_GUI_URI,
};

static gpointer ags_lv2_bridge_parent_class = NULL;
static AgsConnectableInterface* ags_lv2_bridge_parent_connectable_interface;
static AgsPluginInterface* ags_lv2_bridge_parent_plugin_interface;

GType
ags_lv2_bridge_get_type(void)
{
  static GType ags_type_lv2_bridge = 0;

  if(!ags_type_lv2_bridge){
    static const GTypeInfo ags_lv2_bridge_info = {
      sizeof(AgsLv2BridgeClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_lv2_bridge_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsLv2Bridge),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_lv2_bridge_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_lv2_bridge_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_lv2_bridge_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_lv2_bridge = g_type_register_static(AGS_TYPE_MACHINE,
						 "AgsLv2Bridge\0", &ags_lv2_bridge_info,
						 0);

    g_type_add_interface_static(ags_type_lv2_bridge,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_lv2_bridge,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return(ags_type_lv2_bridge);
}

void
ags_lv2_bridge_class_init(AgsLv2BridgeClass *lv2_bridge)
{
  AgsMachineClass *machine;
  GObjectClass *gobject;
  GParamSpec *param_spec;

  ags_lv2_bridge_parent_class = g_type_class_peek_parent(lv2_bridge);

  /* GObjectClass */
  gobject = G_OBJECT_CLASS(lv2_bridge);

  gobject->set_property = ags_lv2_bridge_set_property;
  gobject->get_property = ags_lv2_bridge_get_property;

  /* properties */
  /**
   * AgsRecallLv2:filename:
   *
   * The plugin's filename.
   * 
   * Since: 0.4.3
   */
  param_spec =  g_param_spec_string("filename\0",
				    "the object file\0",
				    "The filename as string of object file\0",
				    NULL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_FILENAME,
				  param_spec);
  
  /**
   * AgsRecallLv2:effect:
   *
   * The effect's name.
   * 
   * Since: 0.4.3
   */
  param_spec =  g_param_spec_string("effect\0",
				    "the effect\0",
				    "The effect's string representation\0",
				    NULL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_EFFECT,
				  param_spec);

  /**
   * AgsRecallLv2:uri:
   *
   * The uri's name.
   * 
   * Since: 0.4.3
   */
  param_spec =  g_param_spec_string("uri\0",
				    "the uri\0",
				    "The uri's string representation\0",
				    NULL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_URI,
				  param_spec);

  /**
   * AgsRecallLv2:index:
   *
   * The uri's index.
   * 
   * Since: 0.4.3
   */
  param_spec =  g_param_spec_ulong("index\0",
				   "index of uri\0",
				   "The numerical index of uri\0",
				   0,
				   65535,
				   0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_INDEX,
				  param_spec);

  /**
   * AgsRecallLv2:has-midi:
   *
   * If has-midi is set to %TRUE appropriate flag is set
   * to audio in order to become a sequencer.
   * 
   * Since: 0.7.6
   */
  param_spec =  g_param_spec_boolean("has-midi\0",
				     "has-midi\0",
				     "If effect has-midi\0",
				     FALSE,
				     G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_HAS_MIDI,
				  param_spec);

  /**
   * AgsRecallLv2:has-gui:
   *
   * If has-gui is set to %TRUE 128 inputs are allocated and appropriate flag is set
   * to audio in order to become a sequencer.
   * 
   * Since: 0.7.6
   */
  param_spec =  g_param_spec_boolean("has-gui\0",
				     "has-gui\0",
				     "If effect has-gui\0",
				     FALSE,
				     G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_HAS_GUI,
				  param_spec);

  /**
   * AgsRecallLv2:gui-filename:
   *
   * The plugin's GUI filename.
   * 
   * Since: 0.7.6
   */
  param_spec =  g_param_spec_string("gui-filename\0",
				    "the GUI object file\0",
				    "The filename as string of GUI object file\0",
				    NULL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_GUI_FILENAME,
				  param_spec);

  /**
   * AgsRecallLv2:gui-uri:
   *
   * The GUI's uri name.
   * 
   * Since: 0.4.3
   */
  param_spec =  g_param_spec_string("gui-uri\0",
				    "the gui-uri\0",
				    "The gui-uri's string representation\0",
				    NULL,
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_GUI_URI,
				  param_spec);

  /* AgsMachine */
  machine = (AgsMachineClass *) lv2_bridge;

  machine->map_recall = ags_lv2_bridge_map_recall;
}

void
ags_lv2_bridge_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_lv2_bridge_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_lv2_bridge_connect;
  connectable->disconnect = ags_lv2_bridge_disconnect;
}

void
ags_lv2_bridge_plugin_interface_init(AgsPluginInterface *plugin)
{
  plugin->get_name = NULL;
  plugin->set_name = NULL;
  plugin->get_version = ags_lv2_bridge_get_version;
  plugin->set_version = ags_lv2_bridge_set_version;
  plugin->get_build_id = ags_lv2_bridge_get_build_id;
  plugin->set_build_id = ags_lv2_bridge_set_build_id;
  plugin->get_xml_type = NULL;
  plugin->set_xml_type = NULL;
  plugin->get_ports = NULL;
  plugin->read = NULL;
  plugin->write = NULL;
  plugin->set_ports = NULL;
}

void
ags_lv2_bridge_init(AgsLv2Bridge *lv2_bridge)
{
  GtkTable *table;
  GtkImageMenuItem *item;

  AgsAudio *audio;

  audio = AGS_MACHINE(lv2_bridge)->audio;
  audio->flags |= (AGS_AUDIO_SYNC);

  g_signal_connect_after(G_OBJECT(audio), "set_audio_channels\0",
			 G_CALLBACK(ags_lv2_bridge_set_audio_channels), NULL);
  
  g_signal_connect_after(G_OBJECT(audio), "set_pads\0",
			 G_CALLBACK(ags_lv2_bridge_set_pads), NULL);
  
  lv2_bridge->flags = 0;

  lv2_bridge->name = NULL;

  lv2_bridge->version = AGS_LV2_BRIDGE_DEFAULT_VERSION;
  lv2_bridge->build_id = AGS_LV2_BRIDGE_DEFAULT_BUILD_ID;

  lv2_bridge->xml_type = "ags-lv2-bridge\0";
  
  lv2_bridge->mapped_output_pad = 0;
  lv2_bridge->mapped_input_pad = 0;
  
  lv2_bridge->filename = NULL;
  lv2_bridge->effect = NULL;
  lv2_bridge->uri = NULL;
  lv2_bridge->uri_index = 0;

  lv2_bridge->has_midi = FALSE;

  AGS_MACHINE(lv2_bridge)->bridge = ags_effect_bridge_new(audio);
  gtk_container_add(gtk_bin_get_child(lv2_bridge),
		    (GtkWidget *) AGS_MACHINE(lv2_bridge)->bridge);

  table = (GtkTable *) gtk_table_new(1, 2, FALSE);
  gtk_box_pack_start(AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge),
		     table,
		     FALSE, FALSE,
		     0);

  AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge)->bulk_input = (GtkWidget *) ags_effect_bulk_new(audio,
												     AGS_TYPE_INPUT);
  AGS_EFFECT_BULK(AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge)->bulk_input)->flags |= (AGS_EFFECT_BULK_HIDE_BUTTONS |
											     AGS_EFFECT_BULK_HIDE_ENTRIES |
											     AGS_EFFECT_BULK_SHOW_LABELS);
  gtk_table_attach(table,
		   AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge)->bulk_input,
		   0, 1,
		   0, 1,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  lv2_bridge->has_gui = FALSE;
  lv2_bridge->gui_filename = NULL;
  lv2_bridge->gui_uri = NULL;
  
  lv2_bridge->lv2_gui = NULL;

  /* lv2 menu */
  item = gtk_image_menu_item_new_with_label("Lv2\0");
  gtk_menu_shell_append(AGS_MACHINE(lv2_bridge)->popup,
			item);
  gtk_widget_show(item);
  
  lv2_bridge->lv2_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(item,
			    lv2_bridge->lv2_menu);

  item = gtk_image_menu_item_new_with_label("show GUI\0");
  gtk_menu_shell_append(lv2_bridge->lv2_menu,
			item);

  gtk_widget_show_all(lv2_bridge->lv2_menu);
}

void
ags_lv2_bridge_set_property(GObject *gobject,
			    guint prop_id,
			    const GValue *value,
			    GParamSpec *param_spec)
{
  AgsLv2Bridge *lv2_bridge;

  lv2_bridge = AGS_LV2_BRIDGE(gobject);

  switch(prop_id){
  case PROP_FILENAME:
    {
      gchar *filename;

      filename = g_value_get_string(value);

      if(filename == lv2_bridge->filename){
	return;
      }

      if(lv2_bridge->filename != NULL){
	g_free(lv2_bridge->filename);
      }

      lv2_bridge->filename = g_strdup(filename);
    }
    break;
  case PROP_EFFECT:
    {
      gchar *effect;
      
      effect = g_value_get_string(value);

      if(effect == lv2_bridge->effect){
	return;
      }

      if(lv2_bridge->effect != NULL){
	g_free(lv2_bridge->effect);
      }

      lv2_bridge->effect = g_strdup(effect);
    }
    break;
  case PROP_URI:
    {
      gchar *uri;
      
      uri = g_value_get_string(value);

      if(uri == lv2_bridge->uri){
	return;
      }

      if(lv2_bridge->uri != NULL){
	g_free(lv2_bridge->uri);
      }

      lv2_bridge->uri = g_strdup(uri);
    }
    break;
  case PROP_INDEX:
    {
      unsigned long *uri_index;
      
      uri_index = g_value_get_ulong(value);

      if(uri_index == lv2_bridge->uri_index){
	return;
      }

      lv2_bridge->uri_index = uri_index;
    }
    break;
  case PROP_HAS_MIDI:
    {
      gboolean has_midi;

      has_midi = g_value_get_boolean(value);

      if(lv2_bridge->has_midi == has_midi){
	return;
      }

      lv2_bridge->has_midi = has_midi;
    }
    break;
  case PROP_HAS_GUI:
    {
      GtkWindow *window;
      gboolean has_gui;

      has_gui = g_value_get_boolean(value);

      if(lv2_bridge->has_gui == has_gui){
	return;
      }

      lv2_bridge->has_gui = has_gui;

      if(has_gui){
	lv2_bridge->lv2_gui = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(lv2_bridge->lv2_gui), "delete-event\0",
			 G_CALLBACK(ags_lv2_bridge_delete_event_callback), lv2_bridge);
      }else{
	gtk_widget_destroy(lv2_bridge->lv2_gui);
	lv2_bridge->lv2_gui = NULL;
      }
    }
    break;
  case PROP_GUI_FILENAME:
    {
      GtkWindow *window;
      
      gchar *gui_filename;

      gui_filename = g_value_get_string(value);

      if(lv2_bridge->gui_filename == gui_filename){
	return;
      }

      if(lv2_bridge->gui_filename != NULL){
	gtk_widget_destroy(gtk_bin_get_child((GtkBin *) lv2_bridge->lv2_gui));
	free(lv2_bridge->gui_filename);
      }

      lv2_bridge->gui_filename = g_strdup(gui_filename);

      /* load GUI */
      if(gui_filename != NULL &&
	 lv2_bridge->lv2_gui != NULL){
	GtkWidget *widget;
	AgsLv2uiPlugin *lv2ui_plugin;

	gchar *uri;
	
	LV2UI_DescriptorFunction lv2ui_descriptor;
	LV2UI_Descriptor *ui_descriptor;
	
	uint32_t ui_index;

	static const LV2_Feature **feature = {
	  NULL,
	};

	lv2ui_plugin = ags_lv2ui_manager_find_lv2ui_plugin(gui_filename);

	if(AGS_BASE_PLUGIN(lv2ui_plugin)->plugin_so == NULL){
	  AGS_BASE_PLUGIN(lv2ui_plugin)->plugin_so = dlopen(AGS_BASE_PLUGIN(lv2ui_plugin)->filename,
							    RTLD_NOW);
	}

	if(AGS_BASE_PLUGIN(lv2ui_plugin)->plugin_so){
	  lv2ui_descriptor = (LV2UI_Descriptor *) dlsym(AGS_BASE_PLUGIN(lv2ui_plugin)->plugin_so,
							"lv2ui_descriptor\0");
	  
	  if(dlerror() == NULL && lv2ui_descriptor){
	    ui_index = ags_lv2ui_manager_uri_index(gui_filename,
						   lv2_bridge->gui_uri);

	    if(ui_index != -1){
	      GtkVBox *vbox;
	      GtkAlignment *alignment;

	      gchar *bundle_path;

	      ui_descriptor = lv2ui_descriptor(ui_index);

	      vbox = (GtkVBox *) gtk_vbox_new(FALSE,
					      0);
	      gtk_container_add(lv2_bridge->lv2_gui,
				vbox);
	      
	      alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	      gtk_box_pack_start(GTK_BOX(vbox),
				 alignment,
				 TRUE, TRUE,
				 0);
	      
	      /* instantiate and pack ui */
	      bundle_path = g_strndup(gui_filename,
				      rindex(gui_filename, '/') - gui_filename);
	      widget = NULL;
	      lv2_bridge->ui_handle = ui_descriptor->instantiate(ui_descriptor,
								 lv2_bridge->uri,
								 bundle_path,
								 ags_lv2_bridge_lv2ui_write_function,
								 lv2_bridge,
								 &widget,
								 feature);
	      lv2_bridge->ui_widget = widget;
	      gtk_container_add(alignment,
				widget);
	      
	      gtk_widget_show_all(vbox);
	    }
	  }
	}
      }
    }
    break;
  case PROP_GUI_URI:
    {
      gchar *gui_uri;
      
      gui_uri = g_value_get_string(value);

      if(gui_uri == lv2_bridge->gui_uri){
	return;
      }

      if(lv2_bridge->gui_uri != NULL){
	g_free(lv2_bridge->gui_uri);
      }

      lv2_bridge->gui_uri = g_strdup(gui_uri);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_lv2_bridge_get_property(GObject *gobject,
			    guint prop_id,
			    GValue *value,
			    GParamSpec *param_spec)
{
  AgsLv2Bridge *lv2_bridge;

  lv2_bridge = AGS_LV2_BRIDGE(gobject);

  switch(prop_id){
  case PROP_FILENAME:
    {
      g_value_set_string(value, lv2_bridge->filename);
    }
    break;
  case PROP_EFFECT:
    {
      g_value_set_string(value, lv2_bridge->effect);
    }
    break;
  case PROP_URI:
    {
      g_value_set_string(value, lv2_bridge->uri);
    }
    break;
  case PROP_INDEX:
    {
      g_value_set_ulong(value, lv2_bridge->uri_index);
    }
    break;
  case PROP_HAS_MIDI:
    {
      g_value_set_boolean(value, lv2_bridge->has_midi);
    }
    break;
  case PROP_HAS_GUI:
    {
      g_value_set_boolean(value, lv2_bridge->has_gui);
    }
    break;
  case PROP_GUI_FILENAME:
    {
      g_value_set_string(value, lv2_bridge->gui_filename);
    }
    break;
  case PROP_GUI_URI:
    {
      g_value_set_string(value, lv2_bridge->gui_uri);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_lv2_bridge_connect(AgsConnectable *connectable)
{
  AgsLv2Bridge *lv2_bridge;

  GList *list;

  if((AGS_MACHINE_CONNECTED & (AGS_MACHINE(connectable)->flags)) != 0){
    return;
  }

  ags_lv2_bridge_parent_connectable_interface->connect(connectable);

  lv2_bridge = AGS_LV2_BRIDGE(connectable);

  /* menu */
  list = gtk_container_get_children(lv2_bridge->lv2_menu);

  g_signal_connect(G_OBJECT(list->data), "activate\0",
		   G_CALLBACK(ags_lv2_bridge_show_gui_callback), lv2_bridge);
}

void
ags_lv2_bridge_disconnect(AgsConnectable *connectable)
{
  //TODO:JK: implement me
}

gchar*
ags_lv2_bridge_get_version(AgsPlugin *plugin)
{
  return(AGS_LV2_BRIDGE(plugin)->version);
}

void
ags_lv2_bridge_set_version(AgsPlugin *plugin, gchar *version)
{
  AgsLv2Bridge *lv2_bridge;

  lv2_bridge = AGS_LV2_BRIDGE(plugin);

  lv2_bridge->version = version;
}

gchar*
ags_lv2_bridge_get_build_id(AgsPlugin *plugin)
{
  return(AGS_LV2_BRIDGE(plugin)->build_id);
}

void
ags_lv2_bridge_set_build_id(AgsPlugin *plugin, gchar *build_id)
{
  AgsLv2Bridge *lv2_bridge;

  lv2_bridge = AGS_LV2_BRIDGE(plugin);

  lv2_bridge->build_id = build_id;
}


gchar*
ags_lv2_bridge_get_xml_type(AgsPlugin *plugin)
{
  return(AGS_LV2_BRIDGE(plugin)->xml_type);
}

void
ags_lv2_bridge_set_xml_type(AgsPlugin *plugin, gchar *xml_type)
{
  AGS_LV2_BRIDGE(plugin)->xml_type = xml_type;
}

void
ags_lv2_bridge_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin)
{
  AgsLv2Bridge *gobject;
  AgsFileLaunch *file_launch;

  gobject = AGS_LV2_BRIDGE(plugin);

  g_object_set(gobject,
	       "filename\0", xmlGetProp(node,
					"filename\0"),
	       "effect\0", xmlGetProp(node,
				      "effect\0"),
	       NULL);

  /* launch */
  file_launch = (AgsFileLaunch *) g_object_new(AGS_TYPE_FILE_LAUNCH,
					       "node\0", node,
					       NULL);
  g_signal_connect(G_OBJECT(file_launch), "start\0",
		   G_CALLBACK(ags_lv2_bridge_launch_task), gobject);
  ags_file_add_launch(file,
		      G_OBJECT(file_launch));
}

void
ags_lv2_bridge_launch_task(AgsFileLaunch *file_launch, AgsLv2Bridge *lv2_bridge)
{
  GtkTreeModel *model;

  GtkTreeIter iter;

  GList *list, *list_start;
  GList *recall;
  
  ags_lv2_bridge_load(lv2_bridge);

  /* block update bulk port */
  list_start = 
    list = gtk_container_get_children(AGS_EFFECT_BULK(AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge)->bulk_input)->table);

  while(list != NULL){
    if(AGS_IS_BULK_MEMBER(list->data)){
      AGS_BULK_MEMBER(list->data)->flags |= AGS_BULK_MEMBER_NO_UPDATE;
    }

    list = list->next;
  }

  /* update value and unblock update bulk port */
  recall = NULL;
  
  if(AGS_MACHINE(lv2_bridge)->audio->input != NULL){
    recall = AGS_MACHINE(lv2_bridge)->audio->input->recall;
    
    while((recall = ags_recall_template_find_type(recall, AGS_TYPE_RECALL_LV2)) != NULL){
      if(!g_strcmp0(AGS_RECALL_LV2(recall->data)->filename,
		  lv2_bridge->filename) &&
	 !g_strcmp0(AGS_RECALL_LV2(recall->data)->effect,
		    lv2_bridge->effect)){
	break;
      }

      recall = recall->next;
    }
  }

  while(list != NULL){
    if(AGS_IS_BULK_MEMBER(list->data)){
      GtkWidget *child_widget;
      
      GList *port;

      child_widget = gtk_bin_get_child(list->data);
      
      if(recall != NULL){
	port = AGS_RECALL(recall->data)->port;

	while(port != port->next){
	  if(!g_strcmp0(AGS_BULK_MEMBER(list->data)->specifier,
			AGS_PORT(port->data)->specifier)){
	    if(AGS_IS_DIAL(child_widget)){
	      gtk_adjustment_set_value(AGS_DIAL(child_widget)->adjustment,
				       AGS_PORT(port->data)->port_value.ags_port_ladspa);
	      ags_dial_draw(child_widget);
	    }else if(GTK_IS_TOGGLE_BUTTON(child_widget)){
	      gtk_toggle_button_set_active(child_widget,
					   ((AGS_PORT(port->data)->port_value.ags_port_ladspa != 0.0) ? TRUE: FALSE));
	    }

	    break;
	  }

	  port = port->next;
	}
      }
     
      AGS_BULK_MEMBER(list->data)->flags &= (~AGS_BULK_MEMBER_NO_UPDATE);
    }
    
    list = list->next;
  }

  g_list_free(list_start);
}

xmlNode*
ags_lv2_bridge_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin)
{
  AgsLv2Bridge *lv2_bridge;

  xmlNode *node;

  gchar *id;
  
  lv2_bridge = AGS_LV2_BRIDGE(plugin);

  id = ags_id_generator_create_uuid();
    
  node = xmlNewNode(NULL,
		    "ags-lv2-bridge\0");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);

  xmlNewProp(node,
	     "filename\0",
	     lv2_bridge->filename);

  xmlNewProp(node,
	     "effect\0",
	     lv2_bridge->effect);
  
  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "application-context\0", file->application_context,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", lv2_bridge,
				   NULL));

  xmlAddChild(parent,
	      node);

  return(node);
}

void
ags_lv2_bridge_set_audio_channels(AgsAudio *audio,
				  guint audio_channels, guint audio_channels_old,
				  gpointer data)
{
  AgsMachine *machine;
  AgsLv2Bridge *lv2_bridge;

  AgsChannel *channel, *next_pad;
  AgsAudioSignal *audio_signal;  

  /* get machine */
  lv2_bridge = (AgsLv2Bridge *) audio->machine;
  machine = AGS_MACHINE(lv2_bridge);
  
  if(audio->input_pads == 0 &&
     audio->output_pads == 0){
    return;
  }

  if(audio_channels > audio_channels_old){
    /* AgsInput */
    channel = audio->input;

    while(channel != NULL){
      next_pad = channel->next_pad;
      channel = ags_channel_pad_nth(channel,
				    audio_channels_old);

      while(channel != next_pad){
	audio_signal = ags_audio_signal_new(audio->soundcard,
					    channel->first_recycling,
					    NULL);
	audio_signal->flags |= AGS_AUDIO_SIGNAL_TEMPLATE;
	audio_signal->loop_start = 0;
	audio_signal->loop_end = audio_signal->buffer_size;
	ags_audio_signal_stream_resize(audio_signal,
				       1);
	ags_recycling_add_audio_signal(channel->first_recycling,
				       audio_signal);
	
	channel = channel->next;
      }
    }

    /* AgsOutput */
    channel = audio->output;

    while(channel != NULL){
      next_pad = channel->next_pad;
      channel = ags_channel_pad_nth(channel,
				    audio_channels_old);

      while(channel != next_pad){
	audio_signal = ags_audio_signal_new(audio->soundcard,
					    channel->first_recycling,
					    NULL);
	audio_signal->flags |= AGS_AUDIO_SIGNAL_TEMPLATE;
	ags_audio_signal_stream_resize(audio_signal,
				       3);
	ags_recycling_add_audio_signal(channel->first_recycling,
				       audio_signal);
	
	channel = channel->next;
      }
    }

    /* recall */
    if((AGS_MACHINE_MAPPED_RECALL & (machine->flags)) != 0){
      ags_lv2_bridge_input_map_recall(lv2_bridge,
				      audio_channels_old,
				      0);

      ags_lv2_bridge_output_map_recall(lv2_bridge,
				       audio_channels_old,
				       0);
    }
  }
}

void
ags_lv2_bridge_set_pads(AgsAudio *audio, GType type,
			guint pads, guint pads_old,
			gpointer data)
{
  AgsMachine *machine;
  AgsLv2Bridge *lv2_bridge;

  AgsChannel *channel;
  AgsAudioSignal *audio_signal;
  
  gboolean grow;

  if(pads == pads_old ||
     audio->audio_channels == 0){
    return;
  }

  /* get machine */
  lv2_bridge = (AgsLv2Bridge *) audio->machine;
  machine = AGS_MACHINE(lv2_bridge);

  if(pads_old < pads){
    grow = TRUE;
  }else{
    grow = FALSE;
  }
  
  if(g_type_is_a(type, AGS_TYPE_INPUT)){
    if(grow){
      /* AgsInput */
      channel = ags_channel_pad_nth(audio->input,
				    pads_old);

      while(channel != NULL){
	audio_signal = ags_audio_signal_new(audio->soundcard,
					    channel->first_recycling,
					    NULL);
	audio_signal->flags |= AGS_AUDIO_SIGNAL_TEMPLATE;
	audio_signal->loop_start = 0;
	audio_signal->loop_end = audio_signal->buffer_size;
	ags_audio_signal_stream_resize(audio_signal,
				       1);
	ags_recycling_add_audio_signal(channel->first_recycling,
				       audio_signal);
	
	channel = channel->next;
      }

      /* recall */
      if((AGS_MACHINE_MAPPED_RECALL & (machine->flags)) != 0){
	ags_lv2_bridge_input_map_recall(lv2_bridge,
					0,
					pads_old);
      }
    }else{
      lv2_bridge->mapped_input_pad = pads;
    }
  }else{
    if(grow){
      /* AgsOutput */
      channel = ags_channel_pad_nth(audio->output,
				    pads_old);

      while(channel != NULL){
	audio_signal = ags_audio_signal_new(audio->soundcard,
					    channel->first_recycling,
					    NULL);
	audio_signal->flags |= AGS_AUDIO_SIGNAL_TEMPLATE;
	ags_audio_signal_stream_resize(audio_signal,
				       3);
	ags_recycling_add_audio_signal(channel->first_recycling,
				       audio_signal);
	
	channel = channel->next;
      }

      /* recall */
      if((AGS_MACHINE_MAPPED_RECALL & (machine->flags)) != 0){
	ags_lv2_bridge_output_map_recall(lv2_bridge,
					 0,
					 pads_old);
      }
    }else{
      lv2_bridge->mapped_output_pad = pads;
    }
  }
}

void
ags_lv2_bridge_map_recall(AgsMachine *machine)
{  
  AgsWindow *window;
  AgsLv2Bridge *lv2_bridge;
  
  AgsAudio *audio;

  AgsDelayAudio *play_delay_audio;
  AgsDelayAudioRun *play_delay_audio_run;
  AgsCountBeatsAudio *play_count_beats_audio;
  AgsCountBeatsAudioRun *play_count_beats_audio_run;
  AgsPlayNotationAudio *recall_notation_audio;
  AgsPlayNotationAudioRun *recall_notation_audio_run;
  AgsRouteLv2Audio *recall_route_lv2_audio;
  AgsRouteLv2AudioRun *recall_route_lv2_audio_run;

  GList *list;
  
  if((AGS_MACHINE_MAPPED_RECALL & (machine->flags)) != 0 ||
     (AGS_MACHINE_PREMAPPED_RECALL & (machine->flags)) != 0){
    return;
  }

  window = gtk_widget_get_ancestor(machine,
				   AGS_TYPE_WINDOW);

  lv2_bridge = machine;
  audio = machine->audio;

  /* ags-delay */
  if((AGS_MACHINE_IS_SYNTHESIZER & (machine->flags)) != 0){
    ags_recall_factory_create(audio,
			      NULL, NULL,
			      "ags-delay\0",
			      0, 0,
			      0, 0,
			      (AGS_RECALL_FACTORY_OUTPUT |
			       AGS_RECALL_FACTORY_ADD |
			       AGS_RECALL_FACTORY_PLAY |
			       AGS_RECALL_FACTORY_RECALL),
			      0);

    list = ags_recall_find_type(audio->play, AGS_TYPE_DELAY_AUDIO_RUN);

    if(list != NULL){
      play_delay_audio_run = AGS_DELAY_AUDIO_RUN(list->data);
      //    AGS_RECALL(play_delay_audio_run)->flags |= AGS_RECALL_PERSISTENT;
    }
  
    /* ags-count-beats */
    ags_recall_factory_create(audio,
			      NULL, NULL,
			      "ags-count-beats\0",
			      0, 0,
			      0, 0,
			      (AGS_RECALL_FACTORY_OUTPUT |
			       AGS_RECALL_FACTORY_ADD |
			       AGS_RECALL_FACTORY_PLAY |
			       AGS_RECALL_FACTORY_RECALL),
			      0);
  
    list = ags_recall_find_type(audio->play, AGS_TYPE_COUNT_BEATS_AUDIO_RUN);

    if(list != NULL){
      play_count_beats_audio_run = AGS_COUNT_BEATS_AUDIO_RUN(list->data);

      /* set dependency */  
      g_object_set(G_OBJECT(play_count_beats_audio_run),
		   "delay-audio-run\0", play_delay_audio_run,
		   NULL);
      ags_seekable_seek(AGS_SEEKABLE(play_count_beats_audio_run),
			window->navigation->position_tact->adjustment->value * ags_soundcard_get_delay(AGS_SOUNDCARD(audio->soundcard)),
			TRUE);
    }

    /* ags-route-lv2 */
    ags_recall_factory_create(audio,
			      NULL, NULL,
			      "ags-route-lv2\0",
			      0, 0,
			      0, 0,
			      (AGS_RECALL_FACTORY_INPUT |
			       AGS_RECALL_FACTORY_ADD |
			       AGS_RECALL_FACTORY_RECALL),
			      0);

    list = ags_recall_find_type(audio->recall, AGS_TYPE_ROUTE_LV2_AUDIO_RUN);

    if(list != NULL){
      recall_route_lv2_audio_run = AGS_ROUTE_LV2_AUDIO_RUN(list->data);

      /* set dependency */
      g_object_set(G_OBJECT(recall_route_lv2_audio_run),
		   "delay-audio-run\0", play_delay_audio_run,
		   NULL);

      /* set dependency */
      g_object_set(G_OBJECT(recall_route_lv2_audio_run),
		   "count-beats-audio-run\0", play_count_beats_audio_run,
		   NULL);
    }

    /* ags-play-notation */
    ags_recall_factory_create(audio,
			      NULL, NULL,
			      "ags-play-notation\0",
			      0, 0,
			      0, 0,
			      (AGS_RECALL_FACTORY_INPUT |
			       AGS_RECALL_FACTORY_ADD |
			       AGS_RECALL_FACTORY_RECALL),
			      0);

    list = ags_recall_find_type(audio->recall, AGS_TYPE_PLAY_NOTATION_AUDIO_RUN);

    if(list != NULL){
      recall_notation_audio_run = AGS_PLAY_NOTATION_AUDIO_RUN(list->data);

      /* set dependency */
      g_object_set(G_OBJECT(recall_notation_audio_run),
		   "delay-audio-run\0", play_delay_audio_run,
		   NULL);

      /* set dependency */
      g_object_set(G_OBJECT(recall_notation_audio_run),
		   "count-beats-audio-run\0", play_count_beats_audio_run,
		   NULL);
    }
  }
  
  /* depending on destination */
  ags_lv2_bridge_input_map_recall(lv2_bridge, 0, 0);

  /* add new controls */
  ags_effect_bulk_add_effect(AGS_EFFECT_BRIDGE(AGS_MACHINE(lv2_bridge)->bridge)->bulk_input,
			     NULL,
			     lv2_bridge->filename,
			     lv2_bridge->effect);

  /* depending on destination */
  ags_lv2_bridge_output_map_recall(lv2_bridge, 0, 0);

  /* call parent */
  AGS_MACHINE_CLASS(ags_lv2_bridge_parent_class)->map_recall(machine);
}

void
ags_lv2_bridge_input_map_recall(AgsLv2Bridge *lv2_bridge, guint audio_channel_start, guint input_pad_start)
{
  AgsAudio *audio;
  AgsChannel *source, *current;
  
  GList *list;

  audio = AGS_MACHINE(lv2_bridge)->audio;

  if(lv2_bridge->mapped_input_pad > input_pad_start){
    return;
  }

  source = ags_channel_nth(audio->input,
			   audio_channel_start + input_pad_start * audio->audio_channels);

  if((AGS_MACHINE_IS_SYNTHESIZER & (AGS_MACHINE(lv2_bridge)->flags)) != 0){
    /* map dependending on output */
    current = source;

    while(current != NULL){
      /* ags-buffer */
      ags_recall_factory_create(audio,
				NULL, NULL,
				"ags-buffer\0",
				audio_channel_start, audio->audio_channels, 
				current->pad, current->pad + 1,
				(AGS_RECALL_FACTORY_INPUT |
				 AGS_RECALL_FACTORY_RECALL |
				 AGS_RECALL_FACTORY_ADD),
				0);
    
      current = current->next_pad;
    }
  
    /*  */
    current = source;

    while(current != NULL){
      /* ags-play */
      ags_recall_factory_create(audio,
				NULL, NULL,
				"ags-play\0",
				audio_channel_start, audio->audio_channels, 
				current->pad, current->pad + 1,
				(AGS_RECALL_FACTORY_INPUT |
				 AGS_RECALL_FACTORY_PLAY |
				 AGS_RECALL_FACTORY_ADD),
				0);

      current = current->next_pad;
    }

    /*  */
    current = source;

    while(current != NULL){
      /* ags-stream */
      ags_recall_factory_create(audio,
				NULL, NULL,
				"ags-stream\0",
				audio_channel_start, audio->audio_channels, 
				current->pad, current->pad + 1,
				(AGS_RECALL_FACTORY_INPUT |
				 AGS_RECALL_FACTORY_PLAY |
				 AGS_RECALL_FACTORY_RECALL | 
				 AGS_RECALL_FACTORY_ADD),
				0);

      current = current->next_pad;
    }
  }
  
  lv2_bridge->mapped_input_pad = audio->input_pads;
}

void
ags_lv2_bridge_output_map_recall(AgsLv2Bridge *lv2_bridge, guint audio_channel_start, guint output_pad_start)
{
  AgsAudio *audio;
  AgsChannel *source, *input, *current;

  AgsDelayAudio *recall_delay_audio;
  AgsCountBeatsAudioRun *recall_count_beats_audio_run;

  GList *list;

  audio = AGS_MACHINE(lv2_bridge)->audio;

  if(lv2_bridge->mapped_output_pad > output_pad_start){
    return;
  }

  source = ags_channel_nth(audio->output,
			   audio_channel_start + output_pad_start * audio->audio_channels);

  if((AGS_MACHINE_IS_SYNTHESIZER & (AGS_MACHINE(lv2_bridge)->flags)) != 0){
    /* remap for input */
    input = audio->input;

    while(input != NULL){
      /* ags-buffer */
      ags_recall_factory_create(audio,
				NULL, NULL,
				"ags-buffer\0",
				audio_channel_start, audio->audio_channels, 
				input->pad, input->pad + 1,
				(AGS_RECALL_FACTORY_INPUT |
				 AGS_RECALL_FACTORY_RECALL |
				 AGS_RECALL_FACTORY_REMAP),
				0);
    
      input = input->next_pad;
    }

    current = ags_channel_nth(audio->output,
			      audio_channel_start + output_pad_start * audio->audio_channels);

    while(current != NULL){
      /* ags-stream */
      ags_recall_factory_create(audio,
				NULL, NULL,
				"ags-stream\0",
				audio_channel_start, audio->audio_channels,
				current->pad, current->pad + 1,
				(AGS_RECALL_FACTORY_OUTPUT |
				 AGS_RECALL_FACTORY_PLAY |
				 AGS_RECALL_FACTORY_RECALL | 
				 AGS_RECALL_FACTORY_ADD),
				0);

      current = current->next_pad;
    }
  }
  
  lv2_bridge->mapped_output_pad = audio->output_pads;
}

void
ags_lv2_bridge_load_gui(AgsLv2Bridge *lv2_bridge)
{
  AgsLv2Plugin *lv2_plugin;
  AgsLv2uiPlugin *lv2ui_plugin;

  GList *gtkui_list;
  GList *binary_list;

  gchar *str;
  gchar *plugin_path;
  gchar *filename;
  gchar *gui_uri;
  
  /* check if works with Gtk+ */
  lv2_plugin = ags_lv2_manager_find_lv2_plugin(lv2_bridge->filename,
					       lv2_bridge->effect);

  if(lv2_plugin == NULL){
    return;
  }

  gtkui_list = ags_turtle_find_xpath(lv2_plugin->turtle,
				     "//rdf-triple//rdf-verb[@verb='a']/following-sibling::*[self::rdf-object-list]//rdf-pname-ln[substring(text(), string-length(text()) - string-length(':GtkUI') + 1) = ':GtkUI']\0");

  if(gtkui_list == NULL){
    return;
  }
  
  /* read ui binary from turtle */
  binary_list = ags_turtle_find_xpath(lv2_plugin->turtle,
				      "//rdf-triple//rdf-verb//rdf-pname-ln[text()='uiext:binary']/ancestor::*[self::rdf-verb][1]/following-sibling::*[self::rdf-object-list][1]//rdf-iriref\0");

  if(binary_list == NULL){
    return;
  }

  str = xmlNodeGetContent(binary_list->data);

  if(strlen(str) < 2){
    return;
  }

  plugin_path = g_strndup(lv2_bridge->filename,
			  rindex(lv2_bridge->filename, '/') - lv2_bridge->filename);
  
  str = g_strndup(&(str[1]),
		  strlen(str) - 2);
  filename = g_strdup_printf("%s/%s\0",
			     plugin_path,
			     str);
  free(str);

#ifdef AGS_DEBUG
  g_message("plugin path - %s\0", plugin_path);
#endif
  
  gui_uri = ags_lv2ui_manager_find_uri(filename,
				       lv2_bridge->effect);

  /* apply ui */
  g_object_set(lv2_bridge,
	       "has-gui\0", TRUE,
	       "gui-uri\0", gui_uri,
	       "gui-filename\0", filename,
	       NULL);
}

void
ags_lv2_bridge_load(AgsLv2Bridge *lv2_bridge)
{
  AgsLv2Plugin *lv2_plugin;
  
  gchar *uri;

  lv2_plugin = ags_lv2_manager_find_lv2_plugin(lv2_bridge->filename,
					       lv2_bridge->effect);

  if(lv2_plugin == NULL){
    return;
  }
  
  g_object_set(lv2_bridge,
	       "uri\0", lv2_plugin->uri,
	       NULL);
  
  ags_lv2_bridge_load_gui(lv2_bridge);
}

/**
 * ags_lv2_bridge_new:
 * @soundcard: the assigned soundcard.
 * @filename: the plugin.so
 * @effect: the effect
 *
 * Creates an #AgsLv2Bridge
 *
 * Returns: a new #AgsLv2Bridge
 *
 * Since: 0.4.3
 */
AgsLv2Bridge*
ags_lv2_bridge_new(GObject *soundcard,
		   gchar *filename,
		   gchar *effect)
{
  AgsLv2Bridge *lv2_bridge;
  GValue value = {0,};

  lv2_bridge = (AgsLv2Bridge *) g_object_new(AGS_TYPE_LV2_BRIDGE,
					     NULL);

  if(soundcard != NULL){
    g_value_init(&value, G_TYPE_OBJECT);
    g_value_set_object(&value, soundcard);
    g_object_set_property(G_OBJECT(AGS_MACHINE(lv2_bridge)->audio),
			  "soundcard\0", &value);
    g_value_unset(&value);
  }

  g_object_set(lv2_bridge,
	       "filename\0", filename,
	       "effect\0", effect,
	       NULL);

  return(lv2_bridge);
}
