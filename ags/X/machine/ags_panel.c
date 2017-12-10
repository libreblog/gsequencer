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

#include <ags/X/machine/ags_panel.h>
#include <ags/X/machine/ags_panel_callbacks.h>

#include <ags/libags.h>
#include <ags/libags-audio.h>
#include <ags/libags-gui.h>

#include <ags/X/ags_window.h>

#include <ags/X/file/ags_gui_file_xml.h>

#include <ags/X/machine/ags_panel_input_pad.h>
#include <ags/X/machine/ags_panel_input_line.h>

void ags_panel_class_init(AgsPanelClass *panel);
void ags_panel_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_panel_plugin_interface_init(AgsPluginInterface *plugin);
void ags_panel_init(AgsPanel *panel);
static void ags_panel_finalize(GObject *gobject);
void ags_panel_connect(AgsConnectable *connectable);
void ags_panel_disconnect(AgsConnectable *connectable);
void ags_panel_show(GtkWidget *widget);
void ags_panel_map_recall(AgsMachine *machine);
gchar* ags_panel_get_name(AgsPlugin *plugin);
void ags_panel_set_name(AgsPlugin *plugin, gchar *name);
gchar* ags_panel_get_xml_type(AgsPlugin *plugin);
void ags_panel_set_xml_type(AgsPlugin *plugin, gchar *xml_type);
void ags_panel_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin);
xmlNode* ags_panel_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin);
void ags_panel_read_resolve_audio(AgsFileLookup *file_lookup,
				  AgsMachine *machine);

void ags_file_read_panel(AgsFile *file, xmlNode *node, AgsMachine *panel);
xmlNode* ags_file_write_panel(AgsFile *file, xmlNode *parent, AgsMachine *panel);

void ags_panel_resize_audio_channels(AgsMachine *machine,
				     guint audio_channels, guint audio_channels_old,
				     gpointer data);
void ags_panel_resize_pads(AgsMachine *machine, GType channel_type,
			   guint pads, guint pads_old,
			   gpointer data);

/**
 * SECTION:ags_panel
 * @short_description: panel
 * @title: AgsPanel
 * @section_id:
 * @include: ags/X/machine/ags_panel.h
 *
 * The #AgsPanel is a composite widget to act as panel.
 */

static gpointer ags_panel_parent_class = NULL;
static AgsConnectableInterface *ags_panel_parent_connectable_interface;

