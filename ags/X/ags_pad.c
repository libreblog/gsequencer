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

#include <ags/X/ags_pad.h>
#include <ags/X/ags_pad_callbacks.h>

#include <ags/libags.h>
#include <ags/libags-audio.h>

#include <ags/X/ags_ui_provider.h>
#include <ags/X/ags_xorg_application_context.h>
#include <ags/X/ags_window.h>
#include <ags/X/ags_machine.h>

#include <ags/i18n.h>

void ags_pad_class_init(AgsPadClass *pad);
void ags_pad_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_pad_plugin_interface_init(AgsPluginInterface *plugin);
void ags_pad_init(AgsPad *pad);
void ags_pad_set_property(GObject *gobject,
			  guint prop_id,
			  const GValue *value,
			  GParamSpec *param_spec);
void ags_pad_get_property(GObject *gobject,
			  guint prop_id,
			  GValue *value,
			  GParamSpec *param_spec);

void ags_pad_connect(AgsConnectable *connectable);
void ags_pad_disconnect(AgsConnectable *connectable);

gchar* ags_pad_get_version(AgsPlugin *plugin);
void ags_pad_set_version(AgsPlugin *plugin, gchar *version);
gchar* ags_pad_get_build_id(AgsPlugin *plugin);
void ags_pad_set_build_id(AgsPlugin *plugin, gchar *build_id);

void ags_pad_real_set_channel(AgsPad *pad, AgsChannel *channel);
void ags_pad_real_resize_lines(AgsPad *pad, GType line_type,
			       guint audio_channels, guint audio_channels_old);
void ags_pad_real_map_recall(AgsPad *pad,
			     guint output_pad_start);
GList* ags_pad_real_find_port(AgsPad *pad);

/**
 * SECTION:ags_pad
 * @short_description: A composite widget to visualize a bunch of #AgsChannel
 * @title: AgsPad
 * @section_id:
 * @include: ags/X/ags_pad.h
 *
 * #AgsPad is a composite widget to visualize a bunch of #AgsChannel. It should be
 * packed by an #AgsMachine.
 */

enum{
  SAMPLERATE_CHANGED,
  BUFFER_SIZE_CHANGED,
  FORMAT_CHANGED,
  SET_CHANNEL,
  RESIZE_LINES,
  MAP_RECALL,
  FIND_PORT,
  LAST_SIGNAL,
};

enum{
  PROP_0,
  PROP_SAMPLERATE,
  PROP_BUFFER_SIZE,
  PROP_FORMAT,
  PROP_CHANNEL,
};

static gpointer ags_pad_parent_class = NULL;
static guint pad_signals[LAST_SIGNAL];

GType
ags_pad_get_type(void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_pad = 0;

    static const GTypeInfo ags_pad_info = {
      sizeof(AgsPadClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_pad_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsPad),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_pad_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_pad_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_pad_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_pad = g_type_register_static(GTK_TYPE_VBOX,
					  "AgsPad", &ags_pad_info,
					  0);

    g_type_add_interface_static(ags_type_pad,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_pad,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_pad);
  }

  return g_define_type_id__volatile;
}

