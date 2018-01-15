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

#ifndef __AGS_RECALL_H__
#define __AGS_RECALL_H__

#include <glib.h>
#include <glib-object.h>

#include <pthread.h>

#include <ags/audio/ags_port.h>
#include <ags/audio/ags_recall_id.h>
#include <ags/audio/ags_recall_dependency.h>

#define AGS_TYPE_RECALL                (ags_recall_get_type())
#define AGS_RECALL(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_RECALL, AgsRecall))
#define AGS_RECALL_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_RECALL, AgsRecallClass))
#define AGS_IS_RECALL(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_RECALL))
#define AGS_IS_RECALL_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_RECALL))
#define AGS_RECALL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_RECALL, AgsRecallClass))

#define AGS_RECALL_HANDLER(handler)    ((AgsRecallHandler *)(handler))

#define AGS_RECALL_DEFAULT_VERSION "0.4.2"
#define AGS_RECALL_DEFAULT_BUILD_ID "CEST 02-10-2014 19:36"

typedef struct _AgsRecall AgsRecall;
typedef struct _AgsRecallClass AgsRecallClass;
typedef struct _AgsRecallHandler AgsRecallHandler;

/**
 * AgsRecallFlags:
 * @AGS_RECALL_ADDED_TO_REGISTRY: recall has been added to registry
 * @AGS_RECALL_CONNECTED: indicates the port was connected by calling #AgsConnectable::connect()
 * @AGS_RECALL_RUN_INITIALIZED: run initialized
 * @AGS_RECALL_TEMPLATE: is template
 * @AGS_RECALL_PLAYBACK: does playback
 * @AGS_RECALL_SEQUENCER: does sequencer
 * @AGS_RECALL_NOTATION: does notation
 * @AGS_RECALL_DEFAULT_TEMPLATE: 
 * @AGS_RECALL_DYNAMIC_CONNECTED: dynamic connected
 * @AGS_RECALL_INPUT_ORIENTATED: input orientated
 * @AGS_RECALL_OUTPUT_ORIENTATED: output orientated
 * @AGS_RECALL_PERSISTENT: persistent processing
 * @AGS_RECALL_INITIAL_RUN: initial call to #AgsRecall::run()
 * @AGS_RECALL_TERMINATING: is terminating
 * @AGS_RECALL_DONE: is done
 * @AGS_RECALL_REMOVE: is remove
 * @AGS_RECALL_HIDE: is hidden
 * @AGS_RECALL_PROPAGATE_DONE: propagate done 
 * @AGS_RECALL_PERSISTENT_PLAYBACK: persistent playback
 * @AGS_RECALL_PERSISTENT_SEQUENCER: persistent sequencer
 * @AGS_RECALL_PERSISTENT_NOTATION: persistent notation
 * @AGS_RECALL_SKIP_DEPENDENCIES: skip dependencies
 * @AGS_RECALL_BULK_MODE: the recall operates on bulk mode
 * @AGS_RECALL_HAS_OUTPUT_PORT: has output port
 * @AGS_RECALL_RUN_FIRST: scheduled for run-first
 * @AGS_RECALL_RUN_LAST: scheduled for run-last
 * 
 * Enum values to control the behavior or indicate internal state of #AgsRecall by
 * enable/disable as flags.
 */
