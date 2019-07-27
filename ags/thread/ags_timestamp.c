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

#include <ags/thread/ags_timestamp.h>

#include <stdlib.h>

void ags_timestamp_class_init(AgsTimestampClass *timestamp);
void ags_timestamp_init (AgsTimestamp *timestamp);
void ags_timestamp_finalize(GObject *gobject);

/**
 * SECTION:ags_timestamp
 * @short_description: Timestamp unix and alike
 * @title: AgsTimestamp
 * @section_id:
 * @include: ags/thread/ags_timestamp.h
 *
 * #AgsTimestamp measure of time.
 */

static gpointer ags_timestamp_parent_class = NULL;

static pthread_mutex_t ags_timestamp_class_mutex = PTHREAD_MUTEX_INITIALIZER;

GType
ags_timestamp_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_timestamp = 0;

    static const GTypeInfo ags_timestamp_info = {
      sizeof(AgsTimestampClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_timestamp_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsTimestamp),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_timestamp_init,
    };

    ags_type_timestamp = g_type_register_static(G_TYPE_OBJECT,
						"AgsTimestamp",
						&ags_timestamp_info,
						0);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_timestamp);
  }

  return g_define_type_id__volatile;
}

void
ags_timestamp_class_init(AgsTimestampClass *timestamp)
{
  GObjectClass *gobject;

  ags_timestamp_parent_class = g_type_class_peek_parent(timestamp);

  /* GObjectClass */
  gobject = (GObjectClass *) timestamp;

  gobject->finalize = ags_timestamp_finalize;
}

void
ags_timestamp_init(AgsTimestamp *timestamp)
{
  pthread_mutex_t *mutex;
  pthread_mutexattr_t *attr;

  timestamp->flags = AGS_TIMESTAMP_UNIX;

  /* add timestamp mutex */
  timestamp->obj_mutexattr = 
    attr = (pthread_mutexattr_t *) malloc(sizeof(pthread_mutexattr_t));
  pthread_mutexattr_init(attr);
  pthread_mutexattr_settype(attr,
			    PTHREAD_MUTEX_RECURSIVE);

#ifdef __linux__
  pthread_mutexattr_setprotocol(attr,
				PTHREAD_PRIO_INHERIT);
#endif

  timestamp->obj_mutex = 
    mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex,
		     attr);  

  /* common fields */
  time(&(timestamp->timer.unix_time.time_val));

  timestamp->delay = 0.0;
  timestamp->attack = 0;
}

void
ags_timestamp_finalize(GObject *gobject)
{
  AgsTimestamp *timestamp;

  timestamp = AGS_TIMESTAMP(gobject);

  pthread_mutex_destroy(timestamp->obj_mutex);
  free(timestamp->obj_mutex);

  pthread_mutexattr_destroy(timestamp->obj_mutexattr);
  free(timestamp->obj_mutexattr);

  /* call parent */
  G_OBJECT_CLASS(ags_timestamp_parent_class)->finalize(gobject);
}

/**
 * ags_timestamp_get_class_mutex:
 * 
 * Use this function's returned mutex to access mutex fields.
 *
 * Returns: the class mutex
 * 
 * Since: 2.0.0
 */
pthread_mutex_t*
ags_timestamp_get_class_mutex()
{
  return(&ags_timestamp_class_mutex);
}

/**
 * ags_timestamp_test_flags:
 * @timestamp: the #AgsTimestamp
 * @flags: the flags
 * 
 * Test @flags to be set.
 * 
 * Returns: if @flags set returning %TRUE otherwise %FALSE
 * 
 * Since: 2.0.0
 */
gboolean
ags_timestamp_test_flags(AgsTimestamp *timestamp,
			 guint flags)
{
  gboolean success;
  
  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return(FALSE);
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* test flags */
  pthread_mutex_lock(timestamp_mutex);

  success = ((flags & (timestamp->flags)) != 0) ? TRUE: FALSE;

  pthread_mutex_unlock(timestamp_mutex);

  return(success);
}

