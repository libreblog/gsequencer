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

#include <ags/audio/task/ags_start_soundcard.h>

#include <ags/object/ags_connectable.h>
#include <ags/object/ags_soundcard.h>

#include <ags/audio/thread/ags_audio_loop.h>
#include <ags/audio/thread/ags_soundcard_thread.h>

void ags_start_soundcard_class_init(AgsStartSoundcardClass *start_soundcard);
void ags_start_soundcard_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_start_soundcard_init(AgsStartSoundcard *start_soundcard);
void ags_start_soundcard_connect(AgsConnectable *connectable);
void ags_start_soundcard_disconnect(AgsConnectable *connectable);
void ags_start_soundcard_finalize(GObject *gobject);

void ags_start_soundcard_launch(AgsTask *task);

/**
 * SECTION:ags_start_soundcard
 * @short_description: start soundcard object
 * @title: AgsStartSoundcard
 * @section_id:
 * @include: ags/audio/task/ags_start_soundcard.h
 *
 * The #AgsStartSoundcard task starts soundcard.
 */

static gpointer ags_start_soundcard_parent_class = NULL;
static AgsConnectableInterface *ags_start_soundcard_parent_connectable_interface;

GType
ags_start_soundcard_get_type()
{
  static GType ags_type_start_soundcard = 0;

  if(!ags_type_start_soundcard){
    static const GTypeInfo ags_start_soundcard_info = {
      sizeof (AgsStartSoundcardClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_start_soundcard_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsStartSoundcard),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_start_soundcard_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_start_soundcard_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_start_soundcard = g_type_register_static(AGS_TYPE_TASK,
						   "AgsStartSoundcard\0",
						   &ags_start_soundcard_info,
						   0);

    g_type_add_interface_static(ags_type_start_soundcard,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
  }
  
  return (ags_type_start_soundcard);
}

void
ags_start_soundcard_class_init(AgsStartSoundcardClass *start_soundcard)
{
  GObjectClass *gobject;
  AgsTaskClass *task;

  ags_start_soundcard_parent_class = g_type_class_peek_parent(start_soundcard);

  /* gobject */
  gobject = (GObjectClass *) start_soundcard;

  gobject->finalize = ags_start_soundcard_finalize;

  /* task */
  task = (AgsTaskClass *) start_soundcard;

  task->launch = ags_start_soundcard_launch;
}

void
ags_start_soundcard_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_start_soundcard_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_start_soundcard_connect;
  connectable->disconnect = ags_start_soundcard_disconnect;
}

void
ags_start_soundcard_init(AgsStartSoundcard *start_soundcard)
{
  start_soundcard->soundcard = NULL;
}

void
ags_start_soundcard_connect(AgsConnectable *connectable)
{
  ags_start_soundcard_parent_connectable_interface->connect(connectable);

  /* empty */
}

void
ags_start_soundcard_disconnect(AgsConnectable *connectable)
{
  ags_start_soundcard_parent_connectable_interface->disconnect(connectable);

  /* empty */
}

void
ags_start_soundcard_finalize(GObject *gobject)
{
  AgsAudioLoop *audio_loop;
  AgsSoundcardThread *soundcard_thread;

  AgsApplicationContext *application_context;
  AgsSoundcard *soundcard;

  soundcard = AGS_SOUNDCARD(AGS_START_SOUNDCARD(gobject)->soundcard);

  application_context = ags_soundcard_get_application_context(soundcard);
  audio_loop = AGS_AUDIO_LOOP(application_context->main_loop);

  soundcard_thread = (AgsSoundcardThread *) ags_thread_find_type((AgsThread *) audio_loop,
								 AGS_TYPE_SOUNDCARD_THREAD);

  if(soundcard_thread->error != NULL){
    g_error_free(soundcard_thread->error);

    soundcard_thread->error = NULL;
  }

  G_OBJECT_CLASS(ags_start_soundcard_parent_class)->finalize(gobject);
}

void
ags_start_soundcard_launch(AgsTask *task)
{
  AgsStartSoundcard *start_soundcard;

  AgsAudioLoop *audio_loop;
  AgsThread *soundcard_thread;

  AgsApplicationContext *application_context;
  AgsSoundcard *soundcard;

  GList *start_queue;
  
  start_soundcard = AGS_START_SOUNDCARD(task);

  soundcard = AGS_SOUNDCARD(start_soundcard->soundcard);

  application_context = ags_soundcard_get_application_context(soundcard);
  audio_loop = AGS_AUDIO_LOOP(application_context->main_loop);

  audio_loop->flags |= (AGS_AUDIO_LOOP_PLAY_AUDIO |
			AGS_AUDIO_LOOP_PLAY_CHANNEL |
			AGS_AUDIO_LOOP_PLAY_RECALL);

  /*
  if(ags_soundcard_is_starting(soundcard) ||
     ags_soundcard_is_playing(soundcard)){
    return;
  }
  */
  soundcard_thread = audio_loop;
  
  while((soundcard_thread = ags_thread_find_type(soundcard_thread,
						 AGS_TYPE_SOUNDCARD_THREAD)) != NULL){
    /* append to AgsSoundcard */
    AGS_SOUNDCARD_THREAD(soundcard_thread)->error = NULL;

    g_message("start soundcard\0");

    start_queue = NULL;    
    start_queue = g_list_prepend(start_queue,
				 soundcard_thread);

    if(start_queue != NULL){
      if(g_atomic_pointer_get(&(AGS_THREAD(audio_loop)->start_queue)) != NULL){
	g_atomic_pointer_set(&(AGS_THREAD(audio_loop)->start_queue),
			     g_list_concat(start_queue,
					   g_atomic_pointer_get(&(AGS_THREAD(audio_loop)->start_queue))));
      }else{
	g_atomic_pointer_set(&(AGS_THREAD(audio_loop)->start_queue),
			     start_queue);
      }
    }

    soundcard_thread = g_atomic_pointer_get(&(soundcard_thread->next));
  }
}

/**
 * ags_start_soundcard_new:
 * @soundcard: the #AgsSoundcard
 *
 * Creates an #AgsStartSoundcard.
 *
 * Returns: an new #AgsStartSoundcard.
 *
 * Since: 0.4
 */
AgsStartSoundcard*
ags_start_soundcard_new(GObject *soundcard)
{
  AgsStartSoundcard *start_soundcard;

  start_soundcard = (AgsStartSoundcard *) g_object_new(AGS_TYPE_START_SOUNDCARD,
						       NULL);

  start_soundcard->soundcard = soundcard;

  return(start_soundcard);
}
