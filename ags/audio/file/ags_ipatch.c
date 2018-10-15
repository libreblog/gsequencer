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

#include <ags/audio/file/ags_ipatch.h>

#include <ags/libags.h>

#include <ags/audio/ags_audio_buffer_util.h>

#include <ags/audio/file/ags_sound_container.h>
#include <ags/audio/file/ags_sound_resource.h>
#include <ags/audio/file/ags_ipatch_sf2_reader.h>
#include <ags/audio/file/ags_ipatch_dls2_reader.h>
#include <ags/audio/file/ags_ipatch_gig_reader.h>
#include <ags/audio/file/ags_ipatch_sample.h>

#include <ags/i18n.h>

void ags_ipatch_class_init(AgsIpatchClass *ipatch);
void ags_ipatch_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_ipatch_sound_container_interface_init(AgsSoundContainerInterface *sound_container);
void ags_ipatch_init(AgsIpatch *ipatch);
void ags_ipatch_set_property(GObject *gobject,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *param_spec);
void ags_ipatch_get_property(GObject *gobject,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *param_spec);
void ags_ipatch_finalize(GObject *gobject);

AgsUUID* ags_ipatch_get_uuid(AgsConnectable *connectable);
gboolean ags_ipatch_has_resource(AgsConnectable *connectable);
gboolean ags_ipatch_is_ready(AgsConnectable *connectable);
void ags_ipatch_add_to_registry(AgsConnectable *connectable);
void ags_ipatch_remove_from_registry(AgsConnectable *connectable);
xmlNode* ags_ipatch_list_resource(AgsConnectable *connectable);
xmlNode* ags_ipatch_xml_compose(AgsConnectable *connectable);
void ags_ipatch_xml_parse(AgsConnectable *connectable,
			  xmlNode *node);
gboolean ags_ipatch_is_connected(AgsConnectable *connectable);
void ags_ipatch_connect(AgsConnectable *connectable);
void ags_ipatch_disconnect(AgsConnectable *connectable);

gboolean ags_ipatch_open(AgsSoundContainer *sound_container, gchar *filename);
guint ags_ipatch_get_level_count(AgsSoundContainer *sound_container);
guint ags_ipatch_get_nesting_level(AgsSoundContainer *sound_container);
gchar* ags_ipatch_get_level_id(AgsSoundContainer *sound_container);
guint ags_ipatch_get_level_index(AgsSoundContainer *sound_container);
guint ags_ipatch_level_up(AgsSoundContainer *sound_container,
			  guint level_count);
guint ags_ipatch_select_level_by_id(AgsSoundContainer *sound_container,
				    gchar *level_id);
guint ags_ipatch_select_level_by_index(AgsSoundContainer *sound_container,
				       guint level_index);
gchar** ags_ipatch_get_sublevel_name(AgsSoundContainer *sound_container);
GList* ags_ipatch_get_resource_all(AgsSoundContainer *sound_container);
GList* ags_ipatch_get_resource_by_name(AgsSoundContainer *sound_container,
				       gchar *resource_name);
GList* ags_ipatch_get_resource_by_index(AgsSoundContainer *sound_container,
					guint resource_index);
GList* ags_ipatch_get_resource_current(AgsSoundContainer *sound_container);
void ags_ipatch_close(AgsSoundContainer *sound_container);

/**
 * SECTION:ags_ipatch
 * @short_description: Libinstpatch wrapper
 * @title: AgsIpatch
 * @section_id:
 * @include: ags/audio/file/ags_ipatch.h
 *
 * #AgsIpatch is the base object to ineract with libinstpatch.
 */
 
enum{
  PROP_0,
  PROP_SOUNDCARD,
  PROP_FILENAME,
  PROP_MODE,
};

static gpointer ags_ipatch_parent_class = NULL;

static pthread_mutex_t ags_ipatch_class_mutex = PTHREAD_MUTEX_INITIALIZER;

GType
ags_ipatch_get_type()
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_ipatch = 0;

    static const GTypeInfo ags_ipatch_info = {
      sizeof (AgsIpatchClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_ipatch_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsIpatch),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_ipatch_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_ipatch_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_sound_container_interface_info = {
      (GInterfaceInitFunc) ags_ipatch_sound_container_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_ipatch = g_type_register_static(G_TYPE_OBJECT,
					      "AgsIpatch",
					      &ags_ipatch_info,
					      0);

    g_type_add_interface_static(ags_type_ipatch,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_ipatch,
				AGS_TYPE_SOUND_CONTAINER,
				&ags_sound_container_interface_info);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_ipatch);
  }

  return g_define_type_id__volatile;
}

