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

#include <ags/audio/ags_midiin.h>

#include <ags/object/ags_application_context.h>
#include <ags/object/ags_connectable.h>

#include <ags/object/ags_config.h>
#include <ags/object/ags_sequencer.h>

#include <ags/thread/ags_mutex_manager.h>

#include <ags/audio/ags_notation.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <alsa/rawmidi.h>
#include <errno.h>

#include <string.h>
#include <math.h>
#include <time.h>

#include <ags/config.h>

/**
 * SECTION:ags_midiin
 * @short_description: Input from sequencer
 * @title: AgsMidiin
 * @section_id:
 * @include: ags/object/ags_sequencer.h
 *
 * #AgsMidiin represents a sequencer and supports midi input.
 */

void ags_midiin_class_init(AgsMidiinClass *midiin);
void ags_midiin_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_midiin_sequencer_interface_init(AgsSequencerInterface *sequencer);
void ags_midiin_init(AgsMidiin *midiin);
void ags_midiin_set_property(GObject *gobject,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *param_spec);
void ags_midiin_get_property(GObject *gobject,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *param_spec);
void ags_midiin_disconnect(AgsConnectable *connectable);
void ags_midiin_connect(AgsConnectable *connectable);
void ags_midiin_finalize(GObject *gobject);

void ags_midiin_switch_buffer_flag(AgsMidiin *midiin);

void ags_midiin_set_application_context(AgsSequencer *sequencer,
					AgsApplicationContext *application_context);
AgsApplicationContext* ags_midiin_get_application_context(AgsSequencer *sequencer);

void ags_midiin_set_application_mutex(AgsSequencer *sequencer,
				      pthread_mutex_t *application_mutex);
pthread_mutex_t* ags_midiin_get_application_mutex(AgsSequencer *sequencer);

void ags_midiin_set_device(AgsSequencer *sequencer,
			   gchar *device);
gchar* ags_midiin_get_device(AgsSequencer *sequencer);

void ags_midiin_list_cards(AgsSequencer *sequencer,
			   GList **card_id, GList **card_name);

gboolean ags_midiin_is_starting(AgsSequencer *sequencer);
gboolean ags_midiin_is_recording(AgsSequencer *sequencer);

void ags_midiin_alsa_init(AgsSequencer *sequencer,
			  GError **error);
void ags_midiin_alsa_record(AgsSequencer *sequencer,
			    GError **error);
void ags_midiin_alsa_free(AgsSequencer *sequencer);

void ags_midiin_tic(AgsSequencer *sequencer);
void ags_midiin_offset_changed(AgsSequencer *sequencer,
			       guint note_offset);

void ags_midiin_set_bpm(AgsSequencer *sequencer,
			gdouble bpm);
gdouble ags_midiin_get_bpm(AgsSequencer *sequencer);

void ags_midiin_set_delay_factor(AgsSequencer *sequencer,
				 gdouble delay_factor);
gdouble ags_midiin_get_delay_factor(AgsSequencer *sequencer);

void* ags_midiin_get_buffer(AgsSequencer *sequencer,
			    guint *buffer_length);
void* ags_midiin_get_next_buffer(AgsSequencer *sequencer,
				 guint *buffer_length);

void ags_midiin_set_note_offset(AgsSequencer *sequencer,
				guint note_offset);
guint ags_midiin_get_note_offset(AgsSequencer *sequencer);

void ags_midiin_set_audio(AgsSequencer *sequencer,
			  GList *audio);
GList* ags_midiin_get_audio(AgsSequencer *sequencer);

enum{
  PROP_0,
  PROP_APPLICATION_CONTEXT,
  PROP_APPLICATION_MUTEX,
  PROP_DEVICE,
  PROP_BUFFER,
  PROP_BPM,
  PROP_DELAY_FACTOR,
  PROP_ATTACK,
};

enum{
  LAST_SIGNAL,
};

static gpointer ags_midiin_parent_class = NULL;
static guint midiin_signals[LAST_SIGNAL];