void
ags_pad_class_init(AgsPadClass *pad)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;

  ags_pad_parent_class = g_type_class_peek_parent(pad);

  /* GObjectClass */
  gobject = G_OBJECT_CLASS(pad);

  gobject->set_property = ags_pad_set_property;
  gobject->get_property = ags_pad_get_property;

  //TODO:JK: add finalize

  /* properties */
  /**
   * AgsPad:samplerate:
   *
   * The samplerate.
   * 
   * Since: 2.1.35
   */
  param_spec = g_param_spec_uint("samplerate",
				 i18n_pspec("samplerate"),
				 i18n_pspec("The samplerate"),
				 0,
				 G_MAXUINT32,
				 AGS_SOUNDCARD_DEFAULT_SAMPLERATE,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_SAMPLERATE,
				  param_spec);

  /**
   * AgsPad:buffer-size:
   *
   * The buffer length.
   * 
   * Since: 2.1.35
   */
  param_spec = g_param_spec_uint("buffer-size",
				 i18n_pspec("buffer size"),
				 i18n_pspec("The buffer size"),
				 0,
				 G_MAXUINT32,
				 AGS_SOUNDCARD_DEFAULT_BUFFER_SIZE,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_BUFFER_SIZE,
				  param_spec);

  /**
   * AgsPad:format:
   *
   * The format.
   * 
   * Since: 2.1.35
   */
  param_spec = g_param_spec_uint("format",
				 i18n_pspec("format"),
				 i18n_pspec("The format"),
				 0,
				 G_MAXUINT32,
				 AGS_SOUNDCARD_DEFAULT_FORMAT,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_FORMAT,
				  param_spec);

  /**
   * AgsPad:channel:
   *
   * The start of a bunch of #AgsChannel to visualize.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("channel",
				   i18n_pspec("assigned channel"),
				   i18n_pspec("The channel it is assigned with"),
				   AGS_TYPE_CHANNEL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_CHANNEL,
				  param_spec);

  /* AgsPadClass */
  pad->samplerate_changed = NULL;
  pad->buffer_size_changed = NULL;
  pad->format_changed = NULL;

  pad->set_channel = ags_pad_real_set_channel;
  pad->resize_lines = ags_pad_real_resize_lines;
  pad->map_recall = ags_pad_real_map_recall;
  pad->find_port = ags_pad_real_find_port;

  /* signals */
  /**
   * AgsPad::samplerate-changed:
   * @pad: the #AgsPad
   * @samplerate: the samplerate
   * @old_samplerate: the old samplerate
   *
   * The ::samplerate-changed signal notifies about changed samplerate.
   * 
   * Since: 2.1.35
   */
  pad_signals[SAMPLERATE_CHANGED] =
    g_signal_new("samplerate-changed",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, samplerate_changed),
		 NULL, NULL,
		 ags_cclosure_marshal_VOID__UINT_UINT,
		 G_TYPE_NONE, 2,
		 G_TYPE_UINT,
		 G_TYPE_UINT);

  /**
   * AgsPad::buffer-size-changed:
   * @pad: the #AgsPad
   * @buffer_size: the buffer size
   * @old_buffer_size: the old buffer size
   *
   * The ::buffer-size-changed signal notifies about changed buffer size.
   * 
   * Since: 2.1.35
   */
  pad_signals[BUFFER_SIZE_CHANGED] =
    g_signal_new("buffer-size-changed",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, buffer_size_changed),
		 NULL, NULL,
		 ags_cclosure_marshal_VOID__UINT_UINT,
		 G_TYPE_NONE, 2,
		 G_TYPE_UINT,
		 G_TYPE_UINT);

  /**
   * AgsPad::format-changed:
   * @pad: the #AgsPad
   * @format: the format
   * @old_format: the old format
   *
   * The ::format-changed signal notifies about changed format.
   * 
   * Since: 2.1.35
   */
  pad_signals[FORMAT_CHANGED] =
    g_signal_new("format-changed",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, format_changed),
		 NULL, NULL,
		 ags_cclosure_marshal_VOID__UINT_UINT,
		 G_TYPE_NONE, 2,
		 G_TYPE_UINT,
		 G_TYPE_UINT);

  /**
   * AgsPad::set-channel:
   * @pad: the #AgsPad to modify
   * @channel: the #AgsChannel to set
   *
   * The ::set-channel signal notifies about changed channel.
   * 
   * Since: 2.0.0
   */
  pad_signals[SET_CHANNEL] =
    g_signal_new("set-channel",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, set_channel),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__OBJECT,
		 G_TYPE_NONE, 1,
		 G_TYPE_OBJECT);

  /**
   * AgsPad::resize-lines:
   * @pad: the #AgsPad to resize
   * @line_type: the channel type
   * @audio_channels: count of lines
   * @audio_channels_old: old count of lines
   *
   * The ::resize-lines is emitted as count of lines pack is modified.
   * 
   * Since: 2.0.0
   */
  pad_signals[RESIZE_LINES] =
    g_signal_new("resize-lines",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, resize_lines),
		 NULL, NULL,
		 ags_cclosure_marshal_VOID__ULONG_UINT_UINT,
		 G_TYPE_NONE, 3,
		 G_TYPE_ULONG, G_TYPE_UINT, G_TYPE_UINT);

  
  /**
   * AgsPad::map-recall:
   * @pad: the #AgsPad to resize
   * @output_pad_start: start of output pad
   *
   * The ::map-recall as recall should be mapped
   * 
   * Since: 2.0.0
   */
  pad_signals[MAP_RECALL] =
    g_signal_new("map-recall",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, map_recall),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__UINT,
		 G_TYPE_NONE, 1,
		 G_TYPE_UINT);

  /**
   * AgsPad::find-port:
   * @pad: the #AgsPad to resize
   *
   * The ::find-port retrieves all associated ports
   * 
   * Returns: a #GList-struct with associated ports
   *
   * Since: 2.0.0
   */
  pad_signals[FIND_PORT] =
    g_signal_new("find-port",
		 G_TYPE_FROM_CLASS(pad),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(AgsPadClass, find_port),
		 NULL, NULL,
		 ags_cclosure_marshal_POINTER__VOID,
		 G_TYPE_POINTER, 0);
}