void
ags_ipatch_class_init(AgsIpatchClass *ipatch)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;

  ags_ipatch_parent_class = g_type_class_peek_parent(ipatch);

  /* GObjectClass */
  gobject = (GObjectClass *) ipatch;

  gobject->set_property = ags_ipatch_set_property;
  gobject->get_property = ags_ipatch_get_property;

  gobject->finalize = ags_ipatch_finalize;

  /* properties */
  /**
   * AgsIpatch:soundcard:
   *
   * The assigned soundcard.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("soundcard",
				   i18n_pspec("soundcard of ipatch"),
				   i18n_pspec("The soundcard what ipatch has it's presets"),
				   G_TYPE_OBJECT,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_SOUNDCARD,
				  param_spec);

  /**
   * AgsIpatch:filename:
   *
   * The assigned filename.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_string("filename",
				   i18n_pspec("the filename"),
				   i18n_pspec("The filename to open"),
				   NULL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_FILENAME,
				  param_spec);

  /**
   * AgsIpatch:mode:
   *
   * The assigned mode.
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_string("mode",
				   i18n_pspec("the mode"),
				   i18n_pspec("The mode to open the file"),
				   NULL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_MODE,
				  param_spec);
}

void
ags_ipatch_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->get_uuid = ags_ipatch_get_uuid;
  connectable->has_resource = ags_ipatch_has_resource;
  connectable->is_ready = ags_ipatch_is_ready;

  connectable->add_to_registry = ags_ipatch_add_to_registry;
  connectable->remove_from_registry = ags_ipatch_remove_from_registry;

  connectable->list_resource = ags_ipatch_list_resource;
  connectable->xml_compose = ags_ipatch_xml_compose;
  connectable->xml_parse = ags_ipatch_xml_parse;

  connectable->is_connected = ags_ipatch_is_connected;
  
  connectable->connect = ags_ipatch_connect;
  connectable->disconnect = ags_ipatch_disconnect;

  connectable->connect_connection = NULL;
  connectable->disconnect_connection = NULL;
}

void
ags_ipatch_sound_container_interface_init(AgsSoundContainerInterface *sound_container)
{
  sound_container->open = ags_ipatch_open;
  
  sound_container->get_level_count = ags_ipatch_get_level_count;
  sound_container->get_nesting_level = ags_ipatch_get_nesting_level;
  
  sound_container->get_level_id = ags_ipatch_get_level_id;
  sound_container->get_level_index = ags_ipatch_get_level_index;
  
  sound_container->get_sublevel_name = ags_ipatch_get_sublevel_name;
  
  sound_container->level_up = ags_ipatch_level_up;
  sound_container->select_level_by_id = ags_ipatch_select_level_by_id;
  sound_container->select_level_by_index = ags_ipatch_select_level_by_index;
  
  sound_container->get_resource_all = ags_ipatch_get_resource_all;
  sound_container->get_resource_by_name = ags_ipatch_get_resource_by_name;
  sound_container->get_resource_by_index = ags_ipatch_get_resource_by_index;
  sound_container->get_resource_current = ags_ipatch_get_resource_current;
  
  sound_container->close = ags_ipatch_close;
}

void
ags_ipatch_init(AgsIpatch *ipatch)
{
  pthread_mutex_t *mutex;
  pthread_mutexattr_t *attr;

  ipatch->flags = 0;

  /* add audio file mutex */
  ipatch->obj_mutexattr = 
    attr = (pthread_mutexattr_t *) malloc(sizeof(pthread_mutexattr_t));
  pthread_mutexattr_init(attr);
  pthread_mutexattr_settype(attr,
			    PTHREAD_MUTEX_RECURSIVE);

#ifdef __linux__
  pthread_mutexattr_setprotocol(attr,
				PTHREAD_PRIO_INHERIT);
#endif

  ipatch->obj_mutex = 
    mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex,
		     attr);  

  /* uuid */
  ipatch->uuid = ags_uuid_alloc();
  ags_uuid_generate(ipatch->uuid);

  ipatch->soundcard = NULL;

  ipatch->filename = NULL;
  ipatch->mode = AGS_IPATCH_READ;

  ipatch->file = NULL;
  ipatch->handle = NULL;

  ipatch->nesting_level = 0;

  ipatch->level_id = NULL;
  ipatch->level_index = 0;

  ipatch->reader = NULL;
  ipatch->writer = NULL;

  ipatch->audio_signal= NULL;
}

