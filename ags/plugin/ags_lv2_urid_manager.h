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

#ifndef __AGS_LV2_URID_MANAGER_H__
#define __AGS_LV2_URID_MANAGER_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>

#include <lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

G_BEGIN_DECLS

#define AGS_TYPE_LV2_URID_MANAGER                (ags_lv2_urid_manager_get_type())
#define AGS_LV2_URID_MANAGER(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_LV2_URID_MANAGER, AgsLv2UridManager))
#define AGS_LV2_URID_MANAGER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_LV2_URID_MANAGER, AgsLv2UridManagerClass))
#define AGS_IS_LV2_URID_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_LV2_URID_MANAGER))
#define AGS_IS_LV2_URID_MANAGER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_LV2_URID_MANAGER))
#define AGS_LV2_URID_MANAGER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_LV2_URID_MANAGER, AgsLv2UridManagerClass))

#define AGS_LV2_URID_MANAGER_GET_OBJ_MUTEX(obj) (&(((AgsLv2UridManager *) obj)->obj_mutex))

typedef struct _AgsLv2UridManager AgsLv2UridManager;
typedef struct _AgsLv2UridManagerClass AgsLv2UridManagerClass;

struct _AgsLv2UridManager
{
  GObject gobject;

  GRecMutex obj_mutex;

  uint32_t id_counter;
  
  GHashTable *urid;  
};

struct _AgsLv2UridManagerClass
{
  GObjectClass gobject;
};

GType ags_lv2_urid_manager_get_type(void);

gboolean ags_lv2_urid_manager_insert(AgsLv2UridManager *lv2_urid_manager,
				     gchar *uri, GValue *id);
gboolean ags_lv2_urid_manager_remove(AgsLv2UridManager *lv2_urid_manager,
				     gchar *uri);

GValue* ags_lv2_urid_manager_lookup(AgsLv2UridManager *lv2_urid_manager,
				    gchar *uri);

void ags_lv2_urid_manager_load_default(AgsLv2UridManager *lv2_urid_manager);

uint32_t ags_lv2_urid_manager_map(LV2_URID_Map_Handle handle,
				  char *uri);
const char* ags_lv2_urid_manager_unmap(LV2_URID_Map_Handle handle,
				       uint32_t urid);

AgsLv2UridManager* ags_lv2_urid_manager_get_instance();
AgsLv2UridManager* ags_lv2_urid_manager_new();

G_END_DECLS

#endif /*__AGS_LV2_URID_MANAGER_H__*/