void
ags_pad_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_pad_connect;
  connectable->disconnect = ags_pad_disconnect;
}

void
ags_pad_plugin_interface_init(AgsPluginInterface *plugin)
{
  plugin->get_name = NULL;
  plugin->set_name = NULL;
  plugin->get_version = ags_pad_get_version;
  plugin->set_version = ags_pad_set_version;
  plugin->get_build_id = ags_pad_get_build_id;
  plugin->set_build_id = ags_pad_set_build_id;
  plugin->get_xml_type = NULL;
  plugin->set_xml_type = NULL;
  plugin->get_ports = NULL;
  plugin->read = NULL;
  plugin->write = NULL;
  plugin->set_ports = NULL;
}

void
ags_pad_init(AgsPad *pad)
{
  GtkMenu *menu;
  GtkHBox *hbox;

  AgsConfig *config;

  pad->flags = 0;

  pad->name = NULL;

  pad->version = AGS_VERSION;
  pad->build_id = AGS_BUILD_ID;

  config = ags_config_get_instance();
  
  pad->samplerate = ags_soundcard_helper_config_get_samplerate(config);
  pad->buffer_size = ags_soundcard_helper_config_get_buffer_size(config);
  pad->format = ags_soundcard_helper_config_get_format(config);

  pad->channel = NULL;
  
  pad->cols = 2;

  pad->expander_set = ags_expander_set_new(1, 1);
  gtk_box_pack_start((GtkBox *) pad, (GtkWidget *) pad->expander_set, TRUE, TRUE, 0);

  hbox = (GtkHBox *) gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start((GtkBox *) pad, (GtkWidget *) hbox, FALSE, FALSE, 0);

  pad->group = (GtkToggleButton *) gtk_toggle_button_new_with_label("G");
  gtk_toggle_button_set_active(pad->group, TRUE);
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) pad->group, FALSE, FALSE, 0);

  pad->mute = (GtkToggleButton *) gtk_toggle_button_new_with_label("M");
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) pad->mute, FALSE, FALSE, 0);

  pad->solo = (GtkToggleButton *) gtk_toggle_button_new_with_label("S");
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) pad->solo, FALSE, FALSE, 0);

  pad->play = NULL;
}