typedef enum{
  AGS_RECALL_ADDED_TO_REGISTRY     = 1,
  AGS_RECALL_CONNECTED             = 1 <<  1,
  AGS_RECALL_RUN_INITIALIZED       = 1 <<  2, //TODO:JK: rename to AGS_RECALL_RUN_CONNECTED
  AGS_RECALL_TEMPLATE              = 1 <<  3,
  AGS_RECALL_PLAYBACK              = 1 <<  4,
  AGS_RECALL_SEQUENCER             = 1 <<  5,
  AGS_RECALL_NOTATION              = 1 <<  6,
  AGS_RECALL_DEFAULT_TEMPLATE      = 1 <<  7,
  AGS_RECALL_DYNAMIC_CONNECTED     = 1 <<  8,
  AGS_RECALL_INPUT_ORIENTATED      = 1 <<  9,
  AGS_RECALL_OUTPUT_ORIENTATED     = 1 << 10,
  AGS_RECALL_PERSISTENT            = 1 << 11,
  AGS_RECALL_INITIAL_RUN           = 1 << 12,
  AGS_RECALL_TERMINATING           = 1 << 13,
  AGS_RECALL_DONE                  = 1 << 14,
  AGS_RECALL_REMOVE                = 1 << 15,
  AGS_RECALL_HIDE                  = 1 << 16,
  AGS_RECALL_PROPAGATE_DONE        = 1 << 17, // see ags_recall_real_remove
  AGS_RECALL_PERSISTENT_PLAYBACK   = 1 << 18,
  AGS_RECALL_PERSISTENT_SEQUENCER  = 1 << 19,
  AGS_RECALL_PERSISTENT_NOTATION   = 1 << 20,
  AGS_RECALL_SKIP_DEPENDENCIES     = 1 << 21,
  AGS_RECALL_BULK_MODE             = 1 << 22,
  AGS_RECALL_HAS_OUTPUT_PORT       = 1 << 23,
  AGS_RECALL_RUN_FIRST             = 1 << 24,
  AGS_RECALL_RUN_LAST              = 1 << 25,
  AGS_RECALL_PACKED                = 1 << 26,
}AgsRecallFlags;

/**
 * AgsRecallNotifyDependencyMode:
 * @AGS_RECALL_NOTIFY_RUN: notify dependency as calling run
 * @AGS_RECALL_NOTIFY_AUDIO: notify dependency audio
 * @AGS_RECALL_NOTIFY_AUDIO_RUN: notifiy dependency audio run
 * @AGS_RECALL_NOTIFY_CHANNEL: notifiy dependency channel
 * @AGS_RECALL_NOTIFY_CHANNEL_RUN: notifiy dependency channel run
 * @AGS_RECALL_NOTIFY_RECALL: notifiy dependency recall
 * 
 */
typedef enum{
  AGS_RECALL_NOTIFY_RUN,
  AGS_RECALL_NOTIFY_AUDIO,
  AGS_RECALL_NOTIFY_AUDIO_RUN,
  AGS_RECALL_NOTIFY_CHANNEL,
  AGS_RECALL_NOTIFY_CHANNEL_RUN,
  AGS_RECALL_NOTIFY_RECALL,
}AgsRecallNotifyDependencyMode;

struct _AgsRecall
{
  GObject object;

  guint flags;
  gboolean rt_safe;
  
  gchar *version;
  gchar *build_id;

  gchar *effect;
  gchar *name;

  gchar *xml_type;

  GObject *soundcard;
  GObject *container; // see AgsRecallContainer

  GList *dependencies;

  AgsRecallID *recall_id;

  pthread_mutexattr_t *children_attr;
  pthread_mutex_t *children_mutex;

  AgsRecall *parent;
  GList *children;

  GType child_type;
  GParameter *child_parameters;
  guint n_params;

  GList *port;
  GList *automation_port;
  
  GList *handlers;
};

struct _AgsRecallClass
{
  GObjectClass object;

  void (*load_automation)(AgsRecall *recall,
			  GList *automation_port);
  void (*unload_automation)(AgsRecall *recall);
  
  void (*resolve_dependencies)(AgsRecall *recall);

  void (*run_init_pre)(AgsRecall *recall);
  void (*run_init_inter)(AgsRecall *recall);
  void (*run_init_post)(AgsRecall *recall);
  void (*check_rt_stream)(AgsRecall *recall);
  
  void (*automate)(AgsRecall *recall);
  void (*run_pre)(AgsRecall *recall);
  void (*run_inter)(AgsRecall *recall);
  void (*run_post)(AgsRecall *recall);
  
  void (*stop_persistent)(AgsRecall *recall);
  void (*done)(AgsRecall *recall);

  void (*cancel)(AgsRecall *recall);
  void (*remove)(AgsRecall *recall);

  AgsRecall* (*duplicate)(AgsRecall *recall,
			  AgsRecallID *recall_id,
			  guint *n_params, GParameter *params); // if a sequencer is linked with a sequencer the AgsRecall's with the flag AGS_RECALL_SOURCE must be duplicated