void
ags_ipatch_set_property(GObject *gobject,
			guint prop_id,
			const GValue *value,
			GParamSpec *param_spec)
{
  AgsIpatch *ipatch;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(gobject);

  /* get audio file mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  switch(prop_id){
  case PROP_SOUNDCARD:
    {
      GObject *soundcard;
      
      soundcard = (GObject *) g_value_get_object(value);

      pthread_mutex_lock(ipatch_mutex);

      if(soundcard == ((GObject *) ipatch->soundcard)){
	pthread_mutex_unlock(ipatch_mutex);

	return;
      }

      if(ipatch->soundcard != NULL){
	g_object_unref(ipatch->soundcard);
      }
      
      if(soundcard != NULL){
	g_object_ref(G_OBJECT(soundcard));
      }
      
      ipatch->soundcard = (GObject *) soundcard;

      pthread_mutex_unlock(ipatch_mutex);
    }
    break;
  case PROP_FILENAME:
    {
      gchar *filename;

      filename = (gchar *) g_value_get_string(value);

      ags_sound_container_open(AGS_SOUND_CONTAINER(ipatch), filename);
    }
    break;
  case PROP_MODE:
    {
#ifdef AGS_WITH_LIBINSTPATCH      
      IpatchFileHandle *handle;
#else
      gpointer handle;
#endif

      gchar *mode;
      
      mode = (gchar *) g_value_get_string(value);

      pthread_mutex_lock(ipatch_mutex);
      
      ipatch->mode = mode;

      handle = ipatch->handle;
      
      pthread_mutex_lock(ipatch_mutex);

#ifdef AGS_WITH_LIBINSTPATCH      
      if(handle != NULL){
	GError *error;

	error = NULL;

	ipatch_file_default_open_method(handle,
					mode,
					&error);

	if(error != NULL){
	  g_warning("%s", error->message);
	}
      }
#endif
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_ipatch_get_property(GObject *gobject,
			guint prop_id,
			GValue *value,
			GParamSpec *param_spec)
{
  AgsIpatch *ipatch;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(gobject);

  /* get audio file mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  switch(prop_id){
  case PROP_SOUNDCARD:
    {
      pthread_mutex_lock(ipatch_mutex);

      g_value_set_object(value, ipatch->soundcard);

      pthread_mutex_unlock(ipatch_mutex);
    }
    break;
  case PROP_FILENAME:
    {
      pthread_mutex_lock(ipatch_mutex);

      g_value_set_string(value, ipatch->filename);

      pthread_mutex_unlock(ipatch_mutex);
    }
    break;
  case PROP_MODE:
    {
      pthread_mutex_lock(ipatch_mutex);

      g_value_set_string(value, ipatch->mode);

      pthread_mutex_unlock(ipatch_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_ipatch_finalize(GObject *gobject)
{
  AgsIpatch *ipatch;

  ipatch = AGS_IPATCH(gobject);

  pthread_mutex_destroy(ipatch->obj_mutex);
  free(ipatch->obj_mutex);

  pthread_mutexattr_destroy(ipatch->obj_mutexattr);
  free(ipatch->obj_mutexattr);
  
  if(ipatch->soundcard != NULL){
    g_object_unref(ipatch->soundcard);
  }

  g_free(ipatch->filename);
  g_free(ipatch->mode);

  if(ipatch->reader != NULL){
    g_object_unref(ipatch->reader);
  }

  g_list_free_full(ipatch->audio_signal,
		   g_object_unref);
    
  /* call parent */
  G_OBJECT_CLASS(ags_ipatch_parent_class)->finalize(gobject);
}

AgsUUID*
ags_ipatch_get_uuid(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;
  
  AgsUUID *ptr;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(connectable);

  /* get audio file mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* get UUID */
  pthread_mutex_lock(ipatch_mutex);

  ptr = ipatch->uuid;

  pthread_mutex_unlock(ipatch_mutex);
  
  return(ptr);
}

gboolean
ags_ipatch_has_resource(AgsConnectable *connectable)
{
  return(TRUE);
}

gboolean
ags_ipatch_is_ready(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;
  
  gboolean is_ready;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(connectable);

  /* get audio file mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* check is ready */
  pthread_mutex_lock(ipatch_mutex);
  
  is_ready = (((AGS_IPATCH_ADDED_TO_REGISTRY & (ipatch->flags)) != 0) ? TRUE: FALSE);
  
  pthread_mutex_unlock(ipatch_mutex);

  return(is_ready);
}