void
ags_pad_set_property(GObject *gobject,
		     guint prop_id,
		     const GValue *value,
		     GParamSpec *param_spec)
{
  AgsPad *pad;

  pad = AGS_PAD(gobject);

  switch(prop_id){
  case PROP_SAMPLERATE:
    {
      GList *start_list, *list;

      guint samplerate, old_samplerate;
      
      samplerate = g_value_get_uint(value);
      old_samplerate = pad->samplerate;

      if(samplerate == old_samplerate){
	return;
      }

      pad->samplerate = samplerate;

      ags_pad_samplerate_changed(pad,
				 samplerate, old_samplerate);

      list =
	start_list = gtk_container_get_children(pad->expander_set);

      while(list != NULL){
	if(AGS_LINE(list->data)){
	  g_object_set(list->data,
		       "samplerate", samplerate,
		       NULL);
	}
	
	list = list->next;
      }

      g_list_free(start_list);
    }
    break;
  case PROP_BUFFER_SIZE:
    {
      GList *start_list, *list;

      guint buffer_size, old_buffer_size;
      
      buffer_size = g_value_get_uint(value);
      old_buffer_size = pad->buffer_size;

      if(buffer_size == old_buffer_size){
	return;
      }

      pad->buffer_size = buffer_size;

      ags_pad_buffer_size_changed(pad,
				  buffer_size, old_buffer_size);

      list =
	start_list = gtk_container_get_children(pad->expander_set);

      while(list != NULL){
	if(AGS_LINE(list->data)){
	  g_object_set(list->data,
		       "buffer-size", buffer_size,
		       NULL);
	}
	
	list = list->next;
      }

      g_list_free(start_list);
    }
    break;
  case PROP_FORMAT:
    {
      GList *start_list, *list;

      guint format, old_format;
      
      format = g_value_get_uint(value);
      old_format = pad->format;

      if(format == old_format){
	return;
      }

      pad->format = format;

      ags_pad_format_changed(pad,
			     format, old_format);

      list =
	start_list = gtk_container_get_children(pad->expander_set);

      while(list != NULL){
	if(AGS_LINE(list->data)){
	  g_object_set(list->data,
		       "format", format,
		       NULL);
	}
	
	list = list->next;
      }

      g_list_free(start_list);
    }
    break;
  case PROP_CHANNEL:
    {
      AgsChannel *channel;

      channel = (AgsChannel *) g_value_get_object(value);

      ags_pad_set_channel(pad, channel);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_pad_get_property(GObject *gobject,
		     guint prop_id,
		     GValue *value,
		     GParamSpec *param_spec)
{
  AgsPad *pad;

  pad = AGS_PAD(gobject);

  switch(prop_id){
  case PROP_SAMPLERATE:
    {
      g_value_set_uint(value,
		       pad->samplerate);
    }
    break;
  case PROP_BUFFER_SIZE:
    {
      g_value_set_uint(value,
		       pad->buffer_size);
    }
    break;
  case PROP_FORMAT:
    {
      g_value_set_uint(value,
		       pad->format);
    }
    break;
  case PROP_CHANNEL:
    {
      g_value_set_object(value, pad->channel);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_pad_connect(AgsConnectable *connectable)
{
  AgsPad *pad;
  GList *line_list, *line_list_start;

  /* AgsPad */
  pad = AGS_PAD(connectable);

  if((AGS_PAD_CONNECTED & (pad->flags)) != 0){
    return;
  }
  
  pad->flags |= AGS_PAD_CONNECTED;

  if((AGS_PAD_PREMAPPED_RECALL & (pad->flags)) == 0){
    if((AGS_PAD_MAPPED_RECALL & (pad->flags)) == 0){
      ags_pad_map_recall(pad,
			 0);
    }
  }else{
    pad->flags &= (~AGS_PAD_PREMAPPED_RECALL);

    ags_pad_find_port(pad);
  }

  /* GtkButton */
  g_signal_connect_after((GObject *) pad->group, "clicked",
			 G_CALLBACK(ags_pad_group_clicked_callback), (gpointer) pad);

  g_signal_connect_after((GObject *) pad->mute, "clicked",
			 G_CALLBACK(ags_pad_mute_clicked_callback), (gpointer) pad);

  g_signal_connect_after((GObject *) pad->solo, "clicked",
			 G_CALLBACK(ags_pad_solo_clicked_callback), (gpointer) pad);

  /* AgsLine */
  line_list_start =  
    line_list = gtk_container_get_children(GTK_CONTAINER(pad->expander_set));

  while(line_list != NULL){
    ags_connectable_connect(AGS_CONNECTABLE(line_list->data));

    line_list = line_list->next;
  }

  g_list_free(line_list_start);
}

void
ags_pad_disconnect(AgsConnectable *connectable)
{
  AgsPad *pad;
  GList *line_list, *line_list_start;

  /* AgsPad */
  pad = AGS_PAD(connectable);

  if((AGS_PAD_CONNECTED & (pad->flags)) == 0){
    return;
  }
  
  pad->flags &= (~AGS_PAD_CONNECTED);

  /* AgsLine */
  line_list_start =  
    line_list = gtk_container_get_children(GTK_CONTAINER(pad->expander_set));

  while(line_list != NULL){
    ags_connectable_disconnect(AGS_CONNECTABLE(line_list->data));

    line_list = line_list->next;
  }

  g_list_free(line_list_start);

  g_signal_handlers_disconnect_by_data(pad->channel,
				       pad);
}

gchar*
ags_pad_get_version(AgsPlugin *plugin)
{
  return(AGS_PAD(plugin)->version);
}

void
ags_pad_set_version(AgsPlugin *plugin, gchar *version)
{
  AgsPad *pad;

  pad = AGS_PAD(plugin);

  pad->version = version;
}

gchar*
ags_pad_get_build_id(AgsPlugin *plugin)
{
  return(AGS_PAD(plugin)->build_id);
}

void
ags_pad_set_build_id(AgsPlugin *plugin, gchar *build_id)
{
  AgsPad *pad;

  pad = AGS_PAD(plugin);

  pad->build_id = build_id;
}

/**
 * ags_pad_samplerate_changed:
 * @pad: the #AgsPad
 * @samplerate: the samplerate
 * @old_samplerate: the old samplerate
 * 
 * Notify about samplerate changed.
 * 
 * Since: 2.1.35
 */
void
ags_pad_samplerate_changed(AgsPad *pad,
			   guint samplerate, guint old_samplerate)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[SAMPLERATE_CHANGED], 0,
		samplerate,
		old_samplerate);
  g_object_unref((GObject *) pad);
}

/**
 * ags_pad_buffer_size_changed:
 * @pad: the #AgsPad
 * @buffer_size: the buffer_size
 * @old_buffer_size: the old buffer_size
 * 
 * Notify about buffer_size changed.
 * 
 * Since: 2.1.35
 */
void
ags_pad_buffer_size_changed(AgsPad *pad,
			    guint buffer_size, guint old_buffer_size)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[BUFFER_SIZE_CHANGED], 0,
		buffer_size,
		old_buffer_size);
  g_object_unref((GObject *) pad);
}

/**
 * ags_pad_format_changed:
 * @pad: the #AgsPad
 * @format: the format
 * @old_format: the old format
 * 
 * Notify about format changed.
 * 
 * Since: 2.1.35
 */
void
ags_pad_format_changed(AgsPad *pad,
		       guint format, guint old_format)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[FORMAT_CHANGED], 0,
		format,
		old_format);
  g_object_unref((GObject *) pad);
}

