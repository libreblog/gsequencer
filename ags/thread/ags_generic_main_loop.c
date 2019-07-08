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

#include <ags/thread/ags_generic_main_loop.h>

#include <ags/object/ags_main_loop.h>

#include <ags/i18n.h>

void ags_generic_main_loop_class_init(AgsGenericMainLoopClass *generic_main_loop);
void ags_generic_main_loop_main_loop_interface_init(AgsMainLoopInterface *main_loop);
void ags_generic_main_loop_init(AgsGenericMainLoop *generic_main_loop);
void ags_generic_main_loop_set_property(GObject *gobject,
					guint prop_id,
					const GValue *value,
					GParamSpec *param_spec);
void ags_generic_main_loop_get_property(GObject *gobject,
					guint prop_id,
					GValue *value,
					GParamSpec *param_spec);
void ags_generic_main_loop_finalize(GObject *gobject);

pthread_mutex_t* ags_generic_main_loop_get_tree_lock(AgsMainLoop *main_loop);
void ags_generic_main_loop_set_async_queue(AgsMainLoop *main_loop, GObject *async_queue);
GObject* ags_generic_main_loop_get_async_queue(AgsMainLoop *main_loop);
void ags_generic_main_loop_set_tic(AgsMainLoop *main_loop, guint tic);
guint ags_generic_main_loop_get_tic(AgsMainLoop *main_loop);
void ags_generic_main_loop_set_last_sync(AgsMainLoop *main_loop, guint last_sync);
guint ags_generic_main_loop_get_last_sync(AgsMainLoop *main_loop);
void ags_generic_main_loop_sync_counter_inc(AgsMainLoop *main_loop, guint tic);
void ags_generic_main_loop_sync_counter_dec(AgsMainLoop *main_loop, guint tic);
gboolean ags_generic_main_loop_sync_counter_test(AgsMainLoop *main_loop, guint tic);

void ags_generic_main_loop_start(AgsThread *thread);

/**
 * SECTION:ags_generic_main_loop
 * @short_description: generic loop
 * @title: AgsGenericMainLoop
 * @section_id:
 * @include: ags/thread/ags_generic_main_loop.h
 *
 * The #AgsGenericMainLoop is suitable as #AgsMainLoop and does
 * generic processing.
 */

static gpointer ags_generic_main_loop_parent_class = NULL;

enum{
  PROP_0,
  PROP_APPLICATION_CONTEXT,
};

GType
ags_generic_main_loop_get_type()
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_generic_main_loop = 0;

    static const GTypeInfo ags_generic_main_loop_info = {
      sizeof (AgsGenericMainLoopClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_generic_main_loop_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsGenericMainLoop),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_generic_main_loop_init,
    };

    static const GInterfaceInfo ags_main_loop_interface_info = {
      (GInterfaceInitFunc) ags_generic_main_loop_main_loop_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_generic_main_loop = g_type_register_static(AGS_TYPE_THREAD,
						 "AgsGenericMainLoop",
						 &ags_generic_main_loop_info,
						 0);
    
    g_type_add_interface_static(ags_type_generic_main_loop,
				AGS_TYPE_MAIN_LOOP,
				&ags_main_loop_interface_info);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_generic_main_loop);
  }

  return g_define_type_id__volatile;
}

void
ags_generic_main_loop_class_init(AgsGenericMainLoopClass *generic_main_loop)
{
  GObjectClass *gobject;

  AgsThreadClass *thread;

  GParamSpec *param_spec;
  
  ags_generic_main_loop_parent_class = g_type_class_peek_parent(generic_main_loop);

  /* GObject */
  gobject = (GObjectClass *) generic_main_loop;

  gobject->set_property = ags_generic_main_loop_set_property;
  gobject->get_property = ags_generic_main_loop_get_property;

  gobject->finalize = ags_generic_main_loop_finalize;

  /* properties */
  /**
   * AgsGenericMainLoop:application-context:
   *
   * The assigned #AgsApplicationContext
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_object("application-context",
				   i18n_pspec("the application context object"),
				   i18n_pspec("The application context object"),
				   AGS_TYPE_APPLICATION_CONTEXT,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_APPLICATION_CONTEXT,
				  param_spec);

  /* AgsThread */
  thread = (AgsThreadClass *) generic_main_loop;
  
  thread->start = ags_generic_main_loop_start;

  /* AgsGenericMainLoop */
}