GType
ags_panel_get_type(void)
{
  static GType ags_type_panel = 0;

  if(!ags_type_panel){
    static const GTypeInfo ags_panel_info = {
      sizeof(AgsPanelClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_panel_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsPanel),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_panel_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_panel_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_panel_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };
    
    ags_type_panel = g_type_register_static(AGS_TYPE_MACHINE,
					    "AgsPanel", &ags_panel_info,
					    0);
    
    g_type_add_interface_static(ags_type_panel,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_panel,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return(ags_type_panel);
}

void
ags_panel_class_init(AgsPanelClass *panel)
{
  GObjectClass *gobject;
  GtkWidgetClass *widget;
  AgsMachineClass *machine;

  ags_panel_parent_class = g_type_class_peek_parent(panel);

  /* GtkObjectClass */
  gobject = (GObjectClass *) panel;

  gobject->finalize = ags_panel_finalize;

  /* GtkWidgetClass */
  widget = (GtkWidgetClass *) panel;

  widget->show = ags_panel_show;

  /* AgsMachine */
  machine = (AgsMachineClass *) panel;

  machine->map_recall = ags_panel_map_recall;
}

void
ags_panel_connectable_interface_init(AgsConnectableInterface *connectable)
{
  AgsConnectableInterface *ags_panel_connectable_parent_interface;

  ags_panel_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_panel_connect;
  connectable->disconnect = ags_panel_disconnect;
}

void
ags_panel_plugin_interface_init(AgsPluginInterface *plugin)
{
  plugin->get_name = ags_panel_get_name;
  plugin->set_name = ags_panel_set_name;
  plugin->get_xml_type = ags_panel_get_xml_type;
  plugin->set_xml_type = ags_panel_set_xml_type;
  plugin->read = ags_panel_read;
  plugin->write = ags_panel_write;
}

void
ags_panel_init(AgsPanel *panel)
{
  g_signal_connect_after((GObject *) panel, "parent-set",
			 G_CALLBACK(ags_panel_parent_set_callback), (gpointer) panel);

  ags_machine_popup_add_connection_options((AgsMachine *) panel,
					   (AGS_MACHINE_POPUP_CONNECTION_EDITOR));

  AGS_MACHINE(panel)->audio->flags |= (AGS_AUDIO_SYNC);
  AGS_MACHINE(panel)->input_pad_type = AGS_TYPE_PANEL_INPUT_PAD;

  AGS_MACHINE(panel)->input_pad_type = AGS_TYPE_PANEL_INPUT_PAD;
  AGS_MACHINE(panel)->input_line_type = AGS_TYPE_PANEL_INPUT_LINE;
  AGS_MACHINE(panel)->output_pad_type = G_TYPE_NONE;
  AGS_MACHINE(panel)->output_line_type = G_TYPE_NONE;

  g_signal_connect(G_OBJECT(panel), "resize-audio-channels",
		   G_CALLBACK(ags_panel_resize_audio_channels), NULL);

  g_signal_connect(G_OBJECT(panel), "resize-pads",
		   G_CALLBACK(ags_panel_resize_pads), NULL);

  /* */
  panel->name = NULL;
  panel->xml_type = "ags-panel";

  panel->vbox = (GtkVBox *) gtk_vbox_new(FALSE, 0);
  gtk_container_add((GtkContainer*) (gtk_bin_get_child((GtkBin *) panel)), (GtkWidget *) panel->vbox);

  //  AGS_MACHINE(panel)->output = (GtkContainer *) gtk_hbox_new(FALSE, 0);
  //  gtk_box_pack_start((GtkBox *) panel->vbox, (GtkWidget *) AGS_MACHINE(panel)->output, FALSE, FALSE, 0);

  AGS_MACHINE(panel)->input = (GtkContainer *) gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start((GtkBox *) panel->vbox, (GtkWidget *) AGS_MACHINE(panel)->input, FALSE, FALSE, 0);
}

static void
ags_panel_finalize(GObject *gobject)
{
  AgsPanel *panel;

  panel = (AgsPanel *) gobject;
  
  g_object_disconnect(G_OBJECT(panel),
		      "any_signal::resize-audio-channels",
		      G_CALLBACK(ags_panel_resize_audio_channels),
		      NULL,
		      "any_signal::resize-pads",
		      G_CALLBACK(ags_panel_resize_pads),
		      NULL,
		      NULL);

  /* call parent */
  G_OBJECT_CLASS(ags_panel_parent_class)->finalize(gobject);
}

void
ags_panel_connect(AgsConnectable *connectable)
{
  if((AGS_MACHINE_CONNECTED & (AGS_MACHINE(connectable)->flags)) != 0){
    return;
  }

  ags_panel_parent_connectable_interface->connect(connectable);

  /* empty */
}

void
ags_panel_disconnect(AgsConnectable *connectable)
{
  if((AGS_MACHINE_CONNECTED & (AGS_MACHINE(connectable)->flags)) == 0){
    return;
  }

  ags_panel_parent_connectable_interface->disconnect(connectable);

  /* empty */
}

void
ags_panel_show(GtkWidget *widget)
{
  GTK_WIDGET_CLASS(ags_panel_parent_class)->show(widget);
}

void
ags_panel_map_recall(AgsMachine *machine)
{
  AGS_MACHINE_CLASS(ags_panel_parent_class)->map_recall(machine);

  /* empty */
}

gchar*
ags_panel_get_name(AgsPlugin *plugin)
{
  return(AGS_PANEL(plugin)->name);
}

void
ags_panel_set_name(AgsPlugin *plugin, gchar *name)
{
  AGS_PANEL(plugin)->name = name;
}

gchar*
ags_panel_get_xml_type(AgsPlugin *plugin)
{
  return(AGS_PANEL(plugin)->xml_type);
}

void
ags_panel_set_xml_type(AgsPlugin *plugin, gchar *xml_type)
{
  AGS_PANEL(plugin)->xml_type = xml_type;
}

void
ags_panel_read(AgsFile *file, xmlNode *node, AgsPlugin *plugin)
{
  AgsPanel *gobject;
  AgsFileLookup *file_lookup;
  GList *list;
  guint64 length, index;

  gobject = AGS_PANEL(plugin);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "application-context", file->application_context,
				   "file", file,
				   "node", node,
				   "xpath", g_strdup_printf("xpath=//*[@id='%s']", xmlGetProp(node, AGS_FILE_ID_PROP)),
				   "reference", gobject,
				   NULL));

  list = file->lookup;

  while((list = ags_file_lookup_find_by_node(list,
					     node->parent)) != NULL){
    file_lookup = AGS_FILE_LOOKUP(list->data);
    
    if(g_signal_handler_find(list->data,
			     G_SIGNAL_MATCH_FUNC,
			     0,
			     0,
			     NULL,
			     ags_file_read_machine_resolve_audio,
			     NULL) != 0){
      g_signal_connect_after(G_OBJECT(file_lookup), "resolve",
			     G_CALLBACK(ags_panel_read_resolve_audio), gobject);
      
      break;
    }

    list = list->next;
  }
}