  void (*notify_dependency)(AgsRecall *recall, guint dependency, gboolean increase);

  void (*child_added)(AgsRecall *recall, AgsRecall *child);
};

/**
 * AgsRecallHandler:
 * @signal_name: the signal to listen
 * @callback: the callback to use
 * @data: user data to pass
 * @handler: the handler id
 *
 * A #AgsRecallHandler-struct acts as a callback definition
 */
struct _AgsRecallHandler
{
  const gchar *signal_name;
  GCallback callback;
  GObject *data;
  gulong handler;
};

GType ags_recall_get_type();

void ags_recall_set_flags(AgsRecall *recall, guint flags);

void ags_recall_load_automation(AgsRecall *recall,
				GList *automation_port);
void ags_recall_unload_automation(AgsRecall *recall);

void ags_recall_resolve_dependencies(AgsRecall *recall);
void ags_recall_child_added(AgsRecall *parent, AgsRecall *child);

void ags_recall_run_init_pre(AgsRecall *recall);
void ags_recall_run_init_inter(AgsRecall *recall);
void ags_recall_run_init_post(AgsRecall *recall);
void ags_recall_check_rt_stream(AgsRecall *recall);

void ags_recall_automate(AgsRecall *recall);
void ags_recall_run_pre(AgsRecall *recall);
void ags_recall_run_inter(AgsRecall *recall);
void ags_recall_run_post(AgsRecall *recall);

void ags_recall_stop_persistent(AgsRecall *recall);
void ags_recall_done(AgsRecall *recall);

void ags_recall_cancel(AgsRecall *recall);
void ags_recall_remove(AgsRecall *recall);

gboolean ags_recall_is_done(GList *recalls, GObject *recycling_context);

AgsRecall* ags_recall_duplicate(AgsRecall *recall,
				AgsRecallID *recall_id);

void ags_recall_set_recall_id(AgsRecall *recall, AgsRecallID *recall_id);
void ags_recall_set_soundcard_recursive(AgsRecall *recall, GObject *soundcard);

void ags_recall_notify_dependency(AgsRecall *recall, guint flags, gint count);

void ags_recall_add_dependency(AgsRecall *recall, AgsRecallDependency *recall_dependency);
void ags_recall_remove_dependency(AgsRecall *recall, AgsRecall *dependency);
GList* ags_recall_get_dependencies(AgsRecall *recall);

void ags_recall_remove_child(AgsRecall *recall, AgsRecall *child);
void ags_recall_add_child(AgsRecall *parent, AgsRecall *child);
GList* ags_recall_get_children(AgsRecall *recall);

GList* ags_recall_get_by_effect(GList *list, gchar *filename, gchar *effect);
GList* ags_recall_find_recall_id_with_effect(GList *list, AgsRecallID *recall_id, gchar *filename, gchar *effect);

GList* ags_recall_find_type(GList *recall_i, GType type);
GList* ags_recall_find_template(GList *recall_i);
GList* ags_recall_template_find_type(GList *recall_i, GType type);
GList* ags_recall_template_find_all_type(GList *recall_i, ...);
GList* ags_recall_find_type_with_recycling_context(GList *recall_i, GType type, GObject *recycling_context);
GList* ags_recall_find_recycling_context(GList *recall_i, GObject *recycling_context);
GList* ags_recall_find_provider(GList *recall, GObject *provider);
GList* ags_recall_template_find_provider(GList *recall, GObject *provider);
GList* ags_recall_find_provider_with_recycling_context(GList *recall_i, GObject *provider, GObject *recycling_context);

void ags_recall_run_init(AgsRecall *recall, guint stage);

AgsRecallHandler* ags_recall_handler_alloc(const gchar *signal_name,
					   GCallback callback,
					   GObject *data);

void ags_recall_add_handler(AgsRecall *recall,
			    AgsRecallHandler *recall_handler);
void ags_recall_remove_handler(AgsRecall *recall,
			       AgsRecallHandler *recall_handler);

void ags_recall_lock_port(AgsRecall *recall);
void ags_recall_unlock_port(AgsRecall *recall);

AgsRecall* ags_recall_new();

#endif /*__AGS_RECALL_H__*/