void
ags_generic_main_loop_main_loop_interface_init(AgsMainLoopInterface *main_loop)
{
  main_loop->get_tree_lock = ags_generic_main_loop_get_tree_lock;

  main_loop->set_async_queue = ags_generic_main_loop_set_async_queue;
  main_loop->get_async_queue = ags_generic_main_loop_get_async_queue;

  main_loop->set_tic = ags_generic_main_loop_set_tic;
  main_loop->get_tic = ags_generic_main_loop_get_tic;

  main_loop->set_last_sync = ags_generic_main_loop_set_last_sync;
  main_loop->get_last_sync = ags_generic_main_loop_get_last_sync;

  main_loop->sync_counter_inc = ags_generic_main_loop_sync_counter_inc;
  main_loop->sync_counter_dec = ags_generic_main_loop_sync_counter_dec;
  main_loop->sync_counter_test = ags_generic_main_loop_sync_counter_test;
}

void
ags_generic_main_loop_init(AgsGenericMainLoop *generic_main_loop)
{
  AgsThread *thread;

  /* calculate frequency */
  thread = (AgsThread *) generic_main_loop;
  
  thread->freq = AGS_GENERIC_MAIN_LOOP_DEFAULT_JIFFIE;

  g_atomic_int_set(&(generic_main_loop->tic), 0);
  g_atomic_int_set(&(generic_main_loop->last_sync), 0);

  generic_main_loop->application_context = NULL;
  
  /* tree lock mutex */
  pthread_mutexattr_init(&(generic_main_loop->tree_lock_mutexattr));
  pthread_mutexattr_settype(&(generic_main_loop->tree_lock_mutexattr), PTHREAD_MUTEX_RECURSIVE);

  generic_main_loop->tree_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(generic_main_loop->tree_lock, &(generic_main_loop->tree_lock_mutexattr));
}

