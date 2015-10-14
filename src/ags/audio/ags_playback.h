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

#ifndef __AGS_PLAYBACK_H__
#define __AGS_PLAYBACK_H__

#include <glib.h>
#include <glib-object.h>

#ifdef AGS_USE_LINUX_THREADS
#include <ags/thread/ags_thread-kthreads.h>
#else
#include <ags/thread/ags_thread-posix.h>
#endif 

#include <ags/audio/ags_recall_id.h>

#include <ags/audio/thread/ags_iterator_thread.h>

#define AGS_TYPE_PLAYBACK                (ags_playback_get_type())
#define AGS_PLAYBACK(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_PLAYBACK, AgsPlayback))
#define AGS_PLAYBACK_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST(class, AGS_TYPE_PLAYBACK, AgsPlayback))
#define AGS_IS_PLAYBACK(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_PLAYBACK))
#define AGS_IS_PLAYBACK_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_PLAYBACK))
#define AGS_PLAYBACK_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS(obj, AGS_TYPE_PLAYBACK, AgsPlaybackClass))

typedef struct _AgsPlayback AgsPlayback;
typedef struct _AgsPlaybackClass AgsPlaybackClass;

typedef enum
{
  AGS_PLAYBACK_DONE                         = 1,
  AGS_PLAYBACK_REMOVE                       = 1 <<  1,
  AGS_PLAYBACK_CHANNEL                      = 1 <<  2,
  AGS_PLAYBACK_PAD                          = 1 <<  3,
  AGS_PLAYBACK_AUDIO                        = 1 <<  4,
  AGS_PLAYBACK_PLAYBACK                     = 1 <<  5,
  AGS_PLAYBACK_SEQUENCER                    = 1 <<  6,
  AGS_PLAYBACK_NOTATION                     = 1 <<  7,
  AGS_PLAYBACK_SINGLE_THREADED              = 1 <<  8,
  AGS_PLAYBACK_SUPER_THREADED_CHANNEL       = 1 <<  9,
  AGS_PLAYBACK_SUPER_THREADED_RECYCLING     = 1 << 10,
}AgsPlaybackFlags;

struct _AgsPlayback
{
  GObject gobject;
  
  volatile guint flags;

  AgsThread **channel_thread;
  AgsIteratorThread **iterator_thread;

  AgsThread **recycling_thread;
  
  GObject *source;
  guint audio_channel;

  AgsRecallID **recall_id;
};

struct _AgsPlaybackClass
{
  GObjectClass gobject;
};

GType ags_playback_get_type();

AgsPlayback* ags_playback_find_source(GList *playback,
				      GObject *source);

AgsPlayback* ags_playback_new();

#endif /*__AGS_PLAYBACK_H__*/
