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

#ifndef __AGS_OPTION_MENU_H__
#define __AGS_OPTION_MENU_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <ags/widget/ags_table.h>

#define AGS_TYPE_OPTION_MENU                (ags_option_menu_get_type())
#define AGS_OPTION_MENU(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_OPTION_MENU, AgsOptionMenu))
#define AGS_OPTION_MENU_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_OPTION_MENU, AgsOptionMenuClass))
#define AGS_IS_OPTION_MENU(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_OPTION_MENU))
#define AGS_IS_OPTION_MENU_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_OPTION_MENU))
#define AGS_OPTION_MENU_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_OPTION_MENU, AgsOptionMenuClass))

typedef struct _AgsOptionMenu AgsOptionMenu;
typedef struct _AgsOptionMenuClass AgsOptionMenuClass;

struct _AgsOptionMenu
{
  GtkHBox hbox;

  GtkWidget *button;
  AgsTable *table;

  GtkWidget *menu;
  GList *active_item;
  GtkWidget *focus;
};

struct _AgsOptionMenuClass
{
  GtkHBoxClass hbox;

  void (*changed)(AgsOptionMenu *option_menu);
  void (*focus_changed)(AgsOptionMenu *option_menu);
};

GType ags_option_menu_get_type(void);

AgsOptionMenu* ags_option_menu_new();

#endif /*__AGS_OPTION_MENU_H__*/