/**
 * ags_timestamp_set_flags:
 * @timestamp: the #AgsTimestamp
 * @flags: the flags
 * 
 * Set @flags of @timestamp.
 * 
 * Since: 2.0.0
 */
void
ags_timestamp_set_flags(AgsTimestamp *timestamp,
			guint flags)
{
  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return;
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* set flags */
  pthread_mutex_lock(timestamp_mutex);

  timestamp->flags |= flags;

  pthread_mutex_unlock(timestamp_mutex);
}

/**
 * ags_timestamp_unset_flags:
 * @timestamp: the #AgsTimestamp
 * @flags: the flags
 * 
 * Unset @flags of @timestamp.
 * 
 * Since: 2.0.0
 */
void
ags_timestamp_unset_flags(AgsTimestamp *timestamp,
			  guint flags)
{
  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return;
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* unset flags */
  pthread_mutex_lock(timestamp_mutex);

  timestamp->flags &= (~flags);

  pthread_mutex_unlock(timestamp_mutex);
}

/**
 * ags_timestamp_get_unix_time:
 * @timestamp: the #AgsTimestamp
 * 
 * Get unix time.
 * 
 * Returns: the unix time as time_t value
 * 
 * Since: 2.0.0
 */
time_t
ags_timestamp_get_unix_time(AgsTimestamp *timestamp)
{
  time_t unix_time;

  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return(0);
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* get ags offset */
  pthread_mutex_lock(timestamp_mutex);
  
  unix_time = timestamp->timer.unix_time.time_val;

  pthread_mutex_unlock(timestamp_mutex);
  
  return(unix_time);
}

/**
 * ags_timestamp_set_unix_time:
 * @timestamp: the #AgsTimestamp
 * @unix_time: the unix time value
 * 
 * Set unix time.
 * 
 * Since: 2.0.0
 */
void
ags_timestamp_set_unix_time(AgsTimestamp *timestamp,
			    time_t unix_time)
{
  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return;
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* get ags offset */
  pthread_mutex_lock(timestamp_mutex);
  
  timestamp->timer.unix_time.time_val = unix_time;

  pthread_mutex_unlock(timestamp_mutex);
}

/**
 * ags_timestamp_get_ags_offset:
 * @timestamp: the #AgsTimestamp
 * 
 * Get AGS offset.
 * 
 * Returns: the AGS offset as unsigned 64 bit integer
 * 
 * Since: 2.0.0
 */
guint64
ags_timestamp_get_ags_offset(AgsTimestamp *timestamp)
{
  guint64 ags_offset;

  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return(0);
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* get ags offset */
  pthread_mutex_lock(timestamp_mutex);
  
  ags_offset = timestamp->timer.ags_offset.offset;

  pthread_mutex_unlock(timestamp_mutex);
  
  return(ags_offset);
}

/**
 * ags_timestamp_set_ags_offset:
 * @timestamp: the #AgsTimestamp
 * @ags_offset: the AGS offset
 * 
 * Set AGS offset as unsigned 64 bit integer.
 * 
 * Since: 2.0.0
 */
void
ags_timestamp_set_ags_offset(AgsTimestamp *timestamp,
			     guint64 ags_offset)
{
  pthread_mutex_t *timestamp_mutex;

  if(!AGS_IS_TIMESTAMP(timestamp)){
    return;
  }
  
  /* get timestamp mutex */
  timestamp_mutex = AGS_TIMESTAMP_GET_OBJ_MUTEX(timestamp);

  /* get ags offset */
  pthread_mutex_lock(timestamp_mutex);
  
  timestamp->timer.ags_offset.offset = ags_offset;

  pthread_mutex_unlock(timestamp_mutex);
}

/**
 * ags_timestamp_new:
 *
 * Creates an #AgsTimestamp
 *
 * Returns: a new #AgsTimestamp
 *
 * Since: 2.0.0
 */
AgsTimestamp*
ags_timestamp_new()
{
  AgsTimestamp *timestamp;

  timestamp = (AgsTimestamp *) g_object_new(AGS_TYPE_TIMESTAMP,
					    NULL);

  return(timestamp);
}
