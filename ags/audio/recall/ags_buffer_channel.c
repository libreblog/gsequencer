/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2017 Joël Krähemann
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

#include <ags/audio/recall/ags_buffer_channel.h>

#include <ags/libags.h>

#include <ags/plugin/ags_base_plugin.h>

#include <ags/i18n.h>

void ags_buffer_channel_class_init(AgsBufferChannelClass *buffer_channel);
void ags_buffer_channel_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_buffer_channel_mutable_interface_init(AgsMutableInterface *mutable);
void ags_buffer_channel_plugin_interface_init(AgsPluginInterface *plugin);
void ags_buffer_channel_init(AgsBufferChannel *buffer_channel);
void ags_buffer_channel_set_property(GObject *gobject,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *param_spec);
void ags_buffer_channel_get_property(GObject *gobject,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *param_spec);
void ags_buffer_channel_connect(AgsConnectable *connectable);
void ags_buffer_channel_disconnect(AgsConnectable *connectable);
void ags_buffer_channel_set_ports(AgsPlugin *plugin, GList *port);
void ags_buffer_channel_dispose(GObject *gobject);
void ags_buffer_channel_finalize(GObject *gobject);

void ags_buffer_channel_set_muted(AgsMutable *mutable, gboolean muted);

static AgsPortDescriptor* ags_buffer_channel_get_muted_port_descriptor();

/**
 * SECTION:ags_buffer_channel
 * @short_description: buffers channel
 * @title: AgsBufferChannel
 * @section_id:
 * @include: ags/audio/recall/ags_buffer_channel.h
 *
 * The #AgsBufferChannel class provides ports to the effect processor.
 */

enum{
  PROP_0,
  PROP_MUTED,
};

static gpointer ags_buffer_channel_parent_class = NULL;
static AgsConnectableInterface *ags_buffer_channel_parent_connectable_interface;
static AgsMutableInterface *ags_buffer_channel_parent_mutable_interface;
static AgsPluginInterface *ags_buffer_channel_parent_plugin_interface;

static const gchar *ags_buffer_channel_plugin_name = "ags-buffer";
static const gchar *ags_buffer_channel_plugin_specifier[] = {
  "./muted[0]",
};
static const gchar *ags_buffer_channel_plugin_control_port[] = {
  "1/1",
};