void
ags_ipatch_add_to_registry(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;

  AgsRegistry *registry;
  AgsRegistryEntry *entry;

  AgsApplicationContext *application_context;

  if(ags_connectable_is_ready(connectable)){
    return;
  }

  ipatch = AGS_IPATCH(connectable);

  ags_ipatch_set_flags(ipatch, AGS_IPATCH_ADDED_TO_REGISTRY);

  application_context = ags_application_context_get_instance();

  registry = ags_service_provider_get_registry(AGS_SERVICE_PROVIDER(application_context));

  if(registry != NULL){
    entry = ags_registry_entry_alloc(registry);
    g_value_set_object(&(entry->entry),
		       (gpointer) ipatch);
    ags_registry_add_entry(registry,
			   entry);
  }  
}

void
ags_ipatch_remove_from_registry(AgsConnectable *connectable)
{
  if(!ags_connectable_is_ready(connectable)){
    return;
  }

  //TODO:JK: implement me
}

xmlNode*
ags_ipatch_list_resource(AgsConnectable *connectable)
{
  xmlNode *node;
  
  node = NULL;

  //TODO:JK: implement me
  
  return(node);
}

xmlNode*
ags_ipatch_xml_compose(AgsConnectable *connectable)
{
  xmlNode *node;
  
  node = NULL;

  //TODO:JK: implement me
  
  return(node);
}

void
ags_ipatch_xml_parse(AgsConnectable *connectable,
			      xmlNode *node)
{
  //TODO:JK: implement me  
}

gboolean
ags_ipatch_is_connected(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;
  
  gboolean is_connected;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(connectable);

  /* get audio file mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* check is connected */
  pthread_mutex_lock(ipatch_mutex);

  is_connected = (((AGS_IPATCH_CONNECTED & (ipatch->flags)) != 0) ? TRUE: FALSE);
  
  pthread_mutex_unlock(ipatch_mutex);

  return(is_connected);
}

void
ags_ipatch_connect(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;

  if(ags_connectable_is_connected(connectable)){
    return;
  }

  ipatch = AGS_IPATCH(connectable);
  
  ags_ipatch_set_flags(ipatch, AGS_IPATCH_CONNECTED);
}

void
ags_ipatch_disconnect(AgsConnectable *connectable)
{
  AgsIpatch *ipatch;

  if(!ags_connectable_is_connected(connectable)){
    return;
  }

  ipatch = AGS_IPATCH(connectable);

  ags_ipatch_unset_flags(ipatch, AGS_IPATCH_CONNECTED);
}

/**
 * ags_ipatch_get_class_mutex:
 * 
 * Use this function's returned mutex to access mutex fields.
 *
 * Returns: the class mutex
 * 
 * Since: 2.0.36
 */
pthread_mutex_t*
ags_ipatch_get_class_mutex()
{
  return(&ags_ipatch_class_mutex);
}

/**
 * ags_ipatch_test_flags:
 * @ipatch: the #AgsIpatch
 * @flags: the flags
 *
 * Test @flags to be set on @ipatch.
 * 
 * Returns: %TRUE if flags are set, else %FALSE
 *
 * Since: 2.0.36
 */
gboolean
ags_ipatch_test_flags(AgsIpatch *ipatch, guint flags)
{
  gboolean retval;  
  
  pthread_mutex_t *ipatch_mutex;

  if(!AGS_IS_IPATCH(ipatch)){
    return(FALSE);
  }

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* test */
  pthread_mutex_lock(ipatch_mutex);

  retval = (flags & (ipatch->flags)) ? TRUE: FALSE;
  
  pthread_mutex_unlock(ipatch_mutex);

  return(retval);
}

/**
 * ags_ipatch_set_flags:
 * @ipatch: the #AgsIpatch
 * @flags: see #AgsIpatchFlags-enum
 *
 * Enable a feature of @ipatch.
 *
 * Since: 2.0.36
 */
void
ags_ipatch_set_flags(AgsIpatch *ipatch, guint flags)
{
  pthread_mutex_t *ipatch_mutex;

  if(!AGS_IS_IPATCH(ipatch)){
    return;
  }

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  //TODO:JK: add more?

  /* set flags */
  pthread_mutex_lock(ipatch_mutex);

  ipatch->flags |= flags;
  
  pthread_mutex_unlock(ipatch_mutex);
}
    
/**
 * ags_ipatch_unset_flags:
 * @ipatch: the #AgsIpatch
 * @flags: see #AgsIpatchFlags-enum
 *
 * Disable a feature of @ipatch.
 *
 * Since: 2.0.36
 */