GType
ags_midiin_get_type (void)
{
  static GType ags_type_midiin = 0;

  if(!ags_type_midiin){
    static const GTypeInfo ags_midiin_info = {
      sizeof (AgsMidiinClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_midiin_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsMidiin),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_midiin_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_midiin_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_sequencer_interface_info = {
      (GInterfaceInitFunc) ags_midiin_sequencer_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_midiin = g_type_register_static(G_TYPE_OBJECT,
					     "AgsMidiin\0",
					     &ags_midiin_info,
					     0);

    g_type_add_interface_static(ags_type_midiin,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_midiin,
				AGS_TYPE_SEQUENCER,
				&ags_sequencer_interface_info);
  }

  return (ags_type_midiin);
}

void
ags_midiin_class_init(AgsMidiinClass *midiin)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;

  ags_midiin_parent_class = g_type_class_peek_parent(midiin);

  /* GObjectClass */
  gobject = (GObjectClass *) midiin;

  gobject->set_property = ags_midiin_set_property;
  gobject->get_property = ags_midiin_get_property;

  gobject->finalize = ags_midiin_finalize;

  /* properties */
  /**
   * AgsMidiin:application-context:
   *
   * The assigned #AgsApplicationContext
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_object("application-context\0",
				   "the application context object\0",
				   "The application context object\0",
				   AGS_TYPE_APPLICATION_CONTEXT,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_APPLICATION_CONTEXT,
				  param_spec);

  /**
   * AgsMidiin:application-mutex:
   *
   * The assigned application mutex
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_pointer("application-mutex\0",
				    "the application mutex object\0",
				    "The application mutex object\0",
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_APPLICATION_MUTEX,
				  param_spec);

  /**
   * AgsMidiin:device:
   *
   * The alsa sequencer indentifier
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_string("device\0",
				   "the device identifier\0",
				   "The device to perform output to\0",
				   "hw:0\0",
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DEVICE,
				  param_spec);

  /**
   * AgsMidiin:buffer:
   *
   * The buffer
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_pointer("buffer\0",
				    "the buffer\0",
				    "The buffer to play\0",
				    G_PARAM_READABLE);
  g_object_class_install_property(gobject,
				  PROP_BUFFER,
				  param_spec);

  /**
   * AgsMidiin:bpm:
   *
   * Beats per minute
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_double("bpm\0",
				   "beats per minute\0",
				   "Beats per minute to use\0",
				   1.0,
				   240.0,
				   120.0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_BPM,
				  param_spec);

  /**
   * AgsMidiin:delay-factor:
   *
   * tact
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_double("delay-factor\0",
				   "delay factor\0",
				   "The delay factor\0",
				   0.0,
				   16.0,
				   1.0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DELAY_FACTOR,
				  param_spec);

  /**
   * AgsMidiin:attack:
   *
   * Attack of the buffer
   * 
   * Since: 0.7.0
   */
  param_spec = g_param_spec_pointer("attack\0",
				    "attack of buffer\0",
				    "The attack to use for the buffer\0",
				    G_PARAM_READABLE);
  g_object_class_install_property(gobject,
				  PROP_ATTACK,
				  param_spec);


  /* AgsMidiinClass */
}

GQuark
ags_midiin_error_quark()
{
  return(g_quark_from_static_string("ags-midiin-error-quark\0"));
}

void
ags_midiin_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_midiin_connect;
  connectable->disconnect = ags_midiin_disconnect;
}