void
ags_pad_real_set_channel(AgsPad *pad, AgsChannel *channel)
{
  AgsChannel *current, *next_current;

  GList *line, *line_start;

  if(pad->channel == channel){
    return;
  }

  if(pad->channel != NULL){
    g_object_unref(G_OBJECT(pad->channel));
  }

  if(channel != NULL){
    g_object_ref(G_OBJECT(channel));
  }

  if(channel != NULL){
    pad->samplerate = channel->samplerate;
    pad->buffer_size = channel->buffer_size;
    pad->format = channel->format;
  }

  pad->channel = channel;

  line_start = 
    line = gtk_container_get_children(GTK_CONTAINER(AGS_PAD(pad)->expander_set));

  current = channel;

  if(current != NULL){
    g_object_ref(current);
  }

  next_current = NULL;
  
  /* set channel */
  while(line != NULL){
    g_object_set(G_OBJECT(line->data),
		 "channel", current,
		 NULL);

    /* iterate */
    if(current != NULL){
      next_current = ags_channel_next(current);

      g_object_unref(current);

      current = next_current;
    }
    
    line = line->next;
  }

  if(next_current != NULL){
    g_object_unref(next_current);
  }
  
  g_list_free(line_start);
}

/**
 * ags_pad_set_channel:
 * @pad: an #AgsPad
 * @channel: the #AgsChannel to set
 *
 * Is emitted as channel gets modified.
 *
 * Since: 2.0.0
 */