void
ags_ipatch_unset_flags(AgsIpatch *ipatch, guint flags)
{  
  pthread_mutex_t *ipatch_mutex;

  if(!AGS_IS_IPATCH(ipatch)){
    return;
  }

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  //TODO:JK: add more?

  /* unset flags */
  pthread_mutex_lock(ipatch_mutex);

  ipatch->flags &= (~flags);
  
  pthread_mutex_unlock(ipatch_mutex);
}

gboolean
ags_ipatch_open(AgsSoundContainer *sound_container, gchar *filename)
{
  AgsIpatch *ipatch;

#ifdef AGS_WITH_LIBINSTPATCH
  IpatchFileHandle *handle;
  IpatchFileIOFuncs *io_funcs;
#endif

  GObject *reader;
  
  gchar *old_filename;
  
  gboolean retval;
  
  GError *error;

  pthread_mutex_t *ipatch_mutex;

  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* get some fields */
  pthread_mutex_lock(ipatch_mutex);

  old_filename = ipatch->filename;
  
  pthread_mutex_unlock(ipatch_mutex);

  /* close current */
  if(old_filename != NULL){
    ags_sound_container_close(sound_container);
    
    g_free(old_filename);
  }

  /* check suffix */
  ipatch->filename = g_strdup(filename);
  
  if(!ags_ipatch_check_suffix(filename)){
    return(FALSE);
  }
  
  error = NULL;

#ifdef AGS_WITH_LIBINSTPATCH
  handle = ipatch_file_identify_open(filename,
				     &error);

  pthread_mutex_lock(ipatch_mutex);

  ipatch->handle = handle;

  pthread_mutex_unlock(ipatch_mutex);
#endif
  
  if(error != NULL){
    g_warning("%s", error->message);
  }

  if(handle == NULL){
    return(FALSE);
  }

  reader = NULL;
  retval = FALSE;
  
  if(IPATCH_IS_DLS_FILE(handle->file)){
    ags_ipatch_set_flags(ipatch, AGS_IPATCH_DLS2);

    /* dls2 */
    reader = (GObject *) ags_ipatch_dls2_reader_new(ipatch);

    if(ags_ipatch_dls2_reader_load(reader,
				   handle)){
      retval = TRUE;
    }    
  }else if(IPATCH_IS_SF2_FILE(handle->file)){
    ags_ipatch_set_flags(ipatch, AGS_IPATCH_SF2);

    /* sf2 */
    reader = (GObject *) ags_ipatch_sf2_reader_new(ipatch);

    if(ags_ipatch_sf2_reader_load(reader,
				  handle)){
      retval = TRUE;
    }
  }else if(IPATCH_IS_GIG_FILE(handle->file)){
    ags_ipatch_set_flags(ipatch, AGS_IPATCH_GIG);

    /* gig */
    reader = (GObject *) ags_ipatch_gig_reader_new(ipatch);

    if(ags_ipatch_gig_reader_load(reader,
				  handle)){
      retval = TRUE;
    }
  }

  pthread_mutex_lock(ipatch_mutex);

  ipatch->reader = reader;

  pthread_mutex_unlock(ipatch_mutex);

  return(retval);
}

guint
ags_ipatch_get_level_count(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;

  ipatch = AGS_IPATCH(sound_container);

  if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_DLS2)){
    return(3);
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_SF2)){
    return(4);
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_GIG)){
    return(3);
  }

  return(0);
}

guint
ags_ipatch_get_nesting_level(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;

  guint nesting_level;

  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* get nesting level */
  pthread_mutex_lock(ipatch_mutex);

  nesting_level = ipatch->nesting_level;

  pthread_mutex_unlock(ipatch_mutex);
  
  return(nesting_level);
}

gchar*
ags_ipatch_get_level_id(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;

  gchar *level_id;

  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* get level id */
  pthread_mutex_lock(ipatch_mutex);

  level_id = ipatch->level_id;

  pthread_mutex_unlock(ipatch_mutex);
  
  return(level_id);
}

guint
ags_ipatch_get_level_index(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;

  guint level_index;

  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());
  
  /* get nesting level */
  pthread_mutex_lock(ipatch_mutex);

  level_index = ipatch->level_index;

  pthread_mutex_unlock(ipatch_mutex);
  
  return(level_index);
}