GType
ags_buffer_channel_get_type()
{
  static GType ags_type_buffer_channel = 0;

  if(!ags_type_buffer_channel){
    static const GTypeInfo ags_buffer_channel_info = {
      sizeof (AgsBufferChannelClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_buffer_channel_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsBufferChannel),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_buffer_channel_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_buffer_channel_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_mutable_interface_info = {
      (GInterfaceInitFunc) ags_buffer_channel_mutable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_plugin_interface_info = {
      (GInterfaceInitFunc) ags_buffer_channel_plugin_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_buffer_channel = g_type_register_static(AGS_TYPE_RECALL_CHANNEL,
						     "AgsBufferChannel",
						     &ags_buffer_channel_info,
						     0);

    g_type_add_interface_static(ags_type_buffer_channel,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_buffer_channel,
				AGS_TYPE_MUTABLE,
				&ags_mutable_interface_info);

    g_type_add_interface_static(ags_type_buffer_channel,
				AGS_TYPE_PLUGIN,
				&ags_plugin_interface_info);
  }

  return (ags_type_buffer_channel);
}

void
ags_buffer_channel_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_buffer_channel_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_buffer_channel_connect;
  connectable->disconnect = ags_buffer_channel_disconnect;
}

void
ags_buffer_channel_mutable_interface_init(AgsMutableInterface *mutable)
{
  ags_buffer_channel_parent_mutable_interface = g_type_interface_peek_parent(mutable);

  mutable->set_muted = ags_buffer_channel_set_muted;
}

void
ags_buffer_channel_plugin_interface_init(AgsPluginInterface *plugin)
{
  ags_buffer_channel_parent_plugin_interface = g_type_interface_peek_parent(plugin);

  plugin->set_ports = ags_buffer_channel_set_ports;
}

void
ags_buffer_channel_class_init(AgsBufferChannelClass *buffer_channel)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  GParamSpec *param_spec;

  ags_buffer_channel_parent_class = g_type_class_peek_parent(buffer_channel);

  /* GObjectClass */
  gobject = (GObjectClass *) buffer_channel;

  gobject->set_property = ags_buffer_channel_set_property;
  gobject->get_property = ags_buffer_channel_get_property;

  gobject->dispose = ags_buffer_channel_dispose;
  gobject->finalize = ags_buffer_channel_finalize;

  /* properties */
  /**
   * AgsBufferChannel:muted:
   *
   * The mute port.
   * 
   * Since: 1.0.0
   */
  param_spec = g_param_spec_object("muted",
				   i18n_pspec("mute channel"),
				   i18n_pspec("Mute the channel"),
				   AGS_TYPE_PORT,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_MUTED,
				  param_spec);
}

void
ags_buffer_channel_init(AgsBufferChannel *buffer_channel)
{
  GList *port;

  AGS_RECALL(buffer_channel)->name = "ags-buffer";
  AGS_RECALL(buffer_channel)->version = AGS_RECALL_DEFAULT_VERSION;
  AGS_RECALL(buffer_channel)->build_id = AGS_RECALL_DEFAULT_BUILD_ID;
  AGS_RECALL(buffer_channel)->xml_type = "ags-buffer-channel";

  port = NULL;

  buffer_channel->muted = g_object_new(AGS_TYPE_PORT,
				       "plugin-name", ags_buffer_channel_plugin_name,
				       "specifier", ags_buffer_channel_plugin_specifier[0],
				       "control-port", ags_buffer_channel_plugin_control_port[0],
				       "port-value-is-pointer", FALSE,
				       "port-value-type", G_TYPE_FLOAT,
				       "port-value-size", sizeof(gfloat),
				       "port-value-length", 1,
				       NULL);
  g_object_ref(buffer_channel->muted);
  buffer_channel->muted->port_value.ags_port_float = (float) FALSE;

  /* port descriptor */
  buffer_channel->muted->port_descriptor = ags_buffer_channel_get_muted_port_descriptor();

  /* add to port */
  port = g_list_prepend(port, buffer_channel->muted);
  g_object_ref(buffer_channel->muted);

  /* set port */
  AGS_RECALL(buffer_channel)->port = port;

}


void
ags_buffer_channel_set_property(GObject *gobject,
				guint prop_id,
				const GValue *value,
				GParamSpec *param_spec)
{
  AgsBufferChannel *buffer_channel;

  buffer_channel = AGS_BUFFER_CHANNEL(gobject);

  switch(prop_id){
  case PROP_MUTED:
    {
      AgsPort *port;

      port = (AgsPort *) g_value_get_object(value);

      if(port == buffer_channel->muted){
	return;
      }

      if(buffer_channel->muted != NULL){
	g_object_unref(G_OBJECT(buffer_channel->muted));
      }
      
      if(port != NULL){
	g_object_ref(G_OBJECT(port));
      }

      buffer_channel->muted = port;
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }  
}

void
ags_buffer_channel_get_property(GObject *gobject,
				guint prop_id,
				GValue *value,
				GParamSpec *param_spec)
{
  AgsBufferChannel *buffer_channel;

  buffer_channel = AGS_BUFFER_CHANNEL(gobject);

  switch(prop_id){
  case PROP_MUTED:
    {
      g_value_set_object(value, buffer_channel->muted);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_buffer_channel_dispose(GObject *gobject)
{
  AgsBufferChannel *buffer_channel;

  buffer_channel = AGS_BUFFER_CHANNEL(gobject);

  if(buffer_channel->muted != NULL){
    g_object_unref(G_OBJECT(buffer_channel->muted));

    buffer_channel->muted = NULL;
  }

  /* call parent */
  G_OBJECT_CLASS(ags_buffer_channel_parent_class)->dispose(gobject);
}

void
ags_buffer_channel_finalize(GObject *gobject)
{
  AgsBufferChannel *buffer_channel;

  buffer_channel = AGS_BUFFER_CHANNEL(gobject);

  if(buffer_channel->muted != NULL){
    g_object_unref(G_OBJECT(buffer_channel->muted));
  }

  /* call parent */
  G_OBJECT_CLASS(ags_buffer_channel_parent_class)->finalize(gobject);
}

void
ags_buffer_channel_connect(AgsConnectable *connectable)
{
  AgsRecall *recall;
  
  recall = AGS_RECALL(connectable);
  
  if((AGS_RECALL_CONNECTED & (recall->flags)) != 0){
    return;
  }

  /* load automation */
  ags_recall_load_automation(recall,
			     g_list_copy(recall->port));

  /* call parent */
  ags_buffer_channel_parent_connectable_interface->connect(connectable);
}

void
ags_buffer_channel_disconnect(AgsConnectable *connectable)
{
  ags_buffer_channel_parent_connectable_interface->disconnect(connectable);

  /* empty */
}

void
ags_buffer_channel_set_ports(AgsPlugin *plugin, GList *port)
{
  while(port != NULL){
    if(!strncmp(AGS_PORT(port->data)->specifier,
		"./muted[0]",
		9)){
      g_object_set(G_OBJECT(plugin),
		   "muted", AGS_PORT(port->data),
		   NULL);
    }

    port = port->next;
  }
}

void
ags_buffer_channel_set_muted(AgsMutable *mutable, gboolean muted)
{
  GValue value = {0,};

  g_value_init(&value, G_TYPE_FLOAT);
  g_value_set_float(&value, (float) muted);

  ags_port_safe_write(AGS_BUFFER_CHANNEL(mutable)->muted, &value);
}

static AgsPortDescriptor*
ags_buffer_channel_get_muted_port_descriptor()
{
  static AgsPortDescriptor *port_descriptor = NULL;

  if(port_descriptor == NULL){
    port_descriptor = ags_port_descriptor_alloc();

    port_descriptor->flags |= (AGS_PORT_DESCRIPTOR_INPUT |
			       AGS_PORT_DESCRIPTOR_CONTROL |
			       AGS_PORT_DESCRIPTOR_TOGGLED);

    port_descriptor->port_index = 0;

    /* range */
    g_value_init(port_descriptor->default_value,
		 G_TYPE_FLOAT);
    g_value_init(port_descriptor->lower_value,
		 G_TYPE_FLOAT);
    g_value_init(port_descriptor->upper_value,
		 G_TYPE_FLOAT);

    g_value_set_float(port_descriptor->default_value,
		      0.0);
    g_value_set_float(port_descriptor->lower_value,
		      0.0);
    g_value_set_float(port_descriptor->upper_value,
		      1.0);
  }
  
  return(port_descriptor);
}

/**
 * ags_buffer_channel_new:
 *
 * Creates an #AgsBufferChannel
 *
 * Returns: a new #AgsBufferChannel
 *
 * Since: 1.0.0
 */
AgsBufferChannel*
ags_buffer_channel_new()
{
  AgsBufferChannel *buffer_channel;

  buffer_channel = (AgsBufferChannel *) g_object_new(AGS_TYPE_BUFFER_CHANNEL,
						     NULL);

  return(buffer_channel);
}