void
ags_midiin_sequencer_interface_init(AgsSequencerInterface *sequencer)
{
  sequencer->set_application_context = ags_midiin_set_application_context;
  sequencer->get_application_context = ags_midiin_get_application_context;

  sequencer->set_application_mutex = ags_midiin_set_application_mutex;
  sequencer->get_application_mutex = ags_midiin_get_application_mutex;

  sequencer->set_device = ags_midiin_set_device;
  sequencer->get_device = ags_midiin_get_device;

  sequencer->list_cards = ags_midiin_list_cards;

  sequencer->is_starting =  ags_midiin_is_starting;
  sequencer->is_playing = NULL;
  sequencer->is_recording = ags_midiin_is_recording;

  sequencer->play_init = NULL;
  sequencer->play = NULL;

  sequencer->record_init = ags_midiin_alsa_init;
  sequencer->record = ags_midiin_alsa_record;

  sequencer->stop = ags_midiin_alsa_free;

  sequencer->tic = ags_midiin_tic;
  sequencer->offset_changed = ags_midiin_offset_changed;
    
  sequencer->set_bpm = ags_midiin_set_bpm;
  sequencer->get_bpm = ags_midiin_get_bpm;

  sequencer->set_delay_factor = ags_midiin_set_delay_factor;
  sequencer->get_delay_factor = ags_midiin_get_delay_factor;
  
  sequencer->get_buffer = ags_midiin_get_buffer;
  sequencer->get_next_buffer = ags_midiin_get_next_buffer;

  sequencer->set_note_offset = ags_midiin_set_note_offset;
  sequencer->get_note_offset = ags_midiin_get_note_offset;

  sequencer->set_audio = ags_midiin_set_audio;
  sequencer->get_audio = ags_midiin_get_audio;
}

void
ags_midiin_init(AgsMidiin *midiin)
{
  /* flags */
  midiin->flags = (AGS_MIDIIN_ALSA);

  //  midiin->in.oss.device = NULL;
  midiin->in.alsa.handle = NULL;
  midiin->in.alsa.device = AGS_SEQUENCER_DEFAULT_DEVICE;

  /* buffer */
  midiin->buffer = (char **) malloc(4 * sizeof(char*));

  midiin->buffer[0] = NULL;
  midiin->buffer[1] = NULL;
  midiin->buffer[2] = NULL;
  midiin->buffer[3] = NULL;

  midiin->buffer_size[0] = 0;
  midiin->buffer_size[1] = 0;
  midiin->buffer_size[2] = 0;
  midiin->buffer_size[3] = 0;

  /* bpm */
  midiin->bpm = AGS_SEQUENCER_DEFAULT_BPM;

  /* delay and delay factor */
  midiin->delay = AGS_SEQUENCER_DEFAULT_DELAY;
  midiin->delay_factor = AGS_SEQUENCER_DEFAULT_DELAY_FACTOR;
    
  /* counters */
  midiin->note_offset = 0;
  midiin->tact_counter = 0.0;
  midiin->delay_counter = 0;
  midiin->tic_counter = 0;

  /* parent */
  midiin->application_context = NULL;
  midiin->application_mutex = NULL;

  /* all AgsAudio */
  midiin->audio = NULL;
}