void
ags_panel_read_resolve_audio(AgsFileLookup *file_lookup,
			     AgsMachine *machine)
{
  AgsPanel *panel;
  GList *pad, *pad_start, *line, *line_start;

  panel = AGS_PANEL(machine);

  g_signal_connect_after(G_OBJECT(machine), "resize-audio-channels",
			 G_CALLBACK(ags_panel_resize_audio_channels), NULL);

  g_signal_connect_after(G_OBJECT(machine), "resize-pads",
			 G_CALLBACK(ags_panel_resize_pads), NULL);
}

xmlNode*
ags_panel_write(AgsFile *file, xmlNode *parent, AgsPlugin *plugin)
{
  AgsPanel *panel;
  AgsFileLookup *file_lookup;
  xmlNode *node;
  GList *list;
  gchar *id;
  guint i;

  panel = AGS_PANEL(plugin);

  id = ags_id_generator_create_uuid();
  
  node = xmlNewNode(NULL,
		    "ags-panel");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "application-context", file->application_context,
				   "file", file,
				   "node", node,
				   "xpath", g_strdup_printf("xpath=//*[@id='%s']", id),
				   "reference", panel,
				   NULL));

  xmlAddChild(parent,
	      node);

  return(node);
}

void
ags_panel_resize_audio_channels(AgsMachine *machine,
				guint audio_channels, guint audio_channels_old,
				gpointer data)
{
  AgsAudio *audio;
  AgsAudioConnection *audio_connection;

  AgsMutexManager *mutex_manager;

  AgsConnectionManager *connection_manager;

  GObject *soundcard;
  
  GList *list;

  guint input_pads;
  guint i, j;

  pthread_mutex_t *application_mutex;
  pthread_mutex_t *audio_mutex;

  mutex_manager = ags_mutex_manager_get_instance();
  application_mutex = ags_mutex_manager_get_application_mutex(mutex_manager);
  
  connection_manager = ags_connection_manager_get_instance();

  audio = machine->audio;

  /* get audio mutex */
  pthread_mutex_lock(application_mutex);

  audio_mutex = ags_mutex_manager_lookup(mutex_manager,
					 (GObject *) audio);
  
  pthread_mutex_unlock(application_mutex);

  /* get some fields */
  pthread_mutex_lock(audio_mutex);

  soundcard = audio->soundcard;
  
  input_pads = audio->input_pads;
  
  pthread_mutex_unlock(audio_mutex);

  if(audio_channels > audio_channels_old){
    for(i = 0; i < audio->input_pads; i++){
      if(i != 0){
	for(j = 0; j < audio_channels_old; j++){
	  pthread_mutex_lock(audio_mutex);
	  
	  list = audio->audio_connection;
	  
	  while((list = ags_audio_connection_find(list,
						  AGS_TYPE_INPUT,
						  i,
						  j)) != NULL){
	    GObject *data_object;

	    g_object_get(G_OBJECT(list->data),
			 "data-object", &data_object,
			 NULL);
	    
	    if(AGS_IS_SOUNDCARD(data_object)){
	      break;
	    }

	    list = list->next;
	  }

	  if(list != NULL){
	    audio_connection = list->data;
	    audio_connection->line = i * audio_channels + j;

	    audio_connection->mapped_line = audio_connection->line;
	  }

	  pthread_mutex_unlock(audio_mutex);
	}
      }

      for(j = audio_channels_old; j < audio_channels; j++){
	audio_connection = g_object_new(AGS_TYPE_AUDIO_CONNECTION,
					"data-object", soundcard,
					NULL);
	audio_connection->flags |= (AGS_AUDIO_CONNECTION_IS_OUTPUT |
				    AGS_AUDIO_CONNECTION_IS_SOUNDCARD_DATA |
				    AGS_AUDIO_CONNECTION_SCOPE_LINE);
	
	audio_connection->audio = (GObject *) audio;
	audio_connection->channel_type = AGS_TYPE_INPUT;

	audio_connection->pad = i;
	audio_connection->audio_channel = j;
	audio_connection->line = i * audio_channels + j;

	audio_connection->mapped_line = audio_connection->line;
	
	ags_audio_add_audio_connection(audio,
				       (GObject *) audio_connection);

	ags_connection_manager_add_connection(connection_manager,
					      (AgsConnection *) audio_connection);
      }
    }
  }else{
    for(i = 0; i < input_pads; i++){
      for(j = audio_channels; j < audio_channels_old; j++){
	pthread_mutex_lock(audio_mutex);
	
	list = audio->audio_connection;
	
	while((list = ags_audio_connection_find(list,
						AGS_TYPE_INPUT,
						i,
						j)) != NULL){
	  GObject *data_object;

	  audio_connection = (AgsAudioConnection *) list->data;
	  
	  g_object_get(audio_connection,
		       "data-object", &data_object,
		       NULL);
	    
	  if(AGS_IS_SOUNDCARD(data_object)){
	    ags_audio_remove_audio_connection(audio,
					      (GObject *) audio_connection);
	    
	    ags_connection_manager_remove_connection(connection_manager,
						     (AgsConnection *) audio_connection);
	    
	    break;
	  }

	  list = list->next;
	}

	pthread_mutex_unlock(audio_mutex);
      }
    }
  }
}

