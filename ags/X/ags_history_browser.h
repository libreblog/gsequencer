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

#ifndef __AGS_HISTORY_BROWSER_H__
#define __AGS_HISTORY_BROWSER_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define AGS_TYPE_HISTORY_BROWSER                (ags_history_browser_get_type())
#define AGS_HISTORY_BROWSER(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_HISTORY_BROWSER, AgsHistoryBrowser))
#define AGS_HISTORY_BROWSER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_HISTORY_BROWSER, AgsHistoryBrowserClass))
#define AGS_IS_HISTORY_BROWSER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_HISTORY_BROWSER))
#define AGS_IS_HISTORY_BROWSER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_HISTORY_BROWSER))
#define AGS_HISTORY_BROWSER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_HISTORY_BROWSER, AgsHistoryBrowserClass))

typedef struct _AgsHistoryBrowser AgsHistoryBrowser;
typedef struct _AgsHistoryBrowserClass AgsHistoryBrowserClass;

typedef enum{
  AGS_HISTORY_BROWSER_CONNECTED   = 1,
}AgsHistoryBrowserFlags;

struct _AgsHistoryBrowser
{
  GtkWindow window;

  guint flags;

  GObject *application_context;
};

struct _AgsHistoryBrowserClass
{
  GtkWindowClass window;
};

GType ags_history_browser_get_type(void);

AgsHistoryBrowser* ags_history_browser_new();

#endif /*__AGS_HISTORY_BROWSER_H__*/