void
ags_midiin_set_property(GObject *gobject,
			guint prop_id,
			const GValue *value,
			GParamSpec *param_spec)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(gobject);

  //TODO:JK: implement set functionality
  
  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      AgsApplicationContext *application_context;

      application_context = g_value_get_object(value);

      if(midiin->application_context == (GObject *) application_context){
	return;
      }

      if(midiin->application_context != NULL){
	g_object_unref(G_OBJECT(midiin->application_context));
      }

      if(application_context != NULL){
	g_object_ref(G_OBJECT(application_context));

	midiin->application_mutex = application_context->mutex;
      }else{
	midiin->application_mutex = NULL;
      }

      midiin->application_context = application_context;
    }
    break;
  case PROP_APPLICATION_MUTEX:
    {
      pthread_mutex_t *application_mutex;

      application_mutex = (pthread_mutex_t *) g_value_get_pointer(value);

      if(midiin->application_mutex == application_mutex){
	return;
      }
      
      midiin->application_mutex = application_mutex;
    }
  case PROP_DEVICE:
    {
      char *device;

      device = (char *) g_value_get_string(value);

      if((AGS_MIDIIN_OSS & (midiin->flags)) != 0){
	midiin->in.oss.device = g_strdup(device);
      }else if((AGS_MIDIIN_ALSA & (midiin->flags)) != 0){
	midiin->in.alsa.device = g_strdup(device);
      }
    }
    break;
  case PROP_BUFFER:
    {
	//TODO:JK: implement me
    }
    break;
  case PROP_BPM:
    {
      gdouble bpm;
      
      bpm = g_value_get_double(value);

      midiin->bpm = bpm;
    }
    break;
  case PROP_DELAY_FACTOR:
    {
      gdouble delay_factor;
      
      delay_factor = g_value_get_double(value);

      midiin->delay_factor = delay_factor;
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_midiin_get_property(GObject *gobject,
			guint prop_id,
			GValue *value,
			GParamSpec *param_spec)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(gobject);
  
  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      g_value_set_object(value, midiin->application_context);
    }
    break;
  case PROP_APPLICATION_MUTEX:
    {
      g_value_set_pointer(value, midiin->application_mutex);
    }
    break;
  case PROP_DEVICE:
    {
      if((AGS_MIDIIN_OSS & (midiin->flags)) != 0){
	g_value_set_string(value, midiin->in.oss.device);
      }else if((AGS_MIDIIN_ALSA & (midiin->flags)) != 0){
	g_value_set_string(value, midiin->in.alsa.device);
      }
    }
    break;
  case PROP_BUFFER:
    {
      g_value_set_pointer(value, midiin->buffer);
    }
    break;
  case PROP_BPM:
    {
      g_value_set_double(value, midiin->bpm);
    }
    break;
  case PROP_DELAY_FACTOR:
    {
      g_value_set_double(value, midiin->delay_factor);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_midiin_finalize(GObject *gobject)
{
  AgsMidiin *midiin;

  AgsMutexManager *mutex_manager;
  
  GList *list, *list_next;

  midiin = AGS_MIDIIN(gobject);

  /* remove midiin mutex */
  pthread_mutex_lock(midiin->application_mutex);
  
  mutex_manager = ags_mutex_manager_get_instance();

  ags_mutex_manager_remove(mutex_manager,
			   gobject);
  
  pthread_mutex_unlock(midiin->application_mutex);

  /* free output buffer */
  if(midiin->buffer[0] != NULL){
    free(midiin->buffer[0]);
  }

  if(midiin->buffer[1] != NULL){
    free(midiin->buffer[1]);
  }
    
  if(midiin->buffer[2] != NULL){
    free(midiin->buffer[2]);
  }
  
  if(midiin->buffer[3] != NULL){
    free(midiin->buffer[3]);
  }
  
  /* free buffer array */
  free(midiin->buffer);

  if(midiin->audio != NULL){
    g_list_free_full(midiin->audio,
		     g_object_unref);
  }
  
  /* call parent */
  G_OBJECT_CLASS(ags_midiin_parent_class)->finalize(gobject);
}

void
ags_midiin_connect(AgsConnectable *connectable)
{
  AgsMidiin *midiin;
  
  AgsMutexManager *mutex_manager;

  GList *list;

  pthread_mutex_t *mutex;
  pthread_mutexattr_t attr;

  midiin = AGS_MIDIIN(connectable);

  /* create midiin mutex */
  //FIXME:JK: memory leak
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr,
			    PTHREAD_MUTEX_RECURSIVE);
  
  mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex,
		     &attr);

  /* insert mutex */
  pthread_mutex_lock(midiin->application_mutex);

  mutex_manager = ags_mutex_manager_get_instance();

  ags_mutex_manager_insert(mutex_manager,
			   (GObject *) midiin,
			   mutex);
  
  pthread_mutex_unlock(midiin->application_mutex);

  /*  */  
  list = midiin->audio;

  while(list != NULL){
    ags_connectable_connect(AGS_CONNECTABLE(list->data));

    list = list->next;
  }
}

void
ags_midiin_disconnect(AgsConnectable *connectable)
{
  //TODO:JK: implement me
}

/**
 * ags_midiin_switch_buffer_flag:
 * @midiin: an #AgsMidiin
 *
 * The buffer flag indicates the currently played buffer.
 *
 * Since: 0.3
 */
void
ags_midiin_switch_buffer_flag(AgsMidiin *midiin)
{
  if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
    midiin->flags &= (~AGS_MIDIIN_BUFFER0);
    midiin->flags |= AGS_MIDIIN_BUFFER1;
  }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
    midiin->flags &= (~AGS_MIDIIN_BUFFER1);
    midiin->flags |= AGS_MIDIIN_BUFFER2;
  }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
    midiin->flags &= (~AGS_MIDIIN_BUFFER2);
    midiin->flags |= AGS_MIDIIN_BUFFER3;
  }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
    midiin->flags &= (~AGS_MIDIIN_BUFFER3);
    midiin->flags |= AGS_MIDIIN_BUFFER0;
  }
}

