#ifndef __AGS_REGISTRY_H__
#define __AGS_REGISTRY_H__

#include <glib.h>
#include <glib-object.h>

#include <xmlrpc.h>
#include <xmlrpc_server.h>

#define AGS_TYPE_REGISTRY                (ags_registry_get_type())
#define AGS_REGISTRY(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_REGISTRY, AgsRegistry))
#define AGS_REGISTRY_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST(class, AGS_TYPE_REGISTRY, AgsRegistryClass))
#define AGS_IS_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_REGISTRY))
#define AGS_IS_REGISTRY_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_REGISTRY))
#define AGS_REGISTRY_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS(obj, AGS_TYPE_REGISTRY, AgsRegistryClass))

#define AGS_REGISTRY_DEFAULT_ID_LENGTH 16

typedef struct _AgsRegistry AgsRegistry;
typedef struct _AgsRegistryClass AgsRegistryClass;
typedef struct _AgsRegistryEntry AgsRegistryEntry;

struct _AgsRegistry
{
  GObject object;

  guint id_length;
  guint counter;

  GList *registry;
};

struct _AgsRegistryClass
{
  GObjectClass object;
};

struct _AgsRegistryEntry
{
  gchar *id;
  GObject *entry;
};

GType ags_registry_get_type();

AgsRegistryEntry* ags_registry_entry_alloc(AgsRegistry *registry);

void ags_registry_add(AgsRegistry *registry,
		      AgsRegistryEntry *registry_entry);

AgsRegistryEntry* ags_registry_entry_find(AgsRegistry *registry,
					  gchar *id);

xmlrpc_value* ags_registry_entry_bulk(xmlrpc_env *env_p,
				      xmlrpc_value *param_array_p,
				      void *server_info);

AgsRegistry* ags_registry_new();

#endif /*__AGS_REGISTRY_H__*/