void
ags_generic_main_loop_set_property(GObject *gobject,
				   guint prop_id,
				   const GValue *value,
				   GParamSpec *param_spec)
{
  AgsGenericMainLoop *generic_main_loop;

  pthread_mutex_t *thread_mutex;

  generic_main_loop = AGS_GENERIC_MAIN_LOOP(gobject);

  /* get thread mutex */
  pthread_mutex_lock(ags_thread_get_class_mutex());
  
  thread_mutex = AGS_THREAD(gobject)->obj_mutex;
  
  pthread_mutex_unlock(ags_thread_get_class_mutex());

  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      AgsApplicationContext *application_context;

      application_context = (AgsApplicationContext *) g_value_get_object(value);

      pthread_mutex_lock(thread_mutex);

      if(generic_main_loop->application_context == (GObject *) application_context){
	pthread_mutex_unlock(thread_mutex);

	return;
      }

      if(generic_main_loop->application_context != NULL){
	g_object_unref(G_OBJECT(generic_main_loop->application_context));
      }

      if(application_context != NULL){
	g_object_ref(G_OBJECT(application_context));
      }
      
      generic_main_loop->application_context = (GObject *) application_context;

      pthread_mutex_unlock(thread_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_generic_main_loop_get_property(GObject *gobject,
				   guint prop_id,
				   GValue *value,
				   GParamSpec *param_spec)
{
  AgsGenericMainLoop *generic_main_loop;

  pthread_mutex_t *thread_mutex;

  generic_main_loop = AGS_GENERIC_MAIN_LOOP(gobject);

  /* get thread mutex */
  pthread_mutex_lock(ags_thread_get_class_mutex());
  
  thread_mutex = AGS_THREAD(gobject)->obj_mutex;
  
  pthread_mutex_unlock(ags_thread_get_class_mutex());

  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      pthread_mutex_lock(thread_mutex);

      g_value_set_object(value, generic_main_loop->application_context);

      pthread_mutex_unlock(thread_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_generic_main_loop_finalize(GObject *gobject)
{
  /* call parent */
  G_OBJECT_CLASS(ags_generic_main_loop_parent_class)->finalize(gobject);
}

pthread_mutex_t*
ags_generic_main_loop_get_tree_lock(AgsMainLoop *main_loop)
{
  return(AGS_THREAD(main_loop)->obj_mutex);
}

void
ags_generic_main_loop_set_async_queue(AgsMainLoop *main_loop, GObject *async_queue)
{
  AGS_GENERIC_MAIN_LOOP(main_loop)->async_queue = async_queue;
}

GObject*
ags_generic_main_loop_get_async_queue(AgsMainLoop *main_loop)
{
  return(AGS_GENERIC_MAIN_LOOP(main_loop)->async_queue);
}

void
ags_generic_main_loop_set_tic(AgsMainLoop *main_loop, guint tic)
{
  g_atomic_int_set(&(AGS_GENERIC_MAIN_LOOP(main_loop)->tic),
		   tic);
}

guint
ags_generic_main_loop_get_tic(AgsMainLoop *main_loop)
{
  return(g_atomic_int_get(&(AGS_GENERIC_MAIN_LOOP(main_loop)->tic)));
}

void
ags_generic_main_loop_set_last_sync(AgsMainLoop *main_loop, guint last_sync)
{
  g_atomic_int_set(&(AGS_GENERIC_MAIN_LOOP(main_loop)->last_sync),
		   last_sync);
}

guint
ags_generic_main_loop_get_last_sync(AgsMainLoop *main_loop)
{
  gint val;

  val = g_atomic_int_get(&(AGS_GENERIC_MAIN_LOOP(main_loop)->last_sync));

  return(val);
}

void
ags_generic_main_loop_sync_counter_inc(AgsMainLoop *main_loop, guint tic)
{
  AgsGenericMainLoop *generic_main_loop;

  pthread_mutex_t *thread_mutex;

  if(tic >= 3){
    return;
  }
  
  generic_main_loop = AGS_GENERIC_MAIN_LOOP(main_loop);

  /* get thread mutex */
  pthread_mutex_lock(ags_thread_get_class_mutex());
  
  thread_mutex = AGS_THREAD(generic_main_loop)->obj_mutex;
  
  pthread_mutex_unlock(ags_thread_get_class_mutex());

  /* increment */
  pthread_mutex_lock(thread_mutex);
  
  generic_main_loop->sync_counter[tic] += 1;

  pthread_mutex_unlock(thread_mutex);
}

void
ags_generic_main_loop_sync_counter_dec(AgsMainLoop *main_loop, guint tic)
{
  AgsGenericMainLoop *generic_main_loop;

  pthread_mutex_t *thread_mutex;

  if(tic >= 3){
    return;
  }
  
  generic_main_loop = AGS_GENERIC_MAIN_LOOP(main_loop);

  /* get thread mutex */
  pthread_mutex_lock(ags_thread_get_class_mutex());
  
  thread_mutex = AGS_THREAD(generic_main_loop)->obj_mutex;
  
  pthread_mutex_unlock(ags_thread_get_class_mutex());

  /* increment */
  pthread_mutex_lock(thread_mutex);

  if(generic_main_loop->sync_counter[tic] > 0){
    generic_main_loop->sync_counter[tic] -= 1;
  }
  
  pthread_mutex_unlock(thread_mutex);
}

gboolean
ags_generic_main_loop_sync_counter_test(AgsMainLoop *main_loop, guint tic)
{
  AgsGenericMainLoop *generic_main_loop;

  gboolean success;
  
  pthread_mutex_t *thread_mutex;

  if(tic >= 3){
    return(FALSE);
  }
  
  generic_main_loop = AGS_GENERIC_MAIN_LOOP(main_loop);

  /* get thread mutex */
  pthread_mutex_lock(ags_thread_get_class_mutex());
  
  thread_mutex = AGS_THREAD(generic_main_loop)->obj_mutex;
  
  pthread_mutex_unlock(ags_thread_get_class_mutex());

  /* test */
  success = FALSE;
  
  pthread_mutex_lock(thread_mutex);

  if(generic_main_loop->sync_counter[tic] == 0){
    success = TRUE;
  }
  
  pthread_mutex_unlock(thread_mutex);

  return(success);
}

void
ags_generic_main_loop_start(AgsThread *thread)
{
  if((AGS_THREAD_SINGLE_LOOP & (g_atomic_int_get(&(thread->flags)))) == 0){
    /*  */
    AGS_THREAD_CLASS(ags_generic_main_loop_parent_class)->start(thread);
  }
}
 
/**
 * ags_generic_main_loop_new:
 * @application_context: the #AgsMain
 *
 * Create a new #AgsGenericMainLoop.
 *
 * Returns: the new #AgsGenericMainLoop
 *
 * Since: 2.0.0
 */
AgsGenericMainLoop*
ags_generic_main_loop_new(GObject *application_context)
{
  AgsGenericMainLoop *generic_main_loop;

  generic_main_loop = (AgsGenericMainLoop *) g_object_new(AGS_TYPE_GENERIC_MAIN_LOOP,
							  "application-context", application_context,
							  NULL);

  return(generic_main_loop);
}