void
ags_midiin_set_application_context(AgsSequencer *sequencer,
				   AgsApplicationContext *application_context)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);
  midiin->application_context = application_context;
}

AgsApplicationContext*
ags_midiin_get_application_context(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  return(midiin->application_context);
}

void
ags_midiin_set_application_mutex(AgsSequencer *sequencer,
				 pthread_mutex_t *application_mutex)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);
  midiin->application_mutex = application_mutex;
}

pthread_mutex_t*
ags_midiin_get_application_mutex(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  return(midiin->application_mutex);
}

void
ags_midiin_set_device(AgsSequencer *sequencer,
		      gchar *device)
{
  AgsMidiin *midiin;
  
  midiin = AGS_MIDIIN(sequencer);
  midiin->in.alsa.device = device;
}

gchar*
ags_midiin_get_device(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;
  
  midiin = AGS_MIDIIN(sequencer);
  
  return(midiin->in.alsa.device);
}

void
ags_midiin_list_cards(AgsSequencer *sequencer,
		      GList **card_id, GList **card_name)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  if(card_id != NULL){
    *card_id = NULL;
  }

  if(card_name != NULL){
    *card_name = NULL;
  }

  if((AGS_MIDIIN_ALSA & (midiin->flags)) != 0){
#ifdef AGS_WITH_ALSA
    snd_ctl_t *card_handle;
    snd_ctl_card_info_t *card_info;
    char *name;
    gchar *str;
    int card_num;
    int device;
    int error;

    *card_id = NULL;
    *card_name = NULL;
    card_num = -1;

    while(TRUE){
      error = snd_card_next(&card_num);

      if(card_num < 0){
	break;
      }

      if(error < 0){
	continue;
      }

      str = g_strdup_printf("hw:%i\0", card_num);
      error = snd_ctl_open(&card_handle, str, 0);

      if(error < 0){
	continue;
      }

      snd_ctl_card_info_alloca(&card_info);
      error = snd_ctl_card_info(card_handle, card_info);

      if(error < 0){
	continue;
      }

      device = -1;
      error = snd_ctl_rawmidi_next_device(card_handle, &device);
    
      if(error < 0){
	continue;
      }

      *card_id = g_list_prepend(*card_id, str);
      *card_name = g_list_prepend(*card_name, g_strdup(snd_ctl_card_info_get_name(card_info)));

      snd_ctl_close(card_handle);
    }

    snd_config_update_free_global();
#endif
  }else{
    //TODO:JK: implement me
  }

  if(card_id != NULL){
    *card_id = g_list_reverse(*card_id);
  }

  if(card_name != NULL){
    *card_name = g_list_reverse(*card_name);
  }
}

gboolean
ags_midiin_is_starting(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);
  
  return(((AGS_MIDIIN_START_RECORD & (midiin->flags)) != 0) ? TRUE: FALSE);
}

gboolean
ags_midiin_is_recording(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);
  
  return(((AGS_MIDIIN_RECORD & (midiin->flags)) != 0) ? TRUE: FALSE);
}