void
ags_pad_set_channel(AgsPad *pad, AgsChannel *channel)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[SET_CHANNEL], 0,
		channel);
  g_object_unref((GObject *) pad);
}

void
ags_pad_real_resize_lines(AgsPad *pad, GType line_type,
			  guint audio_channels, guint audio_channels_old)
{
  AgsLine *line;

  AgsChannel *channel, *next_channel;

  guint i, j;

#ifdef AGS_DEBUG
  g_message("ags_pad_real_resize_lines: audio_channels = %u ; audio_channels_old = %u\n", audio_channels, audio_channels_old);
#endif
  
  /* resize */
  if(audio_channels > audio_channels_old){
    channel = ags_channel_nth(pad->channel,
			      audio_channels_old);

    next_channel = NULL;
    
    /* create AgsLine */
    for(i = audio_channels_old; i < audio_channels;){
      for(j = audio_channels_old % pad->cols; j < pad->cols && i < audio_channels; j++, i++){
	/* instantiate line */
	line = (AgsLine *) g_object_new(line_type,
					"pad", pad,
					"channel", channel,
					NULL);

	if(channel != NULL){
	  channel->line_widget = (GObject *) line;
	}

	ags_expander_set_add(pad->expander_set,
			     (GtkWidget *) line,
			     j, floor(i / pad->cols),
			     1, 1);
	
	/* iterate */
	if(channel != NULL){
	  next_channel = ags_channel_next(channel);

	  g_object_unref(channel);

	  channel = next_channel;
	}
      }
    }

    if(next_channel != NULL){
      g_object_unref(next_channel);
    }
  }else if(audio_channels < audio_channels_old){
    GList *list, *list_start;

    list_start =
      list = g_list_nth(g_list_reverse(gtk_container_get_children(GTK_CONTAINER(pad->expander_set))),
			audio_channels);
    
    while(list != NULL){
      ags_connectable_disconnect(AGS_CONNECTABLE(list->data));

      list = list->next;
    }

    list = list_start;

    while(list != NULL){
      gtk_widget_destroy(GTK_WIDGET(list->data));

      list = list->next;
    }

    g_list_free(list_start);
  }
}

/**
 * ags_pad_resize_lines:
 * @pad: the #AgsPad to resize
 * @line_type: channel type, either %AGS_TYPE_INPUT or %AGS_TYPE_OUTPUT
 * @audio_channels: count of lines
 * @audio_channels_old: old count of lines
 *
 * Resize the count of #AgsLine packe by #AgsPad.
 *
 * Since: 2.0.0
 */
void
ags_pad_resize_lines(AgsPad *pad, GType line_type,
		     guint audio_channels, guint audio_channels_old)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  //  fprintf(stdout, "ags_pad_resize_lines: audio_channels = %u ; audio_channels_old = %u\n", audio_channels, audio_channels_old);

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[RESIZE_LINES], 0,
		line_type,
		audio_channels, audio_channels_old);
  g_object_unref((GObject *) pad);
}

