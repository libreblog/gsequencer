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

#ifndef __AGS_LADSPA_MANAGER_H__
#define __AGS_LADSPA_MANAGER_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>

#include <ags/plugin/ags_ladspa_plugin.h>

G_BEGIN_DECLS

#define AGS_TYPE_LADSPA_MANAGER                (ags_ladspa_manager_get_type())
#define AGS_LADSPA_MANAGER(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_LADSPA_MANAGER, AgsLadspaManager))
#define AGS_LADSPA_MANAGER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_LADSPA_MANAGER, AgsLadspaManagerClass))
#define AGS_IS_LADSPA_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_LADSPA_MANAGER))
#define AGS_IS_LADSPA_MANAGER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_LADSPA_MANAGER))
#define AGS_LADSPA_MANAGER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_LADSPA_MANAGER, AgsLadspaManagerClass))

#define AGS_LADSPA_MANAGER_GET_OBJ_MUTEX(obj) (&(((AgsLadspaManager *) obj)->obj_mutex))

typedef struct _AgsLadspaManager AgsLadspaManager;
typedef struct _AgsLadspaManagerClass AgsLadspaManagerClass;

struct _AgsLadspaManager
{
  GObject gobject;

  GRecMutex obj_mutex;

  GList *ladspa_plugin_blacklist;
  GList *ladspa_plugin;
};

struct _AgsLadspaManagerClass
{
  GObjectClass gobject;
};

GType ags_ladspa_manager_get_type(void);

gchar** ags_ladspa_manager_get_default_path();
void ags_ladspa_manager_set_default_path(gchar** default_path);

gchar** ags_ladspa_manager_get_filenames(AgsLadspaManager *ladspa_manager);
AgsLadspaPlugin* ags_ladspa_manager_find_ladspa_plugin(AgsLadspaManager *ladspa_manager,
						       gchar *filename, gchar *effect);

void ags_ladspa_manager_load_blacklist(AgsLadspaManager *ladspa_manager,
				       gchar *blacklist_filename);

void ags_ladspa_manager_load_file(AgsLadspaManager *ladspa_manager,
				  gchar *ladspa_path,
				  gchar *filename);
void ags_ladspa_manager_load_default_directory(AgsLadspaManager *ladspa_manager);

/*  */
AgsLadspaManager* ags_ladspa_manager_get_instance();

AgsLadspaManager* ags_ladspa_manager_new();

G_END_DECLS

#endif /*__AGS_LADSPA_MANAGER_H__*/
