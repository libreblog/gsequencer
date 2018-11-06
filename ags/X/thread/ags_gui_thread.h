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

#ifndef __AGS_GUI_THREAD_H__
#define __AGS_GUI_THREAD_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>
#include <ags/libags-audio.h>

#include <ags/X/file/ags_simple_file.h>

#include <unistd.h>

#define AGS_TYPE_GUI_THREAD                (ags_gui_thread_get_type())
#define AGS_GUI_THREAD(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_GUI_THREAD, AgsGuiThread))
#define AGS_GUI_THREAD_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST(class, AGS_TYPE_GUI_THREAD, AgsGuiThreadClass))
#define AGS_IS_GUI_THREAD(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_GUI_THREAD))
#define AGS_IS_GUI_THREAD_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_GUI_THREAD))
#define AGS_GUI_THREAD_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS(obj, AGS_TYPE_GUI_THREAD, AgsGuiThreadClass))

#define AGS_GUI_THREAD_RT_PRIORITY (99)

#define AGS_GUI_THREAD_DEFAULT_JIFFIE (60.0)

#define AGS_GUI_THREAD_SYNC_DELAY (50000)
#define AGS_GUI_THREAD_SYNC_AVAILABLE_TIMEOUT (400)

typedef struct _AgsGuiThread AgsGuiThread;
typedef struct _AgsGuiThreadClass AgsGuiThreadClass;

typedef enum{
  AGS_GUI_THREAD_RUNNING    = 1,
}AgsGuiThreadFlags;

struct _AgsGuiThread
{
  AgsThread thread;

  volatile guint flags;
  
  volatile gboolean dispatching;
  
  GMutex mutex;
  GCond cond;
  
  GMainContext *main_context;

  GThread *gtk_thread;
  
  gint cached_poll_array_size;
  GPollFD *cached_poll_array;

  guint max_priority;
  
  GList *poll_fd;
  
  pthread_mutex_t *task_completion_mutex;
  volatile GList *task_completion;

  pthread_mutex_t *dispatch_mutex;

  pthread_mutex_t *task_schedule_mutex;

  guint nth_message;
  GSource *animation_source;

  guint queued_sync;
  GSource *sync_source;

  GList *collected_task;
  GSource *task_source;
};

struct _AgsGuiThreadClass
{
  AgsThreadClass thread;
};

GType ags_gui_thread_get_type();

void ags_gui_thread_complete_task(AgsGuiThread *gui_thread);

/* legacy sync and run */
void* ags_gui_thread_do_poll_loop(void *ptr);

void ags_gui_thread_run(AgsThread *thread);

/* gtk_main() related */
void ags_gui_init(int *argc, char ***argv);

void ags_gui_thread_enter();
void ags_gui_thread_leave();

pthread_mutex_t* ags_gui_thread_get_dispatch_mutex();

void ags_gui_thread_show_file_error(AgsGuiThread *gui_thread,
				    gchar *filename,
				    GError *error);

void ags_gui_thread_launch(AgsGuiThread *gui_thread,
			   gboolean single_thread);
void ags_gui_thread_launch_filename(AgsGuiThread *gui_thread,
				    gchar *filename,
				    gboolean single_thread);

void ags_gui_thread_timer_start(AgsGuiThread *gui_thread,
				void *timer_id);
void ags_gui_thread_timer_launch(AgsGuiThread *gui_thread,
				 void *timer_id,
				 gboolean single_thread);
void ags_gui_thread_timer_launch_filename(AgsGuiThread *gui_thread,
					  void *timer_id, gchar *filename,
					  gboolean single_thread);

void ags_gui_thread_do_animation(AgsGuiThread *gui_thread);
void ags_gui_thread_do_run(AgsGuiThread *gui_thread);

void ags_gui_thread_schedule_task(AgsGuiThread *gui_thread,
				  GObject *task);
void ags_gui_thread_schedule_task_list(AgsGuiThread *gui_thread,
				       GList *task);

/* AgsGuiThread */
AgsGuiThread* ags_gui_thread_new();

#endif /*__AGS_GUI_THREAD_H__*/