gchar**
ags_ipatch_get_sublevel_name(AgsSoundContainer *sound_container)
{  
  AgsIpatch *ipatch;

  guint sublevel;

  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* sublevel */
  sublevel = ags_ipatch_get_nesting_level(ipatch);
  
#ifdef AGS_WITH_LIBINSTPATCH  
  if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_DLS2)){
    AgsIpatchDLS2Reader *ipatch_dls2_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_dls2_reader = AGS_IPATCH_DLS2_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);

    switch(sublevel){
    case AGS_DLS2_FILENAME:
      {
	gchar **sublevel_name;
	
	sublevel_name = (gchar **) malloc(2 * sizeof(gchar*));

	sublevel_name[0] = ipatch->filename;
	sublevel_name[1] = NULL;

	return(sublevel_name);
      }
    case AGS_DLS2_IHDR:
      {
	return(ags_ipatch_dls2_reader_get_instrument_all(ipatch_dls2_reader));
      }
    case AGS_DLS2_SHDR:
      {
	return(ags_ipatch_dls2_reader_get_sample_by_instrument_index(ipatch_dls2_reader,
								     ipatch_dls2_reader->index_selected[1]));
      }
    };
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_SF2)){
    AgsIpatchSF2Reader *ipatch_sf2_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_sf2_reader = AGS_IPATCH_SF2_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);
    
    switch(sublevel){
    case AGS_SF2_FILENAME:
      {
	gchar **sublevel_name;
	
	sublevel_name = (gchar **) malloc(2 * sizeof(gchar*));

	sublevel_name[0] = ipatch->filename;
	sublevel_name[1] = NULL;

	return(sublevel_name);
      }
    case AGS_SF2_PHDR:
      {
	return(ags_ipatch_sf2_reader_get_preset_all(ipatch_sf2_reader));
      }
    case AGS_SF2_IHDR:
      {
	return(ags_ipatch_sf2_reader_get_instrument_by_preset_index(ipatch_sf2_reader,
								    ipatch_sf2_reader->index_selected[1]));
      }
    case AGS_SF2_SHDR:
      {
	return(ags_ipatch_sf2_reader_get_sample_by_preset_and_instrument_index(ipatch_sf2_reader,
									       ipatch_sf2_reader->index_selected[1], ipatch_sf2_reader->index_selected[2]));
      }
    };
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_GIG)){
    AgsIpatchGigReader *ipatch_gig_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_gig_reader = AGS_IPATCH_GIG_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);

    switch(sublevel){
    case AGS_GIG_FILENAME:
      {
	gchar **sublevel_name;
	
	sublevel_name = (gchar **) malloc(2 * sizeof(gchar*));

	sublevel_name[0] = ipatch->filename;
	sublevel_name[1] = NULL;

	return(sublevel_name);
      }
    case AGS_GIG_IHDR:
      {
	return(ags_ipatch_gig_reader_get_instrument_all(ipatch_gig_reader));
      }
    case AGS_GIG_SHDR:
      {
	return(ags_ipatch_gig_reader_get_sample_by_instrument_index(ipatch_gig_reader,
								    ipatch_gig_reader->index_selected[1]));
      }
    };
  }
#endif
  
  return(NULL);
}

guint
ags_ipatch_level_up(AgsSoundContainer *sound_container,
		    guint level_count)
{
  AgsIpatch *ipatch;
  
  guint retval;

  pthread_mutex_t *ipatch_mutex;
  
  if(level_count == 0){
    return(0);
  }

  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* check boundaries */
  if(ags_ipatch_get_nesting_level(ipatch) >= level_count){
    /* level up */
    pthread_mutex_lock(ipatch_mutex);

    retval = level_count;

    ipatch->nesting_level -= level_count;

    pthread_mutex_unlock(ipatch_mutex);
  }else{
    /* level up */
    pthread_mutex_lock(ipatch_mutex);

    retval = ipatch->nesting_level;
    
    ipatch->nesting_level = 0;

    pthread_mutex_unlock(ipatch_mutex);
  }

  return(retval);
}

guint
ags_ipatch_select_level_by_id(AgsSoundContainer *sound_container,
			      gchar *level_id)
{
  return(0);
}