void
ags_panel_resize_pads(AgsMachine *machine, GType channel_type,
		      guint pads, guint pads_old,
		      gpointer data)
{
  AgsAudio *audio;
  AgsAudioConnection *audio_connection;

  AgsMutexManager *mutex_manager;
  
  AgsConnectionManager *connection_manager;

  GObject *soundcard;
  
  GList *list;
  
  guint audio_channels;
  guint i, j;

  pthread_mutex_t *application_mutex;
  pthread_mutex_t *audio_mutex;

  mutex_manager = ags_mutex_manager_get_instance();
  application_mutex = ags_mutex_manager_get_application_mutex(mutex_manager);
  
  connection_manager = ags_connection_manager_get_instance();

  audio = machine->audio;

  /* get audio mutex */
  pthread_mutex_lock(application_mutex);

  audio_mutex = ags_mutex_manager_lookup(mutex_manager,
					 (GObject *) audio);
  
  pthread_mutex_unlock(application_mutex);  

  /* get some fields */
  pthread_mutex_lock(audio_mutex);

  soundcard = audio->soundcard;
  
  audio_channels = audio->audio_channels;
  
  pthread_mutex_unlock(audio_mutex);
  
  if(channel_type == AGS_TYPE_INPUT){
    if(pads > pads_old){
      for(i = pads_old; i < pads; i++){
	for(j = 0; j < audio_channels; j++){
	  audio_connection = g_object_new(AGS_TYPE_AUDIO_CONNECTION,
					  "data-object", soundcard,
					  NULL);
	  audio_connection->flags |= (AGS_AUDIO_CONNECTION_IS_OUTPUT |
				      AGS_AUDIO_CONNECTION_IS_SOUNDCARD_DATA |
				      AGS_AUDIO_CONNECTION_SCOPE_LINE);

	  audio_connection->audio = (GObject *) audio;
	  audio_connection->channel_type = AGS_TYPE_INPUT;

	  audio_connection->pad = i;
	  audio_connection->audio_channel = j;
	  audio_connection->line = i * audio_channels + j;

	  audio_connection->mapped_line = audio_connection->line;
	  
	  ags_audio_add_audio_connection(audio,
					 (GObject *) audio_connection);

	  ags_connection_manager_add_connection(connection_manager,
						(AgsConnection *) audio_connection);
	}
      }
    }else{
      for(i = pads; i < pads_old; i++){
	for(j = 0; j < audio_channels; j++){
	  pthread_mutex_lock(audio_mutex);
	  
	  list = audio->audio_connection;
	
	  while((list = ags_audio_connection_find(list,
						  AGS_TYPE_INPUT,
						  i,
						  j)) != NULL){
	    GObject *data_object;

	    audio_connection = (AgsAudioConnection *) list->data;
	    
	    g_object_get(G_OBJECT(audio_connection),
			 "data-object", &data_object,
			 NULL);
	    
	    if(AGS_IS_SOUNDCARD(data_object)){
	      ags_audio_remove_audio_connection(audio,
						(GObject *) audio_connection);

	      ags_connection_manager_remove_connection(connection_manager,
						       (AgsConnection *) audio_connection);
	      
	      break;
	    }

	    list = list->next;
	  }

	  pthread_mutex_unlock(audio_mutex);
	}
      }
    }
  }
}

/**
 * ags_panel_new:
 * @soundcard: the assigned soundcard.
 *
 * Creates an #AgsPanel
 *
 * Returns: a new #AgsPanel
 *
 * Since: 1.0.0
 */
AgsPanel*
ags_panel_new(GObject *soundcard)
{
  AgsPanel *panel;
  GValue value = {0,};

  panel = (AgsPanel *) g_object_new(AGS_TYPE_PANEL,
				    NULL);

  if(soundcard != NULL){
    g_value_init(&value, G_TYPE_OBJECT);
    g_value_set_object(&value, soundcard);
    g_object_set_property(G_OBJECT(AGS_MACHINE(panel)->audio),
			  "soundcard", &value);
    g_value_unset(&value);
  }

  return(panel);
}