void
ags_pad_real_map_recall(AgsPad *pad, guint output_pad_start)
{
  if((AGS_PAD_MAPPED_RECALL & (pad->flags)) != 0){
    return;
  }
  
  pad->flags |= AGS_PAD_MAPPED_RECALL;

  ags_pad_find_port(pad);
}

/**
 * ags_pad_map_recall:
 * @pad: the #AgsPad to resize
 * @output_pad_start: start of output pad
 *
 * Start of output pad
 *
 * Since: 2.0.0
 */
void
ags_pad_map_recall(AgsPad *pad, guint output_pad_start)
{
  g_return_if_fail(AGS_IS_PAD(pad));

  g_object_ref((GObject *) pad);
  g_signal_emit(G_OBJECT(pad),
		pad_signals[MAP_RECALL], 0,
		output_pad_start);
  g_object_unref((GObject *) pad);
}

GList*
ags_pad_real_find_port(AgsPad *pad)
{
  GList *line;
  
  GList *port, *tmp_port;

  port = NULL;

  /* find output ports */
  if(pad->expander_set != NULL){
    line = gtk_container_get_children((GtkContainer *) pad->expander_set);

    while(line != NULL){
      tmp_port = ags_line_find_port(AGS_LINE(line->data));
      
      if(port != NULL){
	port = g_list_concat(port,
			     tmp_port);
      }else{
	port = tmp_port;
      }

      line = line->next;
    }
  }

  return(port);
}

/**
 * ags_pad_find_port:
 * @pad: an #AgsPad
 *
 * Lookup ports of assigned recalls.
 *
 * Returns: an #GList containing all related #AgsPort
 *
 * Since: 2.0.0
 */
GList*
ags_pad_find_port(AgsPad *pad)
{
  GList *list;

  list = NULL;
  g_return_val_if_fail(AGS_IS_PAD(pad),
		       NULL);

  g_object_ref((GObject *) pad);
  g_signal_emit((GObject *) pad,
		pad_signals[FIND_PORT], 0,
		&list);
  g_object_unref((GObject *) pad);

  return(list);
}