guint
ags_ipatch_select_level_by_index(AgsSoundContainer *sound_container,
				 guint level_index)
{
  AgsIpatch *ipatch;

  guint sublevel;
  guint retval;

  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* sublevel */
  sublevel = ags_ipatch_get_nesting_level(ipatch);
  retval = 0;
  
#ifdef AGS_WITH_LIBINSTPATCH
  if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_DLS2)){
    AgsIpatchDLS2Reader *ipatch_dls2_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_dls2_reader = AGS_IPATCH_DLS2_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);
    
    switch(sublevel){
    case AGS_DLS2_FILENAME:
      {
	if(ags_ipatch_dls2_reader_select_instrument(ipatch_dls2_reader, level_index)){
	  retval = AGS_DLS2_FILENAME;
	}
      }
      break;
    case AGS_DLS2_IHDR:
      {
	if(ags_ipatch_dls2_reader_select_sample(ipatch_dls2_reader, level_index)){
	  retval = AGS_DLS2_IHDR;
	}
      }
      break;
    case AGS_DLS2_SHDR:
      {
	retval = AGS_DLS2_SHDR;
      }
      break;
    };
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_SF2)){
    AgsIpatchSF2Reader *ipatch_sf2_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_sf2_reader = AGS_IPATCH_SF2_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);
    
    switch(sublevel){
    case AGS_SF2_FILENAME:
      {
	if(ags_ipatch_sf2_reader_select_preset(ipatch_sf2_reader, level_index)){
	  retval = AGS_SF2_FILENAME;
	}
      }
      break;
    case AGS_SF2_PHDR:
      {
	if(ags_ipatch_sf2_reader_select_instrument(ipatch_sf2_reader, level_index)){
	  retval = AGS_SF2_PHDR;
	}
      }
      break;
    case AGS_SF2_IHDR:
      {
	if(ags_ipatch_sf2_reader_select_sample(ipatch_sf2_reader, level_index)){
	  retval = AGS_SF2_IHDR;
	}
      }
      break;
    case AGS_SF2_SHDR:
      {
	retval = AGS_SF2_SHDR;
      }
      break;
    };
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_GIG)){
    AgsIpatchGigReader *ipatch_gig_reader;
    
    pthread_mutex_lock(ipatch_mutex);

    ipatch_gig_reader = AGS_IPATCH_GIG_READER(ipatch->reader);

    pthread_mutex_unlock(ipatch_mutex);
    
    switch(sublevel){
    case AGS_GIG_FILENAME:
      {
	if(ags_ipatch_gig_reader_select_instrument(ipatch_gig_reader, level_index)){
	  retval = AGS_GIG_FILENAME;
	}
      }
      break;
    case AGS_GIG_IHDR:
      {
	if(ags_ipatch_gig_reader_select_sample(ipatch_gig_reader, level_index)){
	  retval = AGS_GIG_IHDR;
	}
      }
      break;
    case AGS_GIG_SHDR:
      {
	retval = AGS_GIG_SHDR;
      }
      break;
    };
  }
#endif
  
  return(retval);
}

GList*
ags_ipatch_get_resource_all(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;
  
  GList *resource;

  ipatch = AGS_IPATCH(sound_container);

  resource = NULL;
  
#ifdef AGS_WITH_LIBINSTPATCH
  if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_DLS2)){
    //TODO:JK: implement me
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_SF2)){
    //TODO:JK: implement me
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_GIG)){
    //TODO:JK: implement me
  }
#endif
  
  return(resource);
}

GList*
ags_ipatch_get_resource_by_name(AgsSoundContainer *sound_container,
				gchar *resource_name)
{
  //TODO:JK: implement me

  return(NULL);  
}

GList*
ags_ipatch_get_resource_by_index(AgsSoundContainer *sound_container,
				 guint resource_index)
{
  //TODO:JK: implement me

  return(NULL);
}

