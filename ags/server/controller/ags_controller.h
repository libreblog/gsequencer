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

#ifndef __AGS_CONTROLLER_H__
#define __AGS_CONTROLLER_H__

#include <glib.h>
#include <glib-object.h>

#define AGS_TYPE_CONTROLLER                (ags_controller_get_type())
#define AGS_CONTROLLER(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_CONTROLLER, AgsController))
#define AGS_CONTROLLER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST(class, AGS_TYPE_CONTROLLER, AgsControllerClass))
#define AGS_IS_CONTROLLER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_CONTROLLER))
#define AGS_IS_CONTROLLER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_CONTROLLER))
#define AGS_CONTROLLER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS(obj, AGS_TYPE_CONTROLLER, AgsControllerClass))

typedef struct _AgsController AgsController;
typedef struct _AgsControllerClass AgsControllerClass;

struct _AgsController
{
  GObject gobject;

  GObject *server;

  gchar *context_path;
};

struct _AgsControllerClass
{
  GObjectClass gobject;
};

GType ags_controller_get_type();

AgsController* ags_controller_new();

#endif /*__AGS_CONTROLLER_H__*/