void
ags_midiin_alsa_init(AgsSequencer *sequencer,
		     GError **error)
{
  AgsMidiin *midiin;

  AgsMutexManager *mutex_manager;

  AgsApplicationContext *application_context;

#ifdef AGS_WITH_ALSA
  int mode = SND_RAWMIDI_NONBLOCK;
  snd_rawmidi_t* handle = NULL;
#endif

  int err;
  
  pthread_mutex_t *mutex;
  
  midiin = AGS_MIDIIN(sequencer);

  application_context = ags_sequencer_get_application_context(sequencer);
  
  pthread_mutex_lock(application_context->mutex);
  
  mutex_manager = ags_mutex_manager_get_instance();

  mutex = ags_mutex_manager_lookup(mutex_manager,
				   (GObject *) midiin);
  
  pthread_mutex_unlock(application_context->mutex);

  /*  */
  pthread_mutex_lock(mutex);

  /* prepare for playback */
  midiin->flags |= (AGS_MIDIIN_BUFFER3 |
		    AGS_MIDIIN_START_RECORD |
		    AGS_MIDIIN_RECORD |
		    AGS_MIDIIN_NONBLOCKING);

  midiin->note_offset = 0;

#ifdef AGS_WITH_ALSA
  mode = SND_RAWMIDI_NONBLOCK;
  
  if((err = snd_rawmidi_open(&handle, NULL, midiin->in.alsa.device, mode)) < 0) {
    pthread_mutex_unlock(mutex);
    printf("Record midi open error: %s\n", snd_strerror(err));
    g_set_error(error,
		AGS_MIDIIN_ERROR,
		AGS_MIDIIN_ERROR_LOCKED_SOUNDCARD,
		"unable to open midi device: %s\n\0",
		snd_strerror(err));

    pthread_mutex_unlock(mutex);

    return;
  }
  
  /*  */
  midiin->in.alsa.handle = handle;
#endif

  midiin->tact_counter = 0.0;
  midiin->delay_counter = 0.0;
  midiin->tic_counter = 0;

  midiin->flags |= AGS_MIDIIN_INITIALIZED;

  pthread_mutex_unlock(mutex);
}

void
ags_midiin_alsa_record(AgsSequencer *sequencer,
		       GError **error)
{
  AgsMidiin *midiin;

  AgsMutexManager *mutex_manager;

  AgsApplicationContext *application_context;

  struct timespec time_start, time_now;

  gdouble delay;
  guint64 time_spent, time_exceed;
  int status;
  gchar c;
  gboolean running;
  
  pthread_mutex_t *mutex;
  
  midiin = AGS_MIDIIN(sequencer);
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  /*  */
  application_context = ags_sequencer_get_application_context(sequencer);
  
  pthread_mutex_lock(application_context->mutex);
  
  mutex_manager = ags_mutex_manager_get_instance();

  mutex = ags_mutex_manager_lookup(mutex_manager,
				   (GObject *) midiin);
  
  pthread_mutex_unlock(application_context->mutex);

  /* do recording */
  pthread_mutex_lock(mutex);

  midiin->flags &= (~AGS_MIDIIN_START_RECORD);

  if((AGS_MIDIIN_INITIALIZED & (midiin->flags)) == 0){
    pthread_mutex_unlock(mutex);
    
    return;
  }

  /* get timing */
  delay = midiin->delay;
  time_exceed = (NSEC_PER_SEC / delay) - midiin->latency;

  pthread_mutex_unlock(mutex);
  
  running = TRUE;
  
  /* check buffer flag */
  while(running){
    pthread_mutex_lock(mutex);
    
    status = 0;

#ifdef AGS_WITH_ALSA
    status = snd_rawmidi_read(midiin->in.alsa.handle, &c, 1);

    if((status < 0) && (status != -EBUSY) && (status != -EAGAIN)){
      g_warning("Problem reading MIDI input: %s", snd_strerror(status));
    }

    if(status >= 0){
      if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
	if(midiin->buffer_size[0] % 256 == 0){
	  if(midiin->buffer[0] == NULL){
	    midiin->buffer[0] = malloc(256 * sizeof(char));
	  }else{
	    midiin->buffer[0] = realloc(midiin->buffer[0],
					(midiin->buffer_size[0] + 256) * sizeof(char));
	  }
	}

	midiin->buffer[0][midiin->buffer_size[0]] = (unsigned char) c;
	midiin->buffer_size[0]++;
      }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
	if(midiin->buffer_size[1] % 256 == 0){
	  if(midiin->buffer[1] == NULL){
	    midiin->buffer[1] = malloc(256 * sizeof(char));
	  }else{
	    midiin->buffer[1] = realloc(midiin->buffer[1],
					(midiin->buffer_size[1] + 256) * sizeof(char));
	  }
	}

	midiin->buffer[1][midiin->buffer_size[1]] = (unsigned char) c;
	midiin->buffer_size[1]++;
      }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
	if(midiin->buffer_size[2] % 256 == 0){
	  if(midiin->buffer[2] == NULL){
	    midiin->buffer[2] = malloc(256 * sizeof(char));
	  }else{
	    midiin->buffer[2] = realloc(midiin->buffer[2],
					(midiin->buffer_size[2] + 256) * sizeof(char));
	  }
	}

	midiin->buffer[2][midiin->buffer_size[2]] = (unsigned char) c;
	midiin->buffer_size[2]++;
      }else if((AGS_MIDIIN_BUFFER3 & midiin->flags) != 0){
	if(midiin->buffer_size[3] % 256 == 0){
	  if(midiin->buffer[3] == NULL){
	    midiin->buffer[3] = malloc(256 * sizeof(char));
	  }else{
	    midiin->buffer[3] = realloc(midiin->buffer[3],
					(midiin->buffer_size[3] + 256) * sizeof(char));
	  }
	}

	midiin->buffer[3][midiin->buffer_size[3]] = (unsigned char) c;
	midiin->buffer_size[3]++;
      }
    }