GList*
ags_ipatch_get_resource_current(AgsSoundContainer *sound_container)
{
  AgsIpatch *ipatch;

#ifdef AGS_WITH_LIBINSTPATCH
  IpatchItem *ipatch_item;
  IpatchList *ipatch_list;

  IpatchIter sample_iter;
#endif

  GList *sound_resource;

  guint i, i_stop;
  
  pthread_mutex_t *ipatch_mutex;
  
  ipatch = AGS_IPATCH(sound_container);

  /* get ipatch mutex */
  pthread_mutex_lock(ags_ipatch_get_class_mutex());
  
  ipatch_mutex = ipatch->obj_mutex;
  
  pthread_mutex_unlock(ags_ipatch_get_class_mutex());

  /* get sound resource */
  sound_resource = NULL;

  if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_DLS2)){
    AgsIpatchDLS2Reader *ipatch_dls2_reader;
    
    /* get dls2 reader */
    pthread_mutex_lock(ipatch_mutex);

    ipatch_dls2_reader = ipatch->reader;

    pthread_mutex_unlock(ipatch_mutex);
  
#ifdef AGS_WITH_LIBINSTPATCH
    ipatch_list = ipatch_dls2_inst_get_regions(ipatch_dls2_reader->instrument);

    if(ipatch_list != NULL){
      ipatch_list_init_iter(ipatch_list, &sample_iter);
      ipatch_iter_first(&sample_iter);

      i_stop = ipatch_iter_count(&sample_iter);
    
      for(i  = 0; i < i_stop; i++){
	AgsIpatchSample *ipatch_sample;

	ipatch_item = ipatch_dls2_region_get_sample(ipatch_iter_get(&sample_iter));

	ipatch_sample = ags_ipatch_sample_new();
	g_object_set(ipatch_sample,
		     "sample", ipatch_item,
		     NULL);

	sound_resource = g_list_prepend(sound_resource,
					ipatch_sample);
	
	/* iterate */
	ipatch_iter_next(&sample_iter);
      }
    }
#endif
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_SF2)){
    AgsIpatchSF2Reader *ipatch_sf2_reader;
    
    /* get sf2 reader */
    pthread_mutex_lock(ipatch_mutex);

    ipatch_sf2_reader = ipatch->reader;

    pthread_mutex_unlock(ipatch_mutex);
  
#ifdef AGS_WITH_LIBINSTPATCH
    ipatch_list = ipatch_sf2_inst_get_zones(ipatch_sf2_reader->instrument);

    if(ipatch_list != NULL){
      ipatch_list_init_iter(ipatch_list, &sample_iter);
      ipatch_iter_first(&sample_iter);

      i_stop = ipatch_iter_count(&sample_iter);
    
      for(i  = 0; i < i_stop; i++){
	AgsIpatchSample *ipatch_sample;

	ipatch_item = ipatch_sf2_izone_get_sample(ipatch_iter_get(&sample_iter));

	ipatch_sample = ags_ipatch_sample_new();
	g_object_set(ipatch_sample,
		     "sample", ipatch_item,
		     NULL);

	sound_resource = g_list_prepend(sound_resource,
					ipatch_sample);
	
	/* iterate */
	ipatch_iter_next(&sample_iter);
      }
    }
#endif
  }else if(ags_ipatch_test_flags(ipatch, AGS_IPATCH_GIG)){
    AgsIpatchGigReader *ipatch_gig_reader;
    
    /* get gig reader */
    pthread_mutex_lock(ipatch_mutex);

    ipatch_gig_reader = ipatch->reader;

    pthread_mutex_unlock(ipatch_mutex);
  
#ifdef AGS_WITH_LIBINSTPATCH
    ipatch_list = ipatch_dls2_inst_get_regions(ipatch_gig_reader->instrument);

    if(ipatch_list != NULL){
      ipatch_list_init_iter(ipatch_list, &sample_iter);
      ipatch_iter_first(&sample_iter);

      i_stop = ipatch_iter_count(&sample_iter);
    
      for(i  = 0; i < i_stop; i++){
	AgsIpatchSample *ipatch_sample;

	ipatch_item = ipatch_dls2_region_get_sample(ipatch_iter_get(&sample_iter));

	ipatch_sample = ags_ipatch_sample_new();
	g_object_set(ipatch_sample,
		     "sample", ipatch_item,
		     NULL);

	sound_resource = g_list_prepend(sound_resource,
					ipatch_sample);
	
	/* iterate */
	ipatch_iter_next(&sample_iter);
      }
    }
#endif
  }

  sound_resource = g_list_reverse(sound_resource);
  
  return(sound_resource);
}

void
ags_ipatch_close(AgsSoundContainer *sound_container)
{
  //TODO:JK: implement me
}

/**
 * ags_ipatch_check_suffix:
 * @filename: the filename
 * 
 * Check @filename's suffix to be supported.
 * 
 * Returns: %TRUE if supported, else %FALSE
 * 
 * Since: 2.0.0
 */
gboolean
ags_ipatch_check_suffix(gchar *filename)
{
  if(g_str_has_suffix(filename, ".sf2") ||
     g_str_has_suffix(filename, ".dls") ||
     g_str_has_suffix(filename, ".gig")){
    return(TRUE);
  }

  return(FALSE);
}

/**
 * ags_ipatch_new:
 *
 * Creates an #AgsIpatch.
 *
 * Returns: an empty #AgsIpatch.
 *
 * Since: 2.0.0
 */
AgsIpatch*
ags_ipatch_new()
{
  AgsIpatch *ipatch;

  ipatch = (AgsIpatch *) g_object_new(AGS_TYPE_IPATCH,
				      NULL);
  
  return(ipatch);
}