void
ags_pad_play(AgsPad *pad)
{
  AgsWindow *window;
  AgsMachine *machine;

  AgsChannel *channel, *next_channel, *next_pad;

  AgsStartSoundcard *start_soundcard;
  AgsStartChannel *start_channel;

  AgsApplicationContext *application_context;
  
  GList *start_task;
  GList *start_list;
  
  gboolean no_soundcard;
  gboolean play_all;
  
  machine = (AgsMachine *) gtk_widget_get_ancestor((GtkWidget *) pad,
						   AGS_TYPE_MACHINE);
  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) machine);
  
  application_context = (AgsApplicationContext *) window->application_context;

  no_soundcard = FALSE;
  
  if((start_list = ags_sound_provider_get_soundcard(AGS_SOUND_PROVIDER(application_context))) == NULL){
    no_soundcard = TRUE;
  }

  g_list_free_full(start_list,
		   g_object_unref);

  if(no_soundcard){
    g_message("No soundcard available");
    
    return;
  }

  /*  */
  start_task = NULL;

  play_all = gtk_toggle_button_get_active(pad->group);
  
  if(gtk_toggle_button_get_active(pad->play)){
    if(play_all){
      channel = pad->channel;

      if(channel != NULL){
	g_object_ref(channel);
      }

      next_pad = ags_channel_next_pad(channel);
      next_channel = NULL;
      
      while(channel != next_pad){
	/* start channel for playback */
	start_channel = ags_start_channel_new(channel,
					      AGS_SOUND_SCOPE_PLAYBACK);
    
	g_signal_connect_after(G_OBJECT(start_channel), "launch",
			       G_CALLBACK(ags_pad_start_channel_launch_callback), pad);
	start_task = g_list_prepend(start_task,
				    start_channel);

	/* iterate */
	next_channel = ags_channel_next(channel);

	g_object_unref(channel);

	channel = next_channel;
      }

      /* unref */
      if(next_pad != NULL){
	g_object_unref(next_pad);
      }
      
      if(next_channel != NULL){
	g_object_unref(next_channel);
      }      
    }else{
      GList *list;

      list = gtk_container_get_children(GTK_CONTAINER(pad->expander_set));

      channel = pad->channel;
      
      /* start channel for playback */
      start_channel = ags_start_channel_new(channel,
					    AGS_SOUND_SCOPE_PLAYBACK);
    
      g_signal_connect_after(G_OBJECT(start_channel), "launch",
			     G_CALLBACK(ags_pad_start_channel_launch_callback), pad);
      start_task = g_list_prepend(start_task,
				  start_channel);

      g_list_free(list);
    }

    /* create start task */
    if(start_task != NULL){
      start_soundcard = ags_start_soundcard_new(application_context);
      start_task = g_list_prepend(start_task,
				  start_soundcard);

      /* append AgsStartSoundcard */
      start_task = g_list_reverse(start_task);

      ags_xorg_application_context_schedule_task_list(application_context,
						      start_task);
    }
  }else{
    AgsPlayback *playback;
    AgsRecallID *recall_id;
    
    AgsCancelChannel *cancel_channel;

    if(play_all){
      channel = pad->channel;

      if(channel != NULL){
	g_object_ref(channel);
      }

      next_pad = ags_channel_next_pad(channel);
      next_channel = NULL;

      /* cancel request */
      while(channel != next_pad){
	g_object_get(channel,
		     "playback", &playback,
		     NULL);
	
	recall_id = ags_playback_get_recall_id(playback,
					       AGS_SOUND_SCOPE_PLAYBACK);

	if(recall_id != NULL){
	  cancel_channel = ags_cancel_channel_new(channel,
						  AGS_SOUND_SCOPE_PLAYBACK);

	  ags_xorg_application_context_schedule_task(application_context,
						     (GObject *) cancel_channel);
	}

	g_object_unref(playback);
	
	/* iterate */
	next_channel = ags_channel_next(channel);

	g_object_unref(channel);

	channel = next_channel;
      }

      /* unref */
      if(next_pad != NULL){
	g_object_unref(next_pad);
      }
      
      if(next_channel != NULL){
	g_object_unref(next_channel);
      }      
    }else{
      AgsLine *line;
      
      GList *start_list, *list;

      start_list = gtk_container_get_children(GTK_CONTAINER(pad->expander_set));

      list = ags_line_find_next_grouped(start_list);

      if(list == NULL){	
	g_list_free(start_list);
	
	return;
      }else{
	line = AGS_LINE(list->data);
	
	g_list_free(start_list);
      }
      
      /*  */
      channel = line->channel;
      
      g_object_get(channel,
		   "playback", &playback,
		   NULL);
	
      recall_id = ags_playback_get_recall_id(playback,
					     AGS_SOUND_SCOPE_PLAYBACK);

      if(recall_id != NULL){
	/* cancel request */
	cancel_channel = ags_cancel_channel_new(channel,
						AGS_SOUND_SCOPE_PLAYBACK);

	ags_xorg_application_context_schedule_task(application_context,
						   (GObject *) cancel_channel);
      }

      g_object_unref(playback);
    }
  }
}
  
/**
 * ags_pad_new:
 * @channel: the bunch of channel to visualize
 *
 * Creates an #AgsPad
 *
 * Returns: a new #AgsPad
 *
 * Since: 2.0.0
 */
AgsPad*
ags_pad_new(AgsChannel *channel)
{
  AgsPad *pad;

  pad = (AgsPad *) g_object_new(AGS_TYPE_PAD, NULL);

  return(pad);
}