#endif
    
    pthread_mutex_unlock(mutex);
      
    clock_gettime(CLOCK_MONOTONIC, &time_now);

    if(time_now.tv_sec > time_start.tv_sec){
      time_spent = (time_now.tv_nsec) + (NSEC_PER_SEC - time_start.tv_nsec);
    }else{
      time_spent = time_now.tv_nsec - time_start.tv_nsec;
    }

    if(time_spent > time_exceed){
      running = FALSE;
    }
  }

  pthread_mutex_unlock(mutex);  

  /* tic */
  ags_sequencer_tic(sequencer);

  /* reset - switch buffer flags */
  ags_midiin_switch_buffer_flag(midiin);

  pthread_mutex_unlock(mutex);
}

void
ags_midiin_alsa_free(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  AgsMutexManager *mutex_manager;

  AgsApplicationContext *application_context;

  pthread_mutex_t *mutex;
  
  application_context = ags_sequencer_get_application_context(sequencer);
  
  pthread_mutex_lock(application_context->mutex);
  
  mutex_manager = ags_mutex_manager_get_instance();

  mutex = ags_mutex_manager_lookup(mutex_manager,
				   (GObject *) midiin);
  
  pthread_mutex_unlock(application_context->mutex);

  if((AGS_MIDIIN_INITIALIZED & (midiin->flags)) == 0){
    return;
  }

  midiin = AGS_MIDIIN(sequencer);

#ifdef AGS_WITH_ALSA
  snd_rawmidi_close(midiin->in.alsa.handle);
#endif

  midiin->in.alsa.handle = NULL;
  midiin->flags &= (~(AGS_MIDIIN_BUFFER0 |
		      AGS_MIDIIN_BUFFER1 |
		      AGS_MIDIIN_BUFFER2 |
		      AGS_MIDIIN_BUFFER3 |
		      AGS_MIDIIN_RECORD));

  if(midiin->buffer[1] != NULL){
    free(midiin->buffer[1]);
    midiin->buffer_size[1] = 0;
  }

  if(midiin->buffer[2] != NULL){
    free(midiin->buffer[2]);
    midiin->buffer_size[2] = 0;
  }

  if(midiin->buffer[3] != NULL){
    free(midiin->buffer[3]);
    midiin->buffer_size[3] = 0;
  }

  if(midiin->buffer[0] != NULL){
    free(midiin->buffer[0]);
    midiin->buffer_size[0] = 0;
  }
}

void
ags_midiin_tic(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;
  gdouble delay;
  
  midiin = AGS_MIDIIN(sequencer);

  if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
    if(midiin->buffer[1] != NULL){
      free(midiin->buffer[1]);
      midiin->buffer_size[1] = 0;
    }
  }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
    if(midiin->buffer[2] != NULL){
      free(midiin->buffer[2]);
      midiin->buffer_size[2] = 0;
    }
  }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
    if(midiin->buffer[3] != NULL){
      free(midiin->buffer[3]);
      midiin->buffer_size[3] = 0;
    }
  }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
    if(midiin->buffer[0] != NULL){
      free(midiin->buffer[0]);
      midiin->buffer_size[0] = 0;
    }
  }
}

void
ags_midiin_offset_changed(AgsSequencer *sequencer,
			  guint note_offset)
{
  AgsMidiin *midiin;
  
  midiin = AGS_MIDIIN(sequencer);

  midiin->tic_counter += 1;

  if(midiin->tic_counter == AGS_SEQUENCER_DEFAULT_PERIOD){
    /* reset - tic counter i.e. modified delay index within period */
    midiin->tic_counter = 0;
  }
}

void
ags_midiin_set_bpm(AgsSequencer *sequencer,
		   gdouble bpm)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  midiin->bpm = bpm;
}

gdouble
ags_midiin_get_bpm(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;
  
  midiin = AGS_MIDIIN(sequencer);

  return(midiin->bpm);
}

void
ags_midiin_set_delay_factor(AgsSequencer *sequencer, gdouble delay_factor)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  midiin->delay_factor = delay_factor;
}

gdouble
ags_midiin_get_delay_factor(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;
  
  midiin = AGS_MIDIIN(sequencer);

  return(midiin->delay_factor);
}

void*
ags_midiin_get_buffer(AgsSequencer *sequencer,
		      guint *buffer_length)
{
  AgsMidiin *midiin;
  char *buffer;
  
  midiin = AGS_MIDIIN(sequencer);

  /* get buffer */
  if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
    buffer = midiin->buffer[0];
  }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
    buffer = midiin->buffer[1];
  }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
    buffer = midiin->buffer[2];
  }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
    buffer = midiin->buffer[3];
  }else{
    buffer = NULL;
  }

  /* return the buffer's length */
  if(buffer_length != NULL){
    if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[0];
    }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[1];
    }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[2];
    }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[3];
    }else{
      *buffer_length = 0;
    }
  }
  
  return(buffer);
}

void*
ags_midiin_get_next_buffer(AgsSequencer *sequencer,
			   guint *buffer_length)
{
  AgsMidiin *midiin;
  char *buffer;
  
  midiin = AGS_MIDIIN(sequencer);

  /* get buffer */
  if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
    buffer = midiin->buffer[1];
  }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
    buffer = midiin->buffer[2];
  }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
    buffer = midiin->buffer[3];
  }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
    buffer = midiin->buffer[0];
  }else{
    buffer = NULL;
  }

  /* return the buffer's length */
  if(buffer_length != NULL){
    if((AGS_MIDIIN_BUFFER0 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[1];
    }else if((AGS_MIDIIN_BUFFER1 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[2];
    }else if((AGS_MIDIIN_BUFFER2 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[3];
    }else if((AGS_MIDIIN_BUFFER3 & (midiin->flags)) != 0){
      *buffer_length = midiin->buffer_size[0];
    }else{
      *buffer_length = 0;
    }
  }
  
  return(buffer);
}

void
ags_midiin_set_note_offset(AgsSequencer *sequencer,
			   guint note_offset)
{
  AGS_MIDIIN(sequencer)->note_offset = note_offset;
}

guint
ags_midiin_get_note_offset(AgsSequencer *sequencer)
{
  return(AGS_MIDIIN(sequencer)->note_offset);
}

void
ags_midiin_set_audio(AgsSequencer *sequencer,
		     GList *audio)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);
  midiin->audio = audio;
}

GList*
ags_midiin_get_audio(AgsSequencer *sequencer)
{
  AgsMidiin *midiin;

  midiin = AGS_MIDIIN(sequencer);

  return(midiin->audio);
}

/**
 * ags_midiin_new:
 * @application_context: the #AgsApplicationContext
 *
 * Creates an #AgsMidiin, refering to @application_context.
 *
 * Returns: a new #AgsMidiin
 *
 * Since: 0.7.0
 */
AgsMidiin*
ags_midiin_new(GObject *application_context)
{
  AgsMidiin *midiin;

  midiin = (AgsMidiin *) g_object_new(AGS_TYPE_MIDIIN,
				      "application-context\0", application_context,
				      NULL);
  
  return(midiin);
}
