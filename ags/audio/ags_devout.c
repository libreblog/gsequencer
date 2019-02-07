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

#include <ags/audio/ags_devout.h>

#include <ags/libags.h>

#include <ags/audio/ags_sound_provider.h>
#include <ags/audio/ags_audio_buffer_util.h>

#include <ags/audio/task/ags_tic_device.h>
#include <ags/audio/task/ags_clear_buffer.h>
#include <ags/audio/task/ags_switch_buffer_flag.h>
#include <ags/audio/task/ags_notify_soundcard.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifndef __APPLE__
#include <sys/soundcard.h>
#endif
#include <errno.h>

#define _GNU_SOURCE
#include <signal.h>
#define _GNU_SOURCE
#include <poll.h>

#include <string.h>
#include <math.h>

#include <time.h>
#include <signal.h>
#include <strings.h>
#include <unistd.h>

#include <ags/config.h>
#include <ags/i18n.h>

void ags_devout_class_init(AgsDevoutClass *devout);
void ags_devout_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_devout_soundcard_interface_init(AgsSoundcardInterface *soundcard);
void ags_devout_init(AgsDevout *devout);
void ags_devout_set_property(GObject *gobject,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *param_spec);
void ags_devout_get_property(GObject *gobject,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *param_spec);
void ags_devout_dispose(GObject *gobject);
void ags_devout_finalize(GObject *gobject);

AgsUUID* ags_devout_get_uuid(AgsConnectable *connectable);
gboolean ags_devout_has_resource(AgsConnectable *connectable);
gboolean ags_devout_is_ready(AgsConnectable *connectable);
void ags_devout_add_to_registry(AgsConnectable *connectable);
void ags_devout_remove_from_registry(AgsConnectable *connectable);
xmlNode* ags_devout_list_resource(AgsConnectable *connectable);
xmlNode* ags_devout_xml_compose(AgsConnectable *connectable);
void ags_devout_xml_parse(AgsConnectable *connectable,
			   xmlNode *node);
gboolean ags_devout_is_connected(AgsConnectable *connectable);
void ags_devout_connect(AgsConnectable *connectable);
void ags_devout_disconnect(AgsConnectable *connectable);

void ags_devout_set_application_context(AgsSoundcard *soundcard,
					AgsApplicationContext *application_context);
AgsApplicationContext* ags_devout_get_application_context(AgsSoundcard *soundcard);

void ags_devout_set_device(AgsSoundcard *soundcard,
			   gchar *device);
gchar* ags_devout_get_device(AgsSoundcard *soundcard);

void ags_devout_set_presets(AgsSoundcard *soundcard,
			    guint channels,
			    guint rate,
			    guint buffer_size,
			    guint format);
void ags_devout_get_presets(AgsSoundcard *soundcard,
			    guint *channels,
			    guint *rate,
			    guint *buffer_size,
			    guint *format);

void ags_devout_list_cards(AgsSoundcard *soundcard,
			   GList **card_id, GList **card_name);
void ags_devout_pcm_info(AgsSoundcard *soundcard, gchar *card_id,
			 guint *channels_min, guint *channels_max,
			 guint *rate_min, guint *rate_max,
			 guint *buffer_size_min, guint *buffer_size_max,
			 GError **error);
guint ags_devout_get_capability(AgsSoundcard *soundcard);

GList* ags_devout_get_poll_fd(AgsSoundcard *soundcard);
gboolean ags_devout_is_available(AgsSoundcard *soundcard);

gboolean ags_devout_is_starting(AgsSoundcard *soundcard);
gboolean ags_devout_is_playing(AgsSoundcard *soundcard);

gchar* ags_devout_get_uptime(AgsSoundcard *soundcard);

void ags_devout_delegate_play_init(AgsSoundcard *soundcard,
				   GError **error);
void ags_devout_delegate_play(AgsSoundcard *soundcard,
			      GError **error);
void ags_devout_delegate_stop(AgsSoundcard *soundcard);

void ags_devout_oss_init(AgsSoundcard *soundcard,
			 GError **error);
void ags_devout_oss_play(AgsSoundcard *soundcard,
			 GError **error);
void ags_devout_oss_free(AgsSoundcard *soundcard);

void ags_devout_alsa_init(AgsSoundcard *soundcard,
			  GError **error);
void ags_devout_alsa_play(AgsSoundcard *soundcard,
			  GError **error);
void ags_devout_alsa_free(AgsSoundcard *soundcard);

void ags_devout_tic(AgsSoundcard *soundcard);
void ags_devout_offset_changed(AgsSoundcard *soundcard,
			       guint note_offset);

void ags_devout_set_bpm(AgsSoundcard *soundcard,
			gdouble bpm);
gdouble ags_devout_get_bpm(AgsSoundcard *soundcard);

void ags_devout_set_delay_factor(AgsSoundcard *soundcard,
				 gdouble delay_factor);
gdouble ags_devout_get_delay_factor(AgsSoundcard *soundcard);

gdouble ags_devout_get_absolute_delay(AgsSoundcard *soundcard);

gdouble ags_devout_get_delay(AgsSoundcard *soundcard);
guint ags_devout_get_attack(AgsSoundcard *soundcard);

void* ags_devout_get_buffer(AgsSoundcard *soundcard);
void* ags_devout_get_next_buffer(AgsSoundcard *soundcard);
void* ags_devout_get_prev_buffer(AgsSoundcard *soundcard);

void ags_devout_lock_buffer(AgsSoundcard *soundcard,
			    void *buffer);
void ags_devout_unlock_buffer(AgsSoundcard *soundcard,
			      void *buffer);

guint ags_devout_get_delay_counter(AgsSoundcard *soundcard);

void ags_devout_set_start_note_offset(AgsSoundcard *soundcard,
				      guint start_note_offset);
guint ags_devout_get_start_note_offset(AgsSoundcard *soundcard);

void ags_devout_set_note_offset(AgsSoundcard *soundcard,
				guint note_offset);
guint ags_devout_get_note_offset(AgsSoundcard *soundcard);

void ags_devout_set_note_offset_absolute(AgsSoundcard *soundcard,
					 guint note_offset);
guint ags_devout_get_note_offset_absolute(AgsSoundcard *soundcard);

void ags_devout_set_loop(AgsSoundcard *soundcard,
			 guint loop_left, guint loop_right,
			 gboolean do_loop);
void ags_devout_get_loop(AgsSoundcard *soundcard,
			 guint *loop_left, guint *loop_right,
			 gboolean *do_loop);

guint ags_devout_get_loop_offset(AgsSoundcard *soundcard);

/**
 * SECTION:ags_devout
 * @short_description: Output to soundcard
 * @title: AgsDevout
 * @section_id:
 * @include: ags/audio/ags_devout.h
 *
 * #AgsDevout represents a soundcard and supports output.
 */

enum{
  PROP_0,
  PROP_APPLICATION_CONTEXT,
  PROP_DEVICE,
  PROP_DSP_CHANNELS,
  PROP_PCM_CHANNELS,
  PROP_FORMAT,
  PROP_BUFFER_SIZE,
  PROP_SAMPLERATE,
  PROP_BUFFER,
  PROP_BPM,
  PROP_DELAY_FACTOR,
  PROP_ATTACK,
};

static gpointer ags_devout_parent_class = NULL;

static pthread_mutex_t ags_devout_class_mutex = PTHREAD_MUTEX_INITIALIZER;

GType
ags_devout_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_devout = 0;

    static const GTypeInfo ags_devout_info = {
      sizeof(AgsDevoutClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_devout_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsDevout),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_devout_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_devout_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_soundcard_interface_info = {
      (GInterfaceInitFunc) ags_devout_soundcard_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_devout = g_type_register_static(G_TYPE_OBJECT,
					     "AgsDevout",
					     &ags_devout_info,
					     0);

    g_type_add_interface_static(ags_type_devout,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_devout,
				AGS_TYPE_SOUNDCARD,
				&ags_soundcard_interface_info);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_devout);
  }

  return g_define_type_id__volatile;
}

void
ags_devout_class_init(AgsDevoutClass *devout)
{
  GObjectClass *gobject;

  GParamSpec *param_spec;

  ags_devout_parent_class = g_type_class_peek_parent(devout);

  /* GObjectClass */
  gobject = (GObjectClass *) devout;

  gobject->set_property = ags_devout_set_property;
  gobject->get_property = ags_devout_get_property;

  gobject->dispose = ags_devout_dispose;
  gobject->finalize = ags_devout_finalize;

  /* properties */
  /**
   * AgsDevout:application-context:
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

  /**
   * AgsDevout:device:
   *
   * The alsa soundcard indentifier
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_string("device",
				   i18n_pspec("the device identifier"),
				   i18n_pspec("The device to perform output to"),
				   AGS_DEVOUT_DEFAULT_ALSA_DEVICE,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DEVICE,
				  param_spec);
  
  /**
   * AgsDevout:dsp-channels:
   *
   * The dsp channel count
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_uint("dsp-channels",
				 i18n_pspec("count of DSP channels"),
				 i18n_pspec("The count of DSP channels to use"),
				 AGS_SOUNDCARD_MIN_DSP_CHANNELS,
				 AGS_SOUNDCARD_MAX_DSP_CHANNELS,
				 AGS_SOUNDCARD_DEFAULT_DSP_CHANNELS,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DSP_CHANNELS,
				  param_spec);

  /**
   * AgsDevout:pcm-channels:
   *
   * The pcm channel count
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_uint("pcm-channels",
				 i18n_pspec("count of PCM channels"),
				 i18n_pspec("The count of PCM channels to use"),
				 AGS_SOUNDCARD_MIN_PCM_CHANNELS,
				 AGS_SOUNDCARD_MAX_PCM_CHANNELS,
				 AGS_SOUNDCARD_DEFAULT_PCM_CHANNELS,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_PCM_CHANNELS,
				  param_spec);

  /**
   * AgsDevout:format:
   *
   * The precision of the buffer
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_uint("format",
				 i18n_pspec("precision of buffer"),
				 i18n_pspec("The precision to use for a frame"),
				 0,
				 G_MAXUINT32,
				 AGS_SOUNDCARD_DEFAULT_FORMAT,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_FORMAT,
				  param_spec);

  /**
   * AgsDevout:buffer-size:
   *
   * The buffer size
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_uint("buffer-size",
				 i18n_pspec("frame count of a buffer"),
				 i18n_pspec("The count of frames a buffer contains"),
				 AGS_SOUNDCARD_MIN_BUFFER_SIZE,
				 AGS_SOUNDCARD_MAX_BUFFER_SIZE,
				 AGS_SOUNDCARD_DEFAULT_BUFFER_SIZE,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_BUFFER_SIZE,
				  param_spec);

  /**
   * AgsDevout:samplerate:
   *
   * The samplerate
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_uint("samplerate",
				 i18n_pspec("frames per second"),
				 i18n_pspec("The frames count played during a second"),
				 (guint) AGS_SOUNDCARD_MIN_SAMPLERATE,
				 (guint) AGS_SOUNDCARD_MAX_SAMPLERATE,
				 (guint) AGS_SOUNDCARD_DEFAULT_SAMPLERATE,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_SAMPLERATE,
				  param_spec);

  /**
   * AgsDevout:buffer:
   *
   * The buffer
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_pointer("buffer",
				    i18n_pspec("the buffer"),
				    i18n_pspec("The buffer to play"),
				    G_PARAM_READABLE);
  g_object_class_install_property(gobject,
				  PROP_BUFFER,
				  param_spec);

  /**
   * AgsDevout:bpm:
   *
   * Beats per minute
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_double("bpm",
				   i18n_pspec("beats per minute"),
				   i18n_pspec("Beats per minute to use"),
				   1.0,
				   240.0,
				   AGS_SOUNDCARD_DEFAULT_BPM,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_BPM,
				  param_spec);

  /**
   * AgsDevout:delay-factor:
   *
   * tact
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_double("delay-factor",
				   i18n_pspec("delay factor"),
				   i18n_pspec("The delay factor"),
				   0.0,
				   16.0,
				   1.0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_DELAY_FACTOR,
				  param_spec);

  /**
   * AgsDevout:attack:
   *
   * Attack of the buffer
   * 
   * Since: 2.0.0
   */
  param_spec = g_param_spec_pointer("attack",
				    i18n_pspec("attack of buffer"),
				    i18n_pspec("The attack to use for the buffer"),
				    G_PARAM_READABLE);
  g_object_class_install_property(gobject,
				  PROP_ATTACK,
				  param_spec);
  
  /* AgsDevoutClass */
}

GQuark
ags_devout_error_quark()
{
  return(g_quark_from_static_string("ags-devout-error-quark"));
}

void
ags_devout_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->get_uuid = ags_devout_get_uuid;
  connectable->has_resource = ags_devout_has_resource;

  connectable->is_ready = ags_devout_is_ready;
  connectable->add_to_registry = ags_devout_add_to_registry;
  connectable->remove_from_registry = ags_devout_remove_from_registry;

  connectable->list_resource = ags_devout_list_resource;
  connectable->xml_compose = ags_devout_xml_compose;
  connectable->xml_parse = ags_devout_xml_parse;

  connectable->is_connected = ags_devout_is_connected;  
  connectable->connect = ags_devout_connect;
  connectable->disconnect = ags_devout_disconnect;

  connectable->connect_connection = NULL;
  connectable->disconnect_connection = NULL;
}

void
ags_devout_soundcard_interface_init(AgsSoundcardInterface *soundcard)
{
  soundcard->set_application_context = ags_devout_set_application_context;
  soundcard->get_application_context = ags_devout_get_application_context;

  soundcard->set_device = ags_devout_set_device;
  soundcard->get_device = ags_devout_get_device;
  
  soundcard->set_presets = ags_devout_set_presets;
  soundcard->get_presets = ags_devout_get_presets;

  soundcard->list_cards = ags_devout_list_cards;
  soundcard->pcm_info = ags_devout_pcm_info;
  soundcard->get_capability = ags_devout_get_capability;
  
  soundcard->get_poll_fd = ags_devout_get_poll_fd;
  soundcard->is_available = ags_devout_is_available;

  soundcard->is_starting =  ags_devout_is_starting;
  soundcard->is_playing = ags_devout_is_playing;
  soundcard->is_recording = NULL;

  soundcard->get_uptime = ags_devout_get_uptime;
  
  soundcard->play_init = ags_devout_delegate_play_init;
  soundcard->play = ags_devout_delegate_play;
  
  soundcard->record_init = NULL;
  soundcard->record = NULL;
  
  soundcard->stop = ags_devout_delegate_stop;

  soundcard->tic = ags_devout_tic;
  soundcard->offset_changed = ags_devout_offset_changed;
    
  soundcard->set_bpm = ags_devout_set_bpm;
  soundcard->get_bpm = ags_devout_get_bpm;

  soundcard->set_delay_factor = ags_devout_set_delay_factor;
  soundcard->get_delay_factor = ags_devout_get_delay_factor;

  soundcard->get_absolute_delay = ags_devout_get_absolute_delay;

  soundcard->get_delay = ags_devout_get_delay;
  soundcard->get_attack = ags_devout_get_attack;

  soundcard->get_buffer = ags_devout_get_buffer;
  soundcard->get_next_buffer = ags_devout_get_next_buffer;
  soundcard->get_prev_buffer = ags_devout_get_prev_buffer;

  soundcard->lock_buffer = ags_devout_lock_buffer;
  soundcard->unlock_buffer = ags_devout_unlock_buffer;

  soundcard->get_delay_counter = ags_devout_get_delay_counter;

  soundcard->set_start_note_offset = ags_devout_set_start_note_offset;
  soundcard->get_start_note_offset = ags_devout_get_start_note_offset;

  soundcard->set_note_offset = ags_devout_set_note_offset;
  soundcard->get_note_offset = ags_devout_get_note_offset;

  soundcard->set_note_offset_absolute = ags_devout_set_note_offset_absolute;
  soundcard->get_note_offset_absolute = ags_devout_get_note_offset_absolute;

  soundcard->set_loop = ags_devout_set_loop;
  soundcard->get_loop = ags_devout_get_loop;

  soundcard->get_loop_offset = ags_devout_get_loop_offset;
}

void
ags_devout_init(AgsDevout *devout)
{  
  AgsConfig *config;
  
  gchar *str;
  gchar *segmentation;

  guint i;
  guint denumerator, numerator;
  gboolean use_alsa;  
  
  pthread_mutex_t *mutex;
  pthread_mutexattr_t *attr;

  devout->flags = 0;
  
  /* insert devout mutex */
  devout->obj_mutexattr = 
    attr = (pthread_mutexattr_t *) malloc(sizeof(pthread_mutexattr_t));
  pthread_mutexattr_init(attr);
  pthread_mutexattr_settype(attr,
			    PTHREAD_MUTEX_RECURSIVE);

#ifdef __linux__
  pthread_mutexattr_setprotocol(attr,
				PTHREAD_PRIO_INHERIT);
#endif

  devout->obj_mutex = 
    mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex,
		     attr);

  /* parent */
  devout->application_context = NULL;

  /* uuid */
  devout->uuid = ags_uuid_alloc();
  ags_uuid_generate(devout->uuid);

  /* flags */
  config = ags_config_get_instance();

#ifdef AGS_WITH_ALSA
  use_alsa = TRUE;
#else
  use_alsa = FALSE;
#endif

  str = ags_config_get_value(config,
			     AGS_CONFIG_SOUNDCARD,
			     "backend");

  if(str == NULL){
    str = ags_config_get_value(config,
			       AGS_CONFIG_SOUNDCARD_0,
			       "backend");
  }
  
  if(str != NULL &&
     !g_ascii_strncasecmp(str,
			  "oss",
			  4)){
    use_alsa = FALSE;
  }

  if(use_alsa){
    devout->flags |= (AGS_DEVOUT_ALSA);
  }else{
    devout->flags |= (AGS_DEVOUT_OSS);
  }

  g_free(str);

  /* presets */
  devout->dsp_channels = ags_soundcard_helper_config_get_dsp_channels(config);
  devout->pcm_channels = ags_soundcard_helper_config_get_pcm_channels(config);

  devout->samplerate = ags_soundcard_helper_config_get_samplerate(config);
  devout->buffer_size = ags_soundcard_helper_config_get_buffer_size(config);
  devout->format = ags_soundcard_helper_config_get_format(config);

  /* device */
  if(use_alsa){
    devout->out.alsa.handle = NULL;
    devout->out.alsa.device = AGS_DEVOUT_DEFAULT_ALSA_DEVICE;
  }else{
    devout->out.oss.device_fd = -1;
    devout->out.oss.device = AGS_DEVOUT_DEFAULT_OSS_DEVICE;
  }

  /* buffer */
  devout->buffer_mutex = (pthread_mutex_t **) malloc(4 * sizeof(pthread_mutex_t *));

  for(i = 0; i < 4; i++){
    devout->buffer_mutex[i] = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));

    pthread_mutex_init(devout->buffer_mutex[i],
		       NULL);
  }
  
  devout->buffer = (void **) malloc(4 * sizeof(void *));

  devout->buffer[0] = NULL;
  devout->buffer[1] = NULL;
  devout->buffer[2] = NULL;
  devout->buffer[3] = NULL;

  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  devout->ring_buffer_size = AGS_DEVOUT_DEFAULT_RING_BUFFER_SIZE;
  devout->nth_ring_buffer = 0;
  
  devout->ring_buffer = NULL;

  ags_devout_realloc_buffer(devout);
  
  /* bpm */
  devout->bpm = AGS_SOUNDCARD_DEFAULT_BPM;

  /* delay factor */
  devout->delay_factor = AGS_SOUNDCARD_DEFAULT_DELAY_FACTOR;
  
  /* segmentation */
  segmentation = ags_config_get_value(config,
				      AGS_CONFIG_GENERIC,
				      "segmentation");

  if(segmentation != NULL){
    sscanf(segmentation, "%d/%d",
	   &denumerator,
	   &numerator);
    
    devout->delay_factor = 1.0 / numerator * (numerator / denumerator);

    g_free(segmentation);
  }

  /* delay and attack */
  devout->delay = (gdouble *) malloc((int) 2 * AGS_SOUNDCARD_DEFAULT_PERIOD *
				     sizeof(gdouble));
  
  devout->attack = (guint *) malloc((int) 2 * AGS_SOUNDCARD_DEFAULT_PERIOD *
				    sizeof(guint));

  ags_devout_adjust_delay_and_attack(devout);
  
  /* counters */
  devout->tact_counter = 0.0;
  devout->delay_counter = 0;
  devout->tic_counter = 0;

  devout->start_note_offset = 0;
  devout->note_offset = 0;
  devout->note_offset_absolute = 0;

  devout->loop_left = AGS_SOUNDCARD_DEFAULT_LOOP_LEFT;
  devout->loop_right = AGS_SOUNDCARD_DEFAULT_LOOP_RIGHT;

  devout->do_loop = FALSE;

  devout->loop_offset = 0;
  
  /* poll fd and notify task */
  devout->poll_fd = NULL;
  devout->notify_soundcard = NULL;
}

void
ags_devout_set_property(GObject *gobject,
			guint prop_id,
			const GValue *value,
			GParamSpec *param_spec)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(gobject);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());
  
  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      AgsApplicationContext *application_context;

      application_context = (AgsApplicationContext *) g_value_get_object(value);

      pthread_mutex_lock(devout_mutex);

      if(devout->application_context == application_context){
	pthread_mutex_unlock(devout_mutex);

	return;
      }

      if(devout->application_context != NULL){
	g_object_unref(G_OBJECT(devout->application_context));
      }

      if(application_context != NULL){	
	g_object_ref(G_OBJECT(application_context));
      }

      devout->application_context = application_context;

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_DEVICE:
    {
      char *device;

      device = (char *) g_value_get_string(value);

      pthread_mutex_lock(devout_mutex);

      if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
	devout->out.oss.device = g_strdup(device);
      }else if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
	devout->out.alsa.device = g_strdup(device);
      }

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_DSP_CHANNELS:
    {
      guint dsp_channels;

      dsp_channels = g_value_get_uint(value);

      pthread_mutex_lock(devout_mutex);

      if(dsp_channels == devout->dsp_channels){
	pthread_mutex_unlock(devout_mutex);
	
	return;
      }

      devout->dsp_channels = dsp_channels;

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_PCM_CHANNELS:
    {
      guint pcm_channels;

      pcm_channels = g_value_get_uint(value);

      pthread_mutex_lock(devout_mutex);

      if(pcm_channels == devout->pcm_channels){
	pthread_mutex_unlock(devout_mutex);
	
	return;
      }

      devout->pcm_channels = pcm_channels;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_realloc_buffer(devout);
    }
    break;
  case PROP_FORMAT:
    {
      guint format;

      format = g_value_get_uint(value);

      pthread_mutex_lock(devout_mutex);

      if(format == devout->format){
	pthread_mutex_unlock(devout_mutex);
	
	return;
      }

      devout->format = format;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_realloc_buffer(devout);
    }
    break;
  case PROP_BUFFER_SIZE:
    {
      guint buffer_size;

      buffer_size = g_value_get_uint(value);

      pthread_mutex_lock(devout_mutex);

      if(buffer_size == devout->buffer_size){
	pthread_mutex_unlock(devout_mutex);

	return;
      }

      devout->buffer_size = buffer_size;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_realloc_buffer(devout);
      ags_devout_adjust_delay_and_attack(devout);
    }
    break;
  case PROP_SAMPLERATE:
    {
      guint samplerate;

      samplerate = g_value_get_uint(value);

      pthread_mutex_lock(devout_mutex);

      if(samplerate == devout->samplerate){
	pthread_mutex_unlock(devout_mutex);

	return;
      }

      devout->samplerate = samplerate;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_adjust_delay_and_attack(devout);
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

      pthread_mutex_lock(devout_mutex);

      if(bpm == devout->bpm){
	pthread_mutex_unlock(devout_mutex);

	return;
      }

      devout->bpm = bpm;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_adjust_delay_and_attack(devout);
    }
    break;
  case PROP_DELAY_FACTOR:
    {
      gdouble delay_factor;
      
      delay_factor = g_value_get_double(value);

      pthread_mutex_lock(devout_mutex);

      if(delay_factor == devout->delay_factor){
	pthread_mutex_unlock(devout_mutex);

	return;
      }

      devout->delay_factor = delay_factor;

      pthread_mutex_unlock(devout_mutex);

      ags_devout_adjust_delay_and_attack(devout);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_devout_get_property(GObject *gobject,
			guint prop_id,
			GValue *value,
			GParamSpec *param_spec)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(gobject);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());
  
  switch(prop_id){
  case PROP_APPLICATION_CONTEXT:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_object(value, devout->application_context);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_DEVICE:
    {
      pthread_mutex_lock(devout_mutex);

      if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
	g_value_set_string(value, devout->out.oss.device);
      }else if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
	g_value_set_string(value, devout->out.alsa.device);
      }

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_DSP_CHANNELS:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_uint(value, devout->dsp_channels);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_PCM_CHANNELS:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_uint(value, devout->pcm_channels);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_FORMAT:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_uint(value, devout->format);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_BUFFER_SIZE:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_uint(value, devout->buffer_size);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_SAMPLERATE:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_uint(value, devout->samplerate);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_BUFFER:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_pointer(value, devout->buffer);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_BPM:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_double(value, devout->bpm);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_DELAY_FACTOR:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_double(value, devout->delay_factor);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  case PROP_ATTACK:
    {
      pthread_mutex_lock(devout_mutex);

      g_value_set_pointer(value, devout->attack);

      pthread_mutex_unlock(devout_mutex);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_devout_dispose(GObject *gobject)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(gobject);
  
  /* notify soundcard */
  if(devout->notify_soundcard != NULL){
    if(devout->application_context != NULL){
      AgsTaskThread *task_thread;
    
      task_thread = ags_concurrency_provider_get_task_thread(AGS_CONCURRENCY_PROVIDER(ags_application_context_get_instance()));
      
      ags_task_thread_remove_cyclic_task(task_thread,
					 (AgsTask *) devout->notify_soundcard);
    }

    g_object_unref(devout->notify_soundcard);

    devout->notify_soundcard = NULL;
  }

  /* application context */
  if(devout->application_context != NULL){
    g_object_unref(devout->application_context);

    devout->application_context = NULL;
  }

  /* call parent */
  G_OBJECT_CLASS(ags_devout_parent_class)->dispose(gobject);
}

void
ags_devout_finalize(GObject *gobject)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(gobject);

  /* mutex */
  pthread_mutex_destroy(devout->obj_mutex);
  free(devout->obj_mutex);

  pthread_mutexattr_destroy(devout->obj_mutexattr);
  free(devout->obj_mutexattr);

  ags_uuid_free(devout->uuid);
  
  /* free output buffer */
  free(devout->buffer[0]);
  free(devout->buffer[1]);
  free(devout->buffer[2]);
  free(devout->buffer[3]);

  /* free buffer array */
  free(devout->buffer);

  /* free AgsAttack */
  free(devout->attack);

  /* notify soundcard */
  if(devout->notify_soundcard != NULL){
    if(devout->application_context != NULL){
      AgsTaskThread *task_thread;
      
      task_thread = ags_concurrency_provider_get_task_thread(AGS_CONCURRENCY_PROVIDER(ags_application_context_get_instance()));
      
      ags_task_thread_remove_cyclic_task(task_thread,
					 (AgsTask *) devout->notify_soundcard);
    }

    g_object_unref(devout->notify_soundcard);
  }

  /* application context */
  if(devout->application_context != NULL){
    g_object_unref(devout->application_context);
  }
  
  /* call parent */
  G_OBJECT_CLASS(ags_devout_parent_class)->finalize(gobject);
}

AgsUUID*
ags_devout_get_uuid(AgsConnectable *connectable)
{
  AgsDevout *devout;
  
  AgsUUID *ptr;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(connectable);

  /* get devout signal mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get UUID */
  pthread_mutex_lock(devout_mutex);

  ptr = devout->uuid;

  pthread_mutex_unlock(devout_mutex);
  
  return(ptr);
}

gboolean
ags_devout_has_resource(AgsConnectable *connectable)
{
  return(FALSE);
}

gboolean
ags_devout_is_ready(AgsConnectable *connectable)
{
  AgsDevout *devout;
  
  gboolean is_ready;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(connectable);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* check is added */
  pthread_mutex_lock(devout_mutex);

  is_ready = (((AGS_DEVOUT_ADDED_TO_REGISTRY & (devout->flags)) != 0) ? TRUE: FALSE);

  pthread_mutex_unlock(devout_mutex);
  
  return(is_ready);
}

void
ags_devout_add_to_registry(AgsConnectable *connectable)
{
  AgsDevout *devout;

  if(ags_connectable_is_ready(connectable)){
    return;
  }
  
  devout = AGS_DEVOUT(connectable);

  ags_devout_set_flags(devout, AGS_DEVOUT_ADDED_TO_REGISTRY);
}

void
ags_devout_remove_from_registry(AgsConnectable *connectable)
{
  AgsDevout *devout;

  if(!ags_connectable_is_ready(connectable)){
    return;
  }

  devout = AGS_DEVOUT(connectable);

  ags_devout_unset_flags(devout, AGS_DEVOUT_ADDED_TO_REGISTRY);
}

xmlNode*
ags_devout_list_resource(AgsConnectable *connectable)
{
  xmlNode *node;
  
  node = NULL;

  //TODO:JK: implement me
  
  return(node);
}

xmlNode*
ags_devout_xml_compose(AgsConnectable *connectable)
{
  xmlNode *node;
  
  node = NULL;

  //TODO:JK: implement me
  
  return(node);
}

void
ags_devout_xml_parse(AgsConnectable *connectable,
		      xmlNode *node)
{
  //TODO:JK: implement me
}

gboolean
ags_devout_is_connected(AgsConnectable *connectable)
{
  AgsDevout *devout;
  
  gboolean is_connected;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(connectable);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* check is connected */
  pthread_mutex_lock(devout_mutex);

  is_connected = (((AGS_DEVOUT_CONNECTED & (devout->flags)) != 0) ? TRUE: FALSE);
  
  pthread_mutex_unlock(devout_mutex);
  
  return(is_connected);
}

void
ags_devout_connect(AgsConnectable *connectable)
{
  AgsDevout *devout;
  
  if(ags_connectable_is_connected(connectable)){
    return;
  }

  devout = AGS_DEVOUT(connectable);

  ags_devout_set_flags(devout, AGS_DEVOUT_CONNECTED);
}

void
ags_devout_disconnect(AgsConnectable *connectable)
{

  AgsDevout *devout;

  if(!ags_connectable_is_connected(connectable)){
    return;
  }

  devout = AGS_DEVOUT(connectable);
  
  ags_devout_unset_flags(devout, AGS_DEVOUT_CONNECTED);
}

/**
 * ags_devout_get_class_mutex:
 * 
 * Use this function's returned mutex to access mutex fields.
 *
 * Returns: the class mutex
 * 
 * Since: 2.0.0
 */
pthread_mutex_t*
ags_devout_get_class_mutex()
{
  return(&ags_devout_class_mutex);
}

/**
 * ags_devout_test_flags:
 * @devout: the #AgsDevout
 * @flags: the flags
 *
 * Test @flags to be set on @devout.
 * 
 * Returns: %TRUE if flags are set, else %FALSE
 *
 * Since: 2.0.0
 */
gboolean
ags_devout_test_flags(AgsDevout *devout, guint flags)
{
  gboolean retval;  
  
  pthread_mutex_t *devout_mutex;

  if(!AGS_IS_DEVOUT(devout)){
    return(FALSE);
  }

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* test */
  pthread_mutex_lock(devout_mutex);

  retval = (flags & (devout->flags)) ? TRUE: FALSE;
  
  pthread_mutex_unlock(devout_mutex);

  return(retval);
}

/**
 * ags_devout_set_flags:
 * @devout: the #AgsDevout
 * @flags: see #AgsDevoutFlags-enum
 *
 * Enable a feature of @devout.
 *
 * Since: 2.0.0
 */
void
ags_devout_set_flags(AgsDevout *devout, guint flags)
{
  pthread_mutex_t *devout_mutex;

  if(!AGS_IS_DEVOUT(devout)){
    return;
  }

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  //TODO:JK: add more?

  /* set flags */
  pthread_mutex_lock(devout_mutex);

  devout->flags |= flags;
  
  pthread_mutex_unlock(devout_mutex);
}
    
/**
 * ags_devout_unset_flags:
 * @devout: the #AgsDevout
 * @flags: see #AgsDevoutFlags-enum
 *
 * Disable a feature of @devout.
 *
 * Since: 2.0.0
 */
void
ags_devout_unset_flags(AgsDevout *devout, guint flags)
{  
  pthread_mutex_t *devout_mutex;

  if(!AGS_IS_DEVOUT(devout)){
    return;
  }

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  //TODO:JK: add more?

  /* unset flags */
  pthread_mutex_lock(devout_mutex);

  devout->flags &= (~flags);
  
  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_set_application_context(AgsSoundcard *soundcard,
				   AgsApplicationContext *application_context)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set application context */
  pthread_mutex_lock(devout_mutex);
  
  devout->application_context = application_context;
  
  pthread_mutex_unlock(devout_mutex);
}

AgsApplicationContext*
ags_devout_get_application_context(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  AgsApplicationContext *application_context;
  
  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get application context */
  pthread_mutex_lock(devout_mutex);

  application_context = devout->application_context;

  pthread_mutex_unlock(devout_mutex);
  
  return(application_context);
}

void
ags_devout_set_device(AgsSoundcard *soundcard,
		      gchar *device)
{
  AgsDevout *devout;

  GList *card_id, *card_id_start, *card_name, *card_name_start;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* list cards */
  card_id = NULL;
  card_name = NULL;
  
  ags_soundcard_list_cards(soundcard,
			   &card_id, &card_name);

  card_id_start = card_id;
  card_name_start = card_name;

  /* check card */
  pthread_mutex_lock(devout_mutex);

  while(card_id != NULL){
    if(!g_ascii_strncasecmp(card_id->data,
			    device,
			    strlen(card_id->data))){
      if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
	devout->out.alsa.device = g_strdup(device);
      }else if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
	devout->out.oss.device = g_strdup(device);
      }

      break;
    }
    
    card_id = card_id->next;
  }

  pthread_mutex_unlock(devout_mutex);

  /* free card id and name */
  g_list_free_full(card_id_start,
		   g_free);
  g_list_free_full(card_name_start,
		   g_free);
}

gchar*
ags_devout_get_device(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gchar *device;

  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  device = NULL;
  
  pthread_mutex_lock(devout_mutex);

  if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
    device = g_strdup(devout->out.alsa.device);
  }else if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
    device = g_strdup(devout->out.oss.device);
  }

  pthread_mutex_unlock(devout_mutex);

  return(device);
}

void
ags_devout_set_presets(AgsSoundcard *soundcard,
		       guint channels,
		       guint samplerate,
		       guint buffer_size,
		       guint format)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(soundcard);

  g_object_set(devout,
	       "pcm-channels", channels,
	       "samplerate", samplerate,
	       "buffer-size", buffer_size,
	       "format", format,
	       NULL);
}

void
ags_devout_get_presets(AgsSoundcard *soundcard,
		       guint *channels,
		       guint *samplerate,
		       guint *buffer_size,
		       guint *format)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get presets */
  pthread_mutex_lock(devout_mutex);

  if(channels != NULL){
    *channels = devout->pcm_channels;
  }

  if(samplerate != NULL){
    *samplerate = devout->samplerate;
  }

  if(buffer_size != NULL){
    *buffer_size = devout->buffer_size;
  }

  if(format != NULL){
    *format = devout->format;
  }

  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_list_cards(AgsSoundcard *soundcard,
		      GList **card_id, GList **card_name)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  if(card_id != NULL){
    *card_id = NULL;
  }

  if(card_name != NULL){
    *card_name = NULL;
  }

  pthread_mutex_lock(devout_mutex);

  if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
#ifdef AGS_WITH_ALSA
    snd_ctl_t *card_handle;
    snd_ctl_card_info_t *card_info;

    char *name;
    gchar *str;
    gchar *str_err;
  
    int card_num;
    int device;
    int error;

    /* the default device */
    str = g_strdup("default");
    error = snd_ctl_open(&card_handle, str, 0);

    if(error < 0){
      g_free(str);
      
      goto ags_devout_list_cards_NO_DEFAULT_0;
    }

    snd_ctl_card_info_alloca(&card_info);
    error = snd_ctl_card_info(card_handle, card_info);

    if(error < 0){
      g_free(str);

      goto ags_devout_list_cards_NO_DEFAULT_0;
    }

    if(card_id != NULL){
      *card_id = g_list_prepend(*card_id, str);
    }

    if(card_name != NULL){
      *card_name = g_list_prepend(*card_name, g_strdup(snd_ctl_card_info_get_name(card_info)));
    }
    
    snd_ctl_close(card_handle);

  ags_devout_list_cards_NO_DEFAULT_0:
    
    /* enumerated devices */
    card_num = -1;

    while(TRUE){
      error = snd_card_next(&card_num);

      if(card_num < 0 || error < 0){
	str_err = snd_strerror(error);
	g_message("Can't get the next card number: %s", str_err);

	//free(str_err);
      
	break;
      }

      str = g_strdup_printf("hw:%d", card_num);

#ifdef AGS_DEBUG
      g_message("found soundcard - %s", str);
#endif
    
      error = snd_ctl_open(&card_handle, str, 0);

      if(error < 0){
	g_free(str);
      
	continue;
      }

      snd_ctl_card_info_alloca(&card_info);
      error = snd_ctl_card_info(card_handle, card_info);

      if(error < 0){
	g_free(str);
      
	continue;
      }

      device = -1;
      error = snd_ctl_pcm_next_device(card_handle, &device);

      if(error < 0){
	g_free(str);
      
	continue;
      }

      if(card_id != NULL){
	*card_id = g_list_prepend(*card_id, str);
      }

      if(card_name != NULL){
	*card_name = g_list_prepend(*card_name, g_strdup(snd_ctl_card_info_get_name(card_info)));
      }
    
      snd_ctl_close(card_handle);
    }

    snd_config_update_free_global();
#endif
  }else{
#ifdef AGS_WITH_OSS
    oss_sysinfo sysinfo;
    oss_audioinfo ai;

    char *mixer_device;
    
    int mixerfd = -1;

    int next, n;
    int i;

    if((mixer_device = getenv("OSS_MIXERDEV")) == NULL){
      mixer_device = "/dev/mixer";
    }

    if((mixerfd = open(mixer_device, O_RDONLY, 0)) == -1){
      int e = errno;
      
      switch(e){
      case ENXIO:
      case ENODEV:
	{
	  g_warning("Open Sound System is not running in your system.");
	}
	break;
      case ENOENT:
	{
	  g_warning("No %s device available in your system.\nPerhaps Open Sound System is not installed or running.", mixer_device);
	}
	break;  
      default:
	g_warning("%s", strerror(e));
      }
    }
      
    if(ioctl(mixerfd, SNDCTL_SYSINFO, &sysinfo) == -1){
      if(errno == ENXIO){
	g_warning("OSS has not detected any supported sound hardware in your system.");
      }else{
	g_warning("SNDCTL_SYSINFO");

	if(errno == EINVAL){
	  g_warning("Error: OSS version 4.0 or later is required");
	}
      }

      n = 0;
    }else{
      n = sysinfo.numaudios;
    }

    memset(&ai, 0, sizeof(oss_audioinfo));
    ioctl(mixerfd, SNDCTL_AUDIOINFO_EX, &ai);

    for(i = 0; i < n; i++){
      ai.dev = i;

      if(ioctl(mixerfd, SNDCTL_ENGINEINFO, &ai) == -1){
	int e = errno;
	
	g_warning("Can't get device info for /dev/dsp%d (SNDCTL_AUDIOINFO)\nerrno = %d: %s", i, e, strerror(e));
	
	continue;
      }
      
      if((DSP_CAP_OUTPUT & (ai.caps)) != 0){
	if(card_id != NULL){
	  *card_id = g_list_prepend(*card_id,
				    g_strdup_printf("/dev/dsp%i", i));
	}
	
	if(card_name != NULL){
	  *card_name = g_list_prepend(*card_name,
				      g_strdup(ai.name));
	}
      }

      next = ai.next_play_engine;
      
      if(next <= 0){
	break;
      }
    }
#endif
  }

  pthread_mutex_unlock(devout_mutex);

  if(card_id != NULL){
    *card_id = g_list_reverse(*card_id);
  }

  if(card_name != NULL){
    *card_name = g_list_reverse(*card_name);
  }
}

void
ags_devout_pcm_info(AgsSoundcard *soundcard,
		    char *card_id,
		    guint *channels_min, guint *channels_max,
		    guint *rate_min, guint *rate_max,
		    guint *buffer_size_min, guint *buffer_size_max,
		    GError **error)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;

  if(card_id == NULL){
    return;
  }
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* pcm info */
  pthread_mutex_lock(devout_mutex);

  if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
#ifdef AGS_WITH_ALSA
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    
    gchar *str;
    
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;

    int rc;
    int err;

    /* Open PCM device for playback. */
    handle = NULL;

    rc = snd_pcm_open(&handle, card_id, SND_PCM_STREAM_PLAYBACK, 0);

    if(rc < 0){      
      str = snd_strerror(rc);
      g_message("unable to open pcm device (attempting fixup): %s", str);

      if(index(card_id,
	       ',') != NULL){
	gchar *device_fixup;
	
	device_fixup = g_strndup(card_id,
				 index(card_id,
				       ',') - card_id);
	handle = NULL;
	
	rc = snd_pcm_open(&handle, device_fixup, SND_PCM_STREAM_PLAYBACK, 0);
      
	if(rc < 0){
	  if(error != NULL){
	    g_set_error(error,
			AGS_DEVOUT_ERROR,
			AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
			"unable to open pcm device: %s\n",
			str);
	  }
	  
	  //    free(str);
	  
	  goto ags_devout_pcm_info_ERR;
	}
      }else{
	if(error != NULL){
	  g_set_error(error,
		      AGS_DEVOUT_ERROR,
		      AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
		      "unable to open pcm device: %s\n",
		      str);
	}
	
	goto ags_devout_pcm_info_ERR;
      }
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* channels */
    snd_pcm_hw_params_get_channels_min(params, &val);
    *channels_min = val;

    snd_pcm_hw_params_get_channels_max(params, &val);
    *channels_max = val;

    /* samplerate */
    dir = 0;
    snd_pcm_hw_params_get_rate_min(params, &val, &dir);
    *rate_min = val;

    dir = 0;
    snd_pcm_hw_params_get_rate_max(params, &val, &dir);
    *rate_max = val;

    /* buffer size */
    dir = 0;
    snd_pcm_hw_params_get_buffer_size_min(params, &frames);
    *buffer_size_min = frames;

    dir = 0;
    snd_pcm_hw_params_get_buffer_size_max(params, &frames);
    *buffer_size_max = frames;

    snd_pcm_close(handle);
#endif
  }else{
#ifdef AGS_WITH_OSS
    oss_audioinfo ainfo;

    gchar *str;
    
    int mixerfd;
    int acc;
    unsigned int cmd;
    
    mixerfd = open(card_id, O_RDWR, 0);

    if(mixerfd == -1){
      int e = errno;
      
      str = strerror(e);
      g_message("unable to open pcm device: %s\n", str);

      if(error != NULL){
	g_set_error(error,
		    AGS_DEVOUT_ERROR,
		    AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
		    "unable to open pcm device: %s\n",
		    str);
      }
    
      goto ags_devout_pcm_info_ERR;      
    }
    
    memset(&ainfo, 0, sizeof (ainfo));
    
    cmd = SNDCTL_AUDIOINFO;

    if(card_id != NULL &&
       !g_ascii_strncasecmp(card_id,
			    "/dev/dsp",
			    8)){
      if(strlen(card_id) > 8){
	sscanf(card_id,
	       "/dev/dsp%d",
	       &(ainfo.dev));
      }else{
	ainfo.dev = 0;
      }
    }else{
      goto ags_devout_pcm_info_ERR;
    }
    
    if(ioctl(mixerfd, cmd, &ainfo) == -1){
      int e = errno;

      str = strerror(e);
      g_message("unable to retrieve audio info: %s\n", str);

      if(error != NULL){
	g_set_error(error,
		    AGS_DEVOUT_ERROR,
		    AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
		    "unable to retrieve audio info: %s\n",
		    str);
      }
    
      return;
    }
  
    *channels_min = ainfo.min_channels;
    *channels_max = ainfo.max_channels;
    *rate_min = ainfo.min_rate;
    *rate_max = ainfo.max_rate;
    *buffer_size_min = 64;
    *buffer_size_max = 8192;
#endif
  }

 ags_devout_pcm_info_ERR:

  pthread_mutex_unlock(devout_mutex);
}

guint
ags_devout_get_capability(AgsSoundcard *soundcard)
{
  return(AGS_SOUNDCARD_CAPABILITY_PLAYBACK);
}

GList*
ags_devout_get_poll_fd(AgsSoundcard *soundcard)
{
  AgsDevout *devout;
  AgsPollFd *poll_fd;

  GList *list;

  struct pollfd *fds;
  
  gint count;
  guint i;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get poll fd */
  pthread_mutex_lock(devout_mutex);

  if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
    if(devout->out.alsa.handle == NULL){
      pthread_mutex_unlock(devout_mutex);
      
      return(NULL);
    }
  }else if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
    if(devout->out.oss.device_fd == -1){
      pthread_mutex_unlock(devout_mutex);
      
      return(NULL);
    }
  }
  
  if(devout->poll_fd == NULL){
    count = 0;
    
    if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
#ifdef AGS_WITH_ALSA
      /* get poll fds of ALSA */
      count = snd_pcm_poll_descriptors_count(devout->out.alsa.handle);

      if(count > 0){
	fds = (struct pollfd *) malloc(count * sizeof(struct pollfd));
	snd_pcm_poll_descriptors(devout->out.alsa.handle, fds, count);
      }
#endif
    }else if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
      if(devout->out.oss.device_fd != -1){
	count = 1;
	fds = (struct pollfd *) malloc(sizeof(struct pollfd));
	fds->fd = devout->out.oss.device_fd;
      }
    }
    
    /* map fds */
    list = NULL;

    for(i = 0; i < count; i++){
      poll_fd = ags_poll_fd_new();
      poll_fd->fd = fds[i].fd;
      poll_fd->poll_fd = &(fds[i]);
      
      list = g_list_prepend(list,
			    poll_fd);
    }

    devout->poll_fd = list;
  }else{
    list = devout->poll_fd;
  }
  
  pthread_mutex_unlock(devout_mutex);

  return(list);
}

gboolean
ags_devout_is_available(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  GList *list;

  gboolean retval;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* check available */
  pthread_mutex_lock(devout_mutex);

  list = devout->poll_fd;

  retval = FALSE;
  
  while(list !=	NULL){
    gint16 revents;

    if((AGS_DEVOUT_ALSA & (devout->flags)) != 0){
      revents = 0;
      
#ifdef AGS_WITH_ALSA
      snd_pcm_poll_descriptors_revents(devout->out.alsa.handle, AGS_POLL_FD(list->data)->poll_fd, 1, &revents);
#endif
      
      if((POLLOUT & revents) != 0){
	g_atomic_int_set(&(devout->available),
			 TRUE);
	AGS_POLL_FD(list->data)->poll_fd->revents = 0;

	retval = TRUE;

	break;
      }
    }else if((AGS_DEVOUT_OSS & (devout->flags)) != 0){
#ifdef AGS_WITH_OSS
      fd_set writefds;

      FD_ZERO(&writefds);
      FD_SET(AGS_POLL_FD(list->data)->poll_fd->fd, &writefds);
      
      if(FD_ISSET(AGS_POLL_FD(list->data)->poll_fd->fd, &writefds)){
	g_atomic_int_set(&(devout->available),
			 TRUE);
	AGS_POLL_FD(list->data)->poll_fd->revents = 0;

	retval = TRUE;

	break;
      }
#endif
    }
    
    list = list->next;
  }
  
  pthread_mutex_unlock(devout_mutex);
  
  return(retval);
}

gboolean
ags_devout_is_starting(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gboolean is_starting;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* check is starting */
  pthread_mutex_lock(devout_mutex);

  is_starting = ((AGS_DEVOUT_START_PLAY & (devout->flags)) != 0) ? TRUE: FALSE;

  pthread_mutex_unlock(devout_mutex);
  
  return(is_starting);
}

gboolean
ags_devout_is_playing(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gboolean is_playing;
  
  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* check is starting */
  pthread_mutex_lock(devout_mutex);

  is_playing = ((AGS_DEVOUT_PLAY & (devout->flags)) != 0) ? TRUE: FALSE;

  pthread_mutex_unlock(devout_mutex);

  return(is_playing);
}

gchar*
ags_devout_get_uptime(AgsSoundcard *soundcard)
{
  gchar *uptime;

  if(ags_soundcard_is_playing(soundcard)){
    guint samplerate;
    guint buffer_size;

    guint note_offset;
    gdouble bpm;
    gdouble delay_factor;
    
    gdouble delay;

    ags_soundcard_get_presets(soundcard,
			      NULL,
			      &samplerate,
			      &buffer_size,
			      NULL);
    
    note_offset = ags_soundcard_get_note_offset_absolute(soundcard);

    bpm = ags_soundcard_get_bpm(soundcard);
    delay_factor = ags_soundcard_get_delay_factor(soundcard);

    /* calculate delays */
    delay = ags_soundcard_get_absolute_delay(soundcard);
  
    uptime = ags_time_get_uptime_from_offset(note_offset,
					     bpm,
					     delay,
					     delay_factor);
  }else{
    uptime = g_strdup(AGS_TIME_ZERO);
  }
  
  return(uptime);
}

void
ags_devout_delegate_play_init(AgsSoundcard *soundcard,
			      GError **error)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(soundcard);

  if(ags_devout_test_flags(devout, AGS_DEVOUT_ALSA)){
    ags_devout_alsa_init(soundcard,
			 error);
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_OSS)){
    ags_devout_oss_init(soundcard,
			error);
  }
}

void
ags_devout_delegate_play(AgsSoundcard *soundcard,
			 GError **error)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(soundcard);

  if(ags_devout_test_flags(devout, AGS_DEVOUT_ALSA)){
    ags_devout_alsa_play(soundcard,
			 error);
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_OSS)){
    ags_devout_oss_play(soundcard,
			error);
  }
}

void
ags_devout_delegate_stop(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  devout = AGS_DEVOUT(soundcard);

  if(ags_devout_test_flags(devout, AGS_DEVOUT_ALSA)){
    ags_devout_alsa_free(soundcard);
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_OSS)){
    ags_devout_oss_free(soundcard);
  }
}

void
ags_devout_oss_init(AgsSoundcard *soundcard,
		    GError **error)
{
  AgsDevout *devout;

  gchar *str;

  guint word_size;
  int format;
  int tmp;
  guint i;

  pthread_mutex_t *devout_mutex;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* retrieve word size */
  pthread_mutex_lock(devout_mutex);

  switch(devout->format){
  case AGS_SOUNDCARD_SIGNED_8_BIT:
    {
#ifdef AGS_WITH_OSS
      format = AFMT_U8;
#endif
      
      word_size = sizeof(gint8);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_16_BIT:
    {
#ifdef AGS_WITH_OSS
      format = AFMT_S16_NE;
#endif
      
      word_size = sizeof(gint16);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_24_BIT:
    {
#ifdef AGS_WITH_OSS
      format = AFMT_S24_NE;
#endif
      
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_32_BIT:
    {
#ifdef AGS_WITH_OSS
      format = AFMT_S32_NE;
#endif
      
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_64_BIT:
    {
      word_size = sizeof(gint64);
    }
  default:
    g_warning("ags_devout_oss_init(): unsupported word size");
    return;
  }

  /* prepare for playback */
  devout->flags |= (AGS_DEVOUT_BUFFER3 |
		    AGS_DEVOUT_START_PLAY |
		    AGS_DEVOUT_PLAY |
		    AGS_DEVOUT_NONBLOCKING);

  memset(devout->buffer[0], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[1], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[2], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[3], 0, devout->pcm_channels * devout->buffer_size * word_size);

  /* allocate ring buffer */
  g_atomic_int_set(&(devout->available),
		   FALSE);
  
    devout->ring_buffer = (unsigned char **) malloc(devout->ring_buffer_size * sizeof(unsigned char *));

  for(i = 0; i < devout->ring_buffer_size; i++){
    devout->ring_buffer[i] = (unsigned char *) malloc(devout->pcm_channels *
						      devout->buffer_size * word_size *
						      sizeof(unsigned char));
  }

#ifdef AGS_WITH_OSS
  /* open device fd */
  str = devout->out.oss.device;
  devout->out.oss.device_fd = open(str, O_WRONLY, 0);

  if(devout->out.oss.device_fd == -1){
    pthread_mutex_unlock(devout_mutex);

    g_warning("couldn't open device %s", devout->out.oss.device);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
		  "unable to open dsp device: %s\n",
		  str);
    }

    return;
  }

  //NOTE:JK: unsupported on kfreebsd 9.0
  //  tmp = APF_NORMAL;
  //  ioctl(devout->out.oss.device_fd, SNDCTL_DSP_PROFILE, &tmp);

  tmp = format;

  if(ioctl(devout->out.oss.device_fd, SNDCTL_DSP_SETFMT, &tmp) == -1){
    pthread_mutex_unlock(devout_mutex);

    str = strerror(errno);
    g_warning("failed to select bits/sample");

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLE_FORMAT_NOT_AVAILABLE,
		  "unable to open dsp device: %s",
		  str);
    }

    devout->out.oss.device_fd = -1;

    return;
  }
  
  if(tmp != format){
    pthread_mutex_unlock(devout_mutex);

    str = strerror(errno);
    g_warning("failed to select bits/sample");

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLE_FORMAT_NOT_AVAILABLE,
		  "unable to open dsp device: %s",
		  str);
    }

    devout->out.oss.device_fd = -1;

    return;
  }

  tmp = devout->dsp_channels;

  if(ioctl(devout->out.oss.device_fd, SNDCTL_DSP_CHANNELS, &tmp) == -1){
    pthread_mutex_unlock(devout_mutex);

    str = strerror(errno);
    g_warning("Channels count (%i) not available for playbacks: %s", devout->dsp_channels, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_CHANNELS_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.oss.device_fd = -1;
    
    return;
  }

  if(tmp != devout->dsp_channels){
    pthread_mutex_unlock(devout_mutex);

    str = strerror(errno);
    g_warning("Channels count (%i) not available for playbacks: %s", devout->dsp_channels, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_CHANNELS_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }
    
    devout->out.oss.device_fd = -1;
    
    return;
  }

  tmp = devout->samplerate;

  if(ioctl(devout->out.oss.device_fd, SNDCTL_DSP_SPEED, &tmp) == -1){
    pthread_mutex_unlock(devout_mutex);

    str = strerror(errno);
    g_warning("Rate %iHz not available for playback: %s", devout->samplerate, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLERATE_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.oss.device_fd = -1;
    
    return;
  }

  if(tmp != devout->samplerate){
    g_warning("Warning: Playback using %d Hz (file %d Hz)",
	      tmp,
	      devout->samplerate);
  }
#endif
  
  devout->tact_counter = 0.0;
  devout->delay_counter = 0.0;
  devout->tic_counter = 0;

  devout->nth_ring_buffer = 0;
  
  ags_soundcard_get_poll_fd(soundcard);
  
#ifdef AGS_WITH_OSS
  devout->flags |= AGS_DEVOUT_INITIALIZED;
#endif
  devout->flags |= AGS_DEVOUT_BUFFER0;
  devout->flags &= (~(AGS_DEVOUT_BUFFER1 |
		      AGS_DEVOUT_BUFFER2 |
		      AGS_DEVOUT_BUFFER3));
  
  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_oss_play(AgsSoundcard *soundcard,
		    GError **error)
{
  AgsDevout *devout;

  AgsNotifySoundcard *notify_soundcard;
  AgsTicDevice *tic_device;
  AgsClearBuffer *clear_buffer;
  AgsSwitchBufferFlag *switch_buffer_flag;
  
  AgsThread *task_thread;
  AgsPollFd *poll_fd;

  AgsApplicationContext *application_context;
  
  GList *task;
  GList *list;

  gchar *str;
  
  guint word_size;
  guint nth_buffer;

  int n_write;
  
  pthread_mutex_t *devout_mutex;

  static const struct timespec poll_interval = {
    0,
    250,
  };
  
  auto void ags_devout_oss_play_fill_ring_buffer(void *buffer, guint ags_format, unsigned char *ring_buffer, guint channels, guint buffer_size);

  void ags_devout_oss_play_fill_ring_buffer(void *buffer, guint ags_format, unsigned char *ring_buffer, guint channels, guint buffer_size){
    int format_bits;
    guint word_size;

    int bps;
    int res;
    guint chn;
    guint count, i;
    
    switch(ags_format){
    case AGS_SOUNDCARD_SIGNED_8_BIT:
      {
	word_size = sizeof(char);
	bps = 1;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_16_BIT:
      {
	word_size = sizeof(short);
	bps = 2;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_24_BIT:
      {
	word_size = sizeof(long);
	bps = 3;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_32_BIT:
      {
	word_size = sizeof(long);
	bps = 4;
      }
      break;
    default:
      g_warning("ags_devout_oss_play(): unsupported word size");
      return;
    }

    /* fill the channel areas */
    for(count = 0; count < buffer_size; count++){
      for(chn = 0; chn < channels; chn++){
	switch(ags_format){
	case AGS_SOUNDCARD_SIGNED_8_BIT:
	  {
	    res = (int) ((gint8 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_16_BIT:
	  {
	    res = (int) ((gint16 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_24_BIT:
	  {
	    res = (int) ((gint32 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_32_BIT:
	  {
	    res = (int) ((gint32 *) buffer)[count * channels + chn];
	  }
	  break;
	}
	
	/* Generate data in native endian format */
	if(ags_endian_host_is_be()){
	  for(i = 0; i < bps; i++){
	    *(ring_buffer + chn * bps + word_size - 1 - i) = (res >> i * 8) & 0xff;
	  }
	}else{
	  for(i = 0; i < bps; i++){
	    *(ring_buffer + chn * bps + i) = (res >>  i * 8) & 0xff;
	  }
	}	
      }

      ring_buffer += channels * bps;
    }
  }
  
  devout = AGS_DEVOUT(soundcard);

  application_context = ags_application_context_get_instance();

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* lock */
  pthread_mutex_lock(devout_mutex);
  
  notify_soundcard = AGS_NOTIFY_SOUNDCARD(devout->notify_soundcard);
  
  /* notify cyclic task */
  pthread_mutex_lock(notify_soundcard->return_mutex);

  g_atomic_int_or(&(notify_soundcard->flags),
		  AGS_NOTIFY_SOUNDCARD_DONE_RETURN);
  
  if((AGS_NOTIFY_SOUNDCARD_WAIT_RETURN & (g_atomic_int_get(&(notify_soundcard->flags)))) != 0){
    pthread_cond_signal(notify_soundcard->return_cond);
  }
  
  pthread_mutex_unlock(notify_soundcard->return_mutex);

  /* retrieve word size */
  switch(devout->format){
  case AGS_SOUNDCARD_SIGNED_8_BIT:
    {
      word_size = sizeof(gint8);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_16_BIT:
    {
      word_size = sizeof(gint16);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_24_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_32_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_64_BIT:
    {
      word_size = sizeof(gint64);
    }
    //NOTE:JK: not available    break;
  default:
    g_warning("ags_devout_oss_play(): unsupported word size");
    return;
  }

  /* do playback */
  devout->flags &= (~AGS_DEVOUT_START_PLAY);

  if((AGS_DEVOUT_INITIALIZED & (devout->flags)) == 0){
    pthread_mutex_unlock(devout_mutex);
    
    return;
  }

  /* check buffer flag */
  if((AGS_DEVOUT_BUFFER0 & (devout->flags)) != 0){
    nth_buffer = 0;
  }else if((AGS_DEVOUT_BUFFER1 & (devout->flags)) != 0){
    nth_buffer = 1;
  }else if((AGS_DEVOUT_BUFFER2 & (devout->flags)) != 0){
    nth_buffer = 2;
  }else if((AGS_DEVOUT_BUFFER3 & (devout->flags)) != 0){
    nth_buffer = 3;
  }

#ifdef AGS_WITH_OSS    
  /* fill ring buffer */
  ags_devout_oss_play_fill_ring_buffer(devout->buffer[nth_buffer],
				       devout->format,
				       devout->ring_buffer[devout->nth_ring_buffer],
				       devout->pcm_channels,
				       devout->buffer_size);

  /* wait until available */
  list = ags_soundcard_get_poll_fd(soundcard);

  if(!ags_soundcard_is_available(soundcard) &&
     !g_atomic_int_get(&(devout->available)) &&
     list != NULL){
    poll_fd = list->data;
    poll_fd->poll_fd->events = POLLOUT;
    
    while(!ags_soundcard_is_available(soundcard) &&
	  !g_atomic_int_get(&(devout->available))){
      ppoll(poll_fd->poll_fd,
	    1,
	    &poll_interval,
	    NULL);
    }
  }
  
  /* write ring buffer */
  n_write = write(devout->out.oss.device_fd,
		  devout->ring_buffer[devout->nth_ring_buffer],
		  devout->pcm_channels * devout->buffer_size * word_size * sizeof (char));

  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  if(n_write != devout->pcm_channels * devout->buffer_size * word_size * sizeof (char)){
    g_critical("write() return doesn't match written bytes");
  }
#endif

  /* increment nth ring-buffer */
  if(devout->nth_ring_buffer + 1 >= devout->ring_buffer_size){
    devout->nth_ring_buffer = 0;
  }else{
    devout->nth_ring_buffer += 1;
  }
  
  pthread_mutex_unlock(devout_mutex);

  /* update soundcard */
  task_thread = ags_concurrency_provider_get_task_thread(AGS_CONCURRENCY_PROVIDER(application_context));

  task = NULL;
  
  /* tic soundcard */
  tic_device = ags_tic_device_new((GObject *) devout);
  task = g_list_append(task,
		       tic_device);

  /* reset - clear buffer */
  clear_buffer = ags_clear_buffer_new((GObject *) devout);
  task = g_list_append(task,
		       clear_buffer);
  
  /* reset - clear buffer */
  clear_buffer = ags_clear_buffer_new((GObject *) devout);
  task = g_list_append(task,
		       clear_buffer);

  /* reset - switch buffer flags */
  switch_buffer_flag = ags_switch_buffer_flag_new((GObject *) devout);
  task = g_list_append(task,
		       switch_buffer_flag);

  /* append tasks */
  ags_task_thread_append_tasks((AgsTaskThread *) task_thread,
			       task);
}

void
ags_devout_oss_free(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  AgsNotifySoundcard *notify_soundcard;
  
  GList *poll_fd;

  guint i;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /*  */
  pthread_mutex_lock(devout_mutex);

#ifdef AGS_WITH_OSS
  /* remove poll fd */
  poll_fd = devout->poll_fd;
  
  while(poll_fd != NULL){
    ags_polling_thread_remove_poll_fd(AGS_POLL_FD(poll_fd->data)->polling_thread,
				      poll_fd->data);
    g_object_unref(poll_fd->data);
    
    poll_fd = poll_fd->next;
  }

  g_list_free(poll_fd);

  devout->poll_fd = NULL;
#endif

  notify_soundcard = AGS_NOTIFY_SOUNDCARD(devout->notify_soundcard);

  if((AGS_DEVOUT_INITIALIZED & (devout->flags)) == 0){
    pthread_mutex_unlock(devout_mutex);
    
    return;
  }
  
  close(devout->out.oss.device_fd);
  devout->out.oss.device_fd = -1;

  /* free ring-buffer */
  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  for(i = 0; i < devout->ring_buffer_size; i++){
    free(devout->ring_buffer[i]);
  }
  
  free(devout->ring_buffer);

  devout->ring_buffer = NULL;

  /* reset flags */
  devout->flags &= (~(AGS_DEVOUT_BUFFER0 |
		      AGS_DEVOUT_BUFFER1 |
		      AGS_DEVOUT_BUFFER2 |
		      AGS_DEVOUT_BUFFER3 |
		      AGS_DEVOUT_PLAY |
		      AGS_DEVOUT_INITIALIZED));

  /* notify cyclic task */
  pthread_mutex_lock(notify_soundcard->return_mutex);

  g_atomic_int_or(&(notify_soundcard->flags),
		  AGS_NOTIFY_SOUNDCARD_DONE_RETURN);
  
  if((AGS_NOTIFY_SOUNDCARD_WAIT_RETURN & (g_atomic_int_get(&(notify_soundcard->flags)))) != 0){
    pthread_cond_signal(notify_soundcard->return_cond);
  }
  
  pthread_mutex_unlock(notify_soundcard->return_mutex);

  devout->note_offset = devout->start_note_offset;
  devout->note_offset_absolute = devout->start_note_offset;

  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_alsa_init(AgsSoundcard *soundcard,
		     GError **error)
{
  AgsDevout *devout;

#ifdef AGS_WITH_ALSA

  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;

  gchar *str;
  
  int rc;
  unsigned int val;
  snd_pcm_uframes_t frames;
  unsigned int rate;
  unsigned int rrate;
  unsigned int channels;
  snd_pcm_uframes_t size;
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;
  snd_pcm_format_t format;

  int period_event;

  int err, dir;
#endif

  guint word_size;
  guint i;
  
  pthread_mutex_t *devout_mutex; 
 
  static unsigned int period_time = 100000;
  static unsigned int buffer_time = 100000;

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* retrieve word size */
  pthread_mutex_lock(devout_mutex);

  if(devout->out.alsa.device == NULL){
    pthread_mutex_unlock(devout_mutex);
    
    return;
  }

  switch(devout->format){
  case AGS_SOUNDCARD_SIGNED_8_BIT:
    {
#ifdef AGS_WITH_ALSA
      format = SND_PCM_FORMAT_S8;
#endif

      word_size = sizeof(gint8);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_16_BIT:
    {
#ifdef AGS_WITH_ALSA
      format = SND_PCM_FORMAT_S16;
#endif
      
      word_size = sizeof(gint16);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_24_BIT:
    {
#ifdef AGS_WITH_ALSA
      format = SND_PCM_FORMAT_S24;
#endif
      
      //NOTE:JK: The 24-bit linear samples use 32-bit physical space
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_32_BIT:
    {
#ifdef AGS_WITH_ALSA
      format = SND_PCM_FORMAT_S32;
#endif
      
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_64_BIT:
    {
      word_size = sizeof(gint64);
    }
    break;
  default:
    pthread_mutex_unlock(devout_mutex);

    g_warning("ags_devout_alsa_init(): unsupported word size");

    return;
  }

  /* prepare for playback */
  devout->flags |= (AGS_DEVOUT_BUFFER3 |
		    AGS_DEVOUT_START_PLAY |
		    AGS_DEVOUT_PLAY |
		    AGS_DEVOUT_NONBLOCKING);

  memset(devout->buffer[0], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[1], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[2], 0, devout->pcm_channels * devout->buffer_size * word_size);
  memset(devout->buffer[3], 0, devout->pcm_channels * devout->buffer_size * word_size);

  /* allocate ring buffer */
#ifdef AGS_WITH_ALSA
  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  devout->ring_buffer = (unsigned char **) malloc(devout->ring_buffer_size * sizeof(unsigned char *));

  for(i = 0; i < devout->ring_buffer_size; i++){
    devout->ring_buffer[i] = (unsigned char *) malloc(devout->pcm_channels *
						      devout->buffer_size * (snd_pcm_format_physical_width(format) / 8) *
						      sizeof(unsigned char));
  }
 
  /*  */
  period_event = 0;
  
  /* Open PCM device for playback. */  
  handle = NULL;

  if((err = snd_pcm_open(&handle, devout->out.alsa.device, SND_PCM_STREAM_PLAYBACK, 0)) < 0){
    gchar *device_fixup;
    
    str = snd_strerror(err);
    g_warning("Playback open error (attempting fixup): %s", str);
    
    device_fixup = g_strdup_printf("%s,0",
				   devout->out.alsa.device);

    handle = NULL;
    
    if((err = snd_pcm_open(&handle, device_fixup, SND_PCM_STREAM_PLAYBACK, 0)) < 0){
      pthread_mutex_unlock(devout_mutex);
      
      if(error != NULL){
	g_set_error(error,
		    AGS_DEVOUT_ERROR,
		    AGS_DEVOUT_ERROR_LOCKED_SOUNDCARD,
		    "unable to open pcm device: %s",
		    str);
      }
      
      return;
    }
  }

  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_alloca(&swparams);

  /* choose all parameters */
  err = snd_pcm_hw_params_any(handle, hwparams);

  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Broken configuration for playback: no configurations available: %s", str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_BROKEN_CONFIGURATION,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* set hardware resampling * /
     err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, 0);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Resampling setup failed for playback: %s\n", str);

     //    free(str);
    
     return;
     }
  */
  
  /* set the interleaved read/write format */
  err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Access type not available for playback: %s", str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_ACCESS_TYPE_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* set the sample format */
  err = snd_pcm_hw_params_set_format(handle, hwparams, format);
  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Sample format not available for playback: %s", str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLE_FORMAT_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* set the count of channels */
  channels = devout->pcm_channels;
  err = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Channels count (%i) not available for playbacks: %s", channels, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_CHANNELS_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* set the stream rate */
  rate = devout->samplerate;
  rrate = rate;
  err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, 0);
  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Rate %iHz not available for playback: %s", rate, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLERATE_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  if (rrate != rate) {
    pthread_mutex_unlock(devout_mutex);
    g_warning("Rate doesn't match (requested %iHz, get %iHz)", rate, err);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_SAMPLERATE_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    return;
  }

  /* set the buffer size */
  size = 2 * devout->buffer_size;
  err = snd_pcm_hw_params_set_buffer_size(handle, hwparams, size);
  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Unable to set buffer size %lu for playback: %s", size, str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_BUFFER_SIZE_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* set the period size * /
     period_size = devout->buffer_size;
     err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, period_size, dir);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Unable to get period size for playback: %s\n", str);

     //    free(str);
    
     return;
     }
  */
  
  /* write the parameters to device */
  err = snd_pcm_hw_params(handle, hwparams);

  if (err < 0) {
    pthread_mutex_unlock(devout_mutex);

    str = snd_strerror(err);
    g_warning("Unable to set hw params for playback: %s", str);

    if(error != NULL){
      g_set_error(error,
		  AGS_DEVOUT_ERROR,
		  AGS_DEVOUT_ERROR_HW_PARAMETERS_NOT_AVAILABLE,
		  "unable to open pcm device: %s",
		  str);
    }

    devout->out.alsa.handle = NULL;
    
    //    free(str);
    
    return;
  }

  /* get the current swparams * /
     err = snd_pcm_sw_params_current(handle, swparams);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Unable to determine current swparams for playback: %s\n", str);

     //    free(str);
    
     return;
     }
  */
  /* start the transfer when the buffer is almost full: */
  /* (buffer_size / avail_min) * avail_min * /
     err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Unable to set start threshold mode for playback: %s\n", str);
    
     //    free(str);
    
     return;
     }
  */
  /* allow the transfer when at least period_size samples can be processed */
  /* or disable this mechanism when period event is enabled (aka interrupt like style processing) * /
     err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Unable to set avail min for playback: %s\n", str);

     //    free(str);
    
     return;
     }

     /* write the parameters to the playback device * /
     err = snd_pcm_sw_params(handle, swparams);
     if (err < 0) {
     pthread_mutex_unlock(devout_mutex);

     str = snd_strerror(err);
     g_warning("Unable to set sw params for playback: %s\n", str);

     //    free(str);
    
     return;
     }
  */

  /*  */
  devout->out.alsa.handle = handle;
#endif

  devout->tact_counter = 0.0;
  devout->delay_counter = 0.0;
  devout->tic_counter = 0;

  devout->nth_ring_buffer = 0;
  
  ags_soundcard_get_poll_fd(soundcard);
  
#ifdef AGS_WITH_ALSA
  devout->flags |= AGS_DEVOUT_INITIALIZED;
#endif
  devout->flags |= AGS_DEVOUT_BUFFER0;
  devout->flags &= (~(AGS_DEVOUT_BUFFER1 |
		      AGS_DEVOUT_BUFFER2 |
		      AGS_DEVOUT_BUFFER3));
  
  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_alsa_play(AgsSoundcard *soundcard,
		     GError **error)
{
  AgsDevout *devout;

  AgsNotifySoundcard *notify_soundcard;
  AgsTicDevice *tic_device;
  AgsClearBuffer *clear_buffer;
  AgsSwitchBufferFlag *switch_buffer_flag;
  
  AgsThread *task_thread;
  AgsPollFd *poll_fd;

  AgsApplicationContext *application_context;

  GList *task;
  GList *list;
  
  gchar *str;
  
  guint word_size;
  guint nth_buffer;
  
  pthread_mutex_t *devout_mutex;

  static const struct timespec poll_interval = {
    0,
    250,
  };
  
#ifdef AGS_WITH_ALSA
  auto void ags_devout_alsa_play_fill_ring_buffer(void *buffer, guint ags_format, unsigned char *ring_buffer, guint channels, guint buffer_size);

  void ags_devout_alsa_play_fill_ring_buffer(void *buffer, guint ags_format, unsigned char *ring_buffer, guint channels, guint buffer_size){
    snd_pcm_format_t format;

    int format_bits;

    unsigned int max_val;
    
    int bps; /* bytes per sample */
    int phys_bps;

    int big_endian;
    int to_unsigned;

    int res;

    gint count;
    guint i, chn;
    
    switch(ags_format){
    case AGS_SOUNDCARD_SIGNED_8_BIT:
      {
	format = SND_PCM_FORMAT_S8;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_16_BIT:
      {
	format = SND_PCM_FORMAT_S16;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_24_BIT:
      {
	format = SND_PCM_FORMAT_S24;
      }
      break;
    case AGS_SOUNDCARD_SIGNED_32_BIT:
      {
	format = SND_PCM_FORMAT_S32;
      }
      break;
    default:
      g_warning("ags_devout_alsa_play(): unsupported word size");
      return;
    }

    count = buffer_size;
    format_bits = snd_pcm_format_width(format);

    max_val = (1 << (format_bits - 1)) - 1;

    bps = format_bits / 8;
    phys_bps = snd_pcm_format_physical_width(format) / 8;
    
    big_endian = snd_pcm_format_big_endian(format) == 1;
    to_unsigned = snd_pcm_format_unsigned(format) == 1;

    /* fill the channel areas */
    for(count = 0; count < buffer_size; count++){
      for(chn = 0; chn < channels; chn++){
	switch(ags_format){
	case AGS_SOUNDCARD_SIGNED_8_BIT:
	  {
	    res = (int) ((gint8 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_16_BIT:
	  {
	    res = (int) ((gint16 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_24_BIT:
	  {
	    res = (int) ((gint32 *) buffer)[count * channels + chn];
	  }
	  break;
	case AGS_SOUNDCARD_SIGNED_32_BIT:
	  {
	    res = (int) ((gint32 *) buffer)[count * channels + chn];
	  }
	  break;
	}

	if(to_unsigned){
	  res ^= 1U << (format_bits - 1);
	}
	
	/* Generate data in native endian format */
	if (big_endian) {
	  for (i = 0; i < bps; i++)
	    *(ring_buffer + chn * bps + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
	} else {
	  for (i = 0; i < bps; i++)
	    *(ring_buffer + chn * bps + i) = (res >>  i * 8) & 0xff;
	}	
      }

      ring_buffer += channels * bps;
    }
  }
#endif
  
  devout = AGS_DEVOUT(soundcard);
  
  application_context = ags_application_context_get_instance();

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* lock */
  pthread_mutex_lock(devout_mutex);
  
  notify_soundcard = AGS_NOTIFY_SOUNDCARD(devout->notify_soundcard);

  /* notify cyclic task */
  pthread_mutex_lock(notify_soundcard->return_mutex);

  g_atomic_int_or(&(notify_soundcard->flags),
		  AGS_NOTIFY_SOUNDCARD_DONE_RETURN);
  
  if((AGS_NOTIFY_SOUNDCARD_WAIT_RETURN & (g_atomic_int_get(&(notify_soundcard->flags)))) != 0){
    pthread_cond_signal(notify_soundcard->return_cond);
  }
  
  pthread_mutex_unlock(notify_soundcard->return_mutex);

  /* retrieve word size */
  switch(devout->format){
  case AGS_SOUNDCARD_SIGNED_8_BIT:
    {
      word_size = sizeof(gint8);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_16_BIT:
    {
      word_size = sizeof(gint16);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_24_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_32_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_64_BIT:
    {
      word_size = sizeof(gint64);
    }
    break;
  default:
    pthread_mutex_unlock(devout_mutex);
    
    g_warning("ags_devout_alsa_play(): unsupported word size");

    return;
  }

  /* do playback */
  devout->flags &= (~AGS_DEVOUT_START_PLAY);

  if((AGS_DEVOUT_INITIALIZED & (devout->flags)) == 0){
    pthread_mutex_unlock(devout_mutex);
    
    return;
  }

  //  g_message("play - 0x%0x", ((AGS_DEVOUT_BUFFER0 |
  //				AGS_DEVOUT_BUFFER1 |
  //				AGS_DEVOUT_BUFFER2 |
  //				AGS_DEVOUT_BUFFER3) & (devout->flags)));

  /* check buffer flag */
  if((AGS_DEVOUT_BUFFER0 & (devout->flags)) != 0){
    nth_buffer = 0;
  }else if((AGS_DEVOUT_BUFFER1 & (devout->flags)) != 0){
    nth_buffer = 1;
  }else if((AGS_DEVOUT_BUFFER2 & (devout->flags)) != 0){
    nth_buffer = 2;
  }else if((AGS_DEVOUT_BUFFER3 & (devout->flags)) != 0){
    nth_buffer = 3;
  }

#ifdef AGS_WITH_ALSA

  /* fill ring buffer */
  ags_devout_alsa_play_fill_ring_buffer(devout->buffer[nth_buffer], devout->format,
					devout->ring_buffer[devout->nth_ring_buffer],
					devout->pcm_channels, devout->buffer_size);

  /* wait until available */
  list = ags_soundcard_get_poll_fd(soundcard);

  if(!ags_soundcard_is_available(soundcard) &&
     !g_atomic_int_get(&(devout->available)) &&
     list != NULL){
    poll_fd = list->data;
    poll_fd->poll_fd->events = POLLOUT;
    
    while(!ags_soundcard_is_available(soundcard) &&
	  !g_atomic_int_get(&(devout->available))){
      ppoll(poll_fd->poll_fd,
	    1,
	    &poll_interval,
	    NULL);
    }
  }
  
  /* write ring buffer */
  devout->out.alsa.rc = snd_pcm_writei(devout->out.alsa.handle,
				       devout->ring_buffer[devout->nth_ring_buffer],
				       (snd_pcm_uframes_t) (devout->buffer_size));

  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  /* check error flag */
  if((AGS_DEVOUT_NONBLOCKING & (devout->flags)) == 0){
    if(devout->out.alsa.rc == -EPIPE){
      /* EPIPE means underrun */
      snd_pcm_prepare(devout->out.alsa.handle);

#ifdef AGS_DEBUG
      g_message("underrun occurred");
#endif
    }else if(devout->out.alsa.rc == -ESTRPIPE){
      static const struct timespec idle = {
	0,
	4000,
      };

      int err;

      while((err = snd_pcm_resume(devout->out.alsa.handle)) < 0){ // == -EAGAIN
	nanosleep(&idle, NULL); /* wait until the suspend flag is released */
      }
	
      if(err < 0){
	err = snd_pcm_prepare(devout->out.alsa.handle);
      }
    }else if(devout->out.alsa.rc < 0){
      str = snd_strerror(devout->out.alsa.rc);
      
      g_message("error from writei: %s", str);
    }else if(devout->out.alsa.rc != (int) devout->buffer_size) {
      g_message("short write, write %d frames", devout->out.alsa.rc);
    }
  }      
  
#endif

  /* increment nth ring-buffer */
  g_atomic_int_set(&(devout->available),
		   FALSE);
  
  if(devout->nth_ring_buffer + 1 >= devout->ring_buffer_size){
    devout->nth_ring_buffer = 0;
  }else{
    devout->nth_ring_buffer += 1;
  }
  
  pthread_mutex_unlock(devout_mutex);

  /* update soundcard */
  task_thread = ags_concurrency_provider_get_task_thread(AGS_CONCURRENCY_PROVIDER(application_context));

  task = NULL;
  
  /* tic soundcard */
  tic_device = ags_tic_device_new((GObject *) devout);
  task = g_list_append(task,
		       tic_device);

  /* reset - clear buffer */
  clear_buffer = ags_clear_buffer_new((GObject *) devout);
  task = g_list_append(task,
		       clear_buffer);

  /* reset - switch buffer flags */
  switch_buffer_flag = ags_switch_buffer_flag_new((GObject *) devout);
  task = g_list_append(task,
		       switch_buffer_flag);

  /* append tasks */
  ags_task_thread_append_tasks((AgsTaskThread *) task_thread,
			       task);
  
#ifdef AGS_WITH_ALSA
  snd_pcm_prepare(devout->out.alsa.handle);
#endif
}

void
ags_devout_alsa_free(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  AgsNotifySoundcard *notify_soundcard;

  GList *start_poll_fd, *poll_fd;

  guint i;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());  

  /* lock */
#ifdef AGS_WITH_ALSA
  /* remove poll fd */
  pthread_mutex_lock(devout_mutex);

  poll_fd =
    start_poll_fd = g_list_copy(devout->poll_fd);

  pthread_mutex_unlock(devout_mutex);
  
  while(poll_fd != NULL){
    ags_polling_thread_remove_poll_fd(AGS_POLL_FD(poll_fd->data)->polling_thread,
				      poll_fd->data);
    g_object_unref(poll_fd->data);
    
    poll_fd = poll_fd->next;
  }

  g_list_free(start_poll_fd);

  pthread_mutex_lock(devout_mutex);

  g_list_free(devout->poll_fd);
  devout->poll_fd = NULL;

  pthread_mutex_unlock(devout_mutex);
#endif

  pthread_mutex_lock(devout_mutex);

  notify_soundcard = AGS_NOTIFY_SOUNDCARD(devout->notify_soundcard);

  if((AGS_DEVOUT_INITIALIZED & (devout->flags)) == 0){
    pthread_mutex_unlock(devout_mutex);
    
    return;
  }

  pthread_mutex_unlock(devout_mutex);
  
#ifdef AGS_WITH_ALSA
  //  snd_pcm_drain(devout->out.alsa.handle);
  snd_pcm_close(devout->out.alsa.handle);
  devout->out.alsa.handle = NULL;
#endif

  /* free ring-buffer */
  pthread_mutex_lock(devout_mutex);

  for(i = 0; i < devout->ring_buffer_size; i++){
    free(devout->ring_buffer[i]);
  }

  free(devout->ring_buffer);

  devout->ring_buffer = NULL;

  /* reset flags */
  devout->flags &= (~(AGS_DEVOUT_BUFFER0 |
		      AGS_DEVOUT_BUFFER1 |
		      AGS_DEVOUT_BUFFER2 |
		      AGS_DEVOUT_BUFFER3 |
		      AGS_DEVOUT_PLAY |
		      AGS_DEVOUT_INITIALIZED));

  pthread_mutex_unlock(devout_mutex);

  /* notify cyclic task */
  pthread_mutex_lock(notify_soundcard->return_mutex);

  g_atomic_int_or(&(notify_soundcard->flags),
		  AGS_NOTIFY_SOUNDCARD_DONE_RETURN);
  
  if((AGS_NOTIFY_SOUNDCARD_WAIT_RETURN & (g_atomic_int_get(&(notify_soundcard->flags)))) != 0){
    pthread_cond_signal(notify_soundcard->return_cond);
  }
  
  pthread_mutex_unlock(notify_soundcard->return_mutex);

  pthread_mutex_lock(devout_mutex);

  devout->note_offset = devout->start_note_offset;
  devout->note_offset_absolute = devout->start_note_offset;

  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_tic(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gdouble delay;
  gdouble delay_counter;
  guint note_offset_absolute;
  guint note_offset;
  guint loop_left, loop_right;
  gboolean do_loop;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());
  
  /* determine if attack should be switched */
  pthread_mutex_lock(devout_mutex);

  delay = devout->delay[devout->tic_counter];
  delay_counter = devout->delay_counter;

  note_offset = devout->note_offset;
  note_offset_absolute = devout->note_offset_absolute;
  
  loop_left = devout->loop_left;
  loop_right = devout->loop_right;
  
  do_loop = devout->do_loop;

  pthread_mutex_unlock(devout_mutex);

  if((guint) delay_counter + 1 >= (guint) delay){
    if(do_loop &&
       note_offset + 1 == loop_right){
      ags_soundcard_set_note_offset(soundcard,
				    loop_left);
    }else{
      ags_soundcard_set_note_offset(soundcard,
				    note_offset + 1);
    }
    
    ags_soundcard_set_note_offset_absolute(soundcard,
					   note_offset_absolute + 1);

    /* delay */
    ags_soundcard_offset_changed(soundcard,
				 note_offset);
    
    /* reset - delay counter */
    pthread_mutex_lock(devout_mutex);
    
    devout->delay_counter = 0.0;
    devout->tact_counter += 1.0;

    pthread_mutex_unlock(devout_mutex);
  }else{
    pthread_mutex_lock(devout_mutex);
    
    devout->delay_counter += 1.0;

    pthread_mutex_unlock(devout_mutex);
  }
}

void
ags_devout_offset_changed(AgsSoundcard *soundcard,
			  guint note_offset)
{
  AgsDevout *devout;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* offset changed */
  pthread_mutex_lock(devout_mutex);

  devout->tic_counter += 1;

  if(devout->tic_counter == AGS_SOUNDCARD_DEFAULT_PERIOD){
    /* reset - tic counter i.e. modified delay index within period */
    devout->tic_counter = 0;
  }

  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_set_bpm(AgsSoundcard *soundcard,
		   gdouble bpm)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set bpm */
  pthread_mutex_lock(devout_mutex);

  devout->bpm = bpm;

  pthread_mutex_unlock(devout_mutex);

  ags_devout_adjust_delay_and_attack(devout);
}

gdouble
ags_devout_get_bpm(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gdouble bpm;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get bpm */
  pthread_mutex_lock(devout_mutex);

  bpm = devout->bpm;
  
  pthread_mutex_unlock(devout_mutex);

  return(bpm);
}

void
ags_devout_set_delay_factor(AgsSoundcard *soundcard,
			    gdouble delay_factor)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set delay factor */
  pthread_mutex_lock(devout_mutex);

  devout->delay_factor = delay_factor;

  pthread_mutex_unlock(devout_mutex);

  ags_devout_adjust_delay_and_attack(devout);
}

gdouble
ags_devout_get_delay_factor(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gdouble delay_factor;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get delay factor */
  pthread_mutex_lock(devout_mutex);

  delay_factor = devout->delay_factor;
  
  pthread_mutex_unlock(devout_mutex);

  return(delay_factor);
}

gdouble
ags_devout_get_delay(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint delay_index;
  gdouble delay;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get delay */
  pthread_mutex_lock(devout_mutex);

  delay_index = devout->tic_counter;

  delay = devout->delay[delay_index];
  
  pthread_mutex_unlock(devout_mutex);
  
  return(delay);
}

gdouble
ags_devout_get_absolute_delay(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  gdouble absolute_delay;
  
  pthread_mutex_t *devout_mutex;
  
  devout = AGS_DEVOUT(soundcard);
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get absolute delay */
  pthread_mutex_lock(devout_mutex);

  absolute_delay = (60.0 * (((gdouble) devout->samplerate / (gdouble) devout->buffer_size) / (gdouble) devout->bpm) * ((1.0 / 16.0) * (1.0 / (gdouble) devout->delay_factor)));

  pthread_mutex_unlock(devout_mutex);

  return(absolute_delay);
}

guint
ags_devout_get_attack(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint attack_index;
  guint attack;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get attack */
  pthread_mutex_lock(devout_mutex);

  attack_index = devout->tic_counter;

  attack = devout->attack[attack_index];

  pthread_mutex_unlock(devout_mutex);
  
  return(attack);
}

void*
ags_devout_get_buffer(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  void *buffer;
  
  devout = AGS_DEVOUT(soundcard);

  if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER0)){
    buffer = devout->buffer[0];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER1)){
    buffer = devout->buffer[1];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER2)){
    buffer = devout->buffer[2];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER3)){
    buffer = devout->buffer[3];
  }else{
    buffer = NULL;
  }

  return(buffer);
}

void*
ags_devout_get_next_buffer(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  void *buffer;
  
  devout = AGS_DEVOUT(soundcard);

  //  g_message("next - 0x%0x", ((AGS_DEVOUT_BUFFER0 |
  //				AGS_DEVOUT_BUFFER1 |
  //				AGS_DEVOUT_BUFFER2 |
  //				AGS_DEVOUT_BUFFER3) & (devout->flags)));

  if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER0)){
    buffer = devout->buffer[1];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER1)){
    buffer = devout->buffer[2];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER2)){
    buffer = devout->buffer[3];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER3)){
    buffer = devout->buffer[0];
  }else{
    buffer = NULL;
  }

  return(buffer);
}

void*
ags_devout_get_prev_buffer(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  void *buffer;
  
  devout = AGS_DEVOUT(soundcard);

  if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER0)){
    buffer = devout->buffer[3];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER1)){
    buffer = devout->buffer[0];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER2)){
    buffer = devout->buffer[1];
  }else if(ags_devout_test_flags(devout, AGS_DEVOUT_BUFFER3)){
    buffer = devout->buffer[2];
  }else{
    buffer = NULL;
  }

  return(buffer);
}

void
ags_devout_lock_buffer(AgsSoundcard *soundcard,
		       void *buffer)
{
  AgsDevout *devout;

  pthread_mutex_t *buffer_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  buffer_mutex = NULL;

  if(devout->buffer != NULL){
    if(buffer == devout->buffer[0]){
      buffer_mutex = devout->buffer_mutex[0];
    }else if(buffer == devout->buffer[1]){
      buffer_mutex = devout->buffer_mutex[1];
    }else if(buffer == devout->buffer[2]){
      buffer_mutex = devout->buffer_mutex[2];
    }else if(buffer == devout->buffer[3]){
      buffer_mutex = devout->buffer_mutex[3];
    }
  }
  
  if(buffer_mutex != NULL){
    pthread_mutex_lock(buffer_mutex);
  }
}

void
ags_devout_unlock_buffer(AgsSoundcard *soundcard,
			 void *buffer)
{
  AgsDevout *devout;

  pthread_mutex_t *buffer_mutex;
  
  devout = AGS_DEVOUT(soundcard);

  buffer_mutex = NULL;

  if(devout->buffer != NULL){
    if(buffer == devout->buffer[0]){
      buffer_mutex = devout->buffer_mutex[0];
    }else if(buffer == devout->buffer[1]){
      buffer_mutex = devout->buffer_mutex[1];
    }else if(buffer == devout->buffer[2]){
      buffer_mutex = devout->buffer_mutex[2];
    }else if(buffer == devout->buffer[3]){
      buffer_mutex = devout->buffer_mutex[3];
    }
  }

  if(buffer_mutex != NULL){
    pthread_mutex_unlock(buffer_mutex);
  }
}

guint
ags_devout_get_delay_counter(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint delay_counter;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* delay counter */
  pthread_mutex_lock(devout_mutex);

  delay_counter = devout->delay_counter;
  
  pthread_mutex_unlock(devout_mutex);

  return(delay_counter);
}

void
ags_devout_set_start_note_offset(AgsSoundcard *soundcard,
				 guint start_note_offset)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  devout->start_note_offset = start_note_offset;

  pthread_mutex_unlock(devout_mutex);
}

guint
ags_devout_get_start_note_offset(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint start_note_offset;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  start_note_offset = devout->start_note_offset;

  pthread_mutex_unlock(devout_mutex);

  return(start_note_offset);
}

void
ags_devout_set_note_offset(AgsSoundcard *soundcard,
			   guint note_offset)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  devout->note_offset = note_offset;

  pthread_mutex_unlock(devout_mutex);
}

guint
ags_devout_get_note_offset(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint note_offset;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  note_offset = devout->note_offset;

  pthread_mutex_unlock(devout_mutex);

  return(note_offset);
}

void
ags_devout_set_note_offset_absolute(AgsSoundcard *soundcard,
				    guint note_offset_absolute)
{
  AgsDevout *devout;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  devout->note_offset_absolute = note_offset_absolute;

  pthread_mutex_unlock(devout_mutex);
}

guint
ags_devout_get_note_offset_absolute(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint note_offset_absolute;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set note offset */
  pthread_mutex_lock(devout_mutex);

  note_offset_absolute = devout->note_offset_absolute;

  pthread_mutex_unlock(devout_mutex);

  return(note_offset_absolute);
}

void
ags_devout_set_loop(AgsSoundcard *soundcard,
		    guint loop_left, guint loop_right,
		    gboolean do_loop)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* set loop */
  pthread_mutex_lock(devout_mutex);

  devout->loop_left = loop_left;
  devout->loop_right = loop_right;
  devout->do_loop = do_loop;

  if(do_loop){
    devout->loop_offset = devout->note_offset;
  }

  pthread_mutex_unlock(devout_mutex);
}

void
ags_devout_get_loop(AgsSoundcard *soundcard,
		    guint *loop_left, guint *loop_right,
		    gboolean *do_loop)
{
  AgsDevout *devout;

  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get loop */
  pthread_mutex_lock(devout_mutex);

  if(loop_left != NULL){
    *loop_left = devout->loop_left;
  }

  if(loop_right != NULL){
    *loop_right = devout->loop_right;
  }

  if(do_loop != NULL){
    *do_loop = devout->do_loop;
  }

  pthread_mutex_unlock(devout_mutex);
}

guint
ags_devout_get_loop_offset(AgsSoundcard *soundcard)
{
  AgsDevout *devout;

  guint loop_offset;
  
  pthread_mutex_t *devout_mutex;  

  devout = AGS_DEVOUT(soundcard);

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get loop offset */
  pthread_mutex_lock(devout_mutex);

  loop_offset = devout->loop_offset;
  
  pthread_mutex_unlock(devout_mutex);

  return(loop_offset);
}

/**
 * ags_devout_switch_buffer_flag:
 * @devout: the #AgsDevout
 *
 * The buffer flag indicates the currently played buffer.
 *
 * Since: 2.0.0
 */
void
ags_devout_switch_buffer_flag(AgsDevout *devout)
{
  pthread_mutex_t *devout_mutex;
  
  if(!AGS_IS_DEVOUT(devout)){
    return;
  }

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* switch buffer flag */
  pthread_mutex_lock(devout_mutex);

  if((AGS_DEVOUT_BUFFER0 & (devout->flags)) != 0){
    devout->flags &= (~AGS_DEVOUT_BUFFER0);
    devout->flags |= AGS_DEVOUT_BUFFER1;
  }else if((AGS_DEVOUT_BUFFER1 & (devout->flags)) != 0){
    devout->flags &= (~AGS_DEVOUT_BUFFER1);
    devout->flags |= AGS_DEVOUT_BUFFER2;
  }else if((AGS_DEVOUT_BUFFER2 & (devout->flags)) != 0){
    devout->flags &= (~AGS_DEVOUT_BUFFER2);
    devout->flags |= AGS_DEVOUT_BUFFER3;
  }else if((AGS_DEVOUT_BUFFER3 & (devout->flags)) != 0){
    devout->flags &= (~AGS_DEVOUT_BUFFER3);
    devout->flags |= AGS_DEVOUT_BUFFER0;
  }

  pthread_mutex_unlock(devout_mutex);
}

/**
 * ags_devout_adjust_delay_and_attack:
 * @devout: the #AgsDevout
 *
 * Calculate delay and attack and reset it.
 *
 * Since: 2.0.0
 */
void
ags_devout_adjust_delay_and_attack(AgsDevout *devout)
{
  gdouble delay;
  guint default_tact_frames;
  guint delay_tact_frames;
  guint default_period;
  gint next_attack;
  guint i;

  pthread_mutex_t *devout_mutex;

  if(!AGS_IS_DEVOUT(devout)){
    return;
  }
  
  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get some initial values */
  delay = ags_devout_get_absolute_delay(AGS_SOUNDCARD(devout));

#ifdef AGS_DEBUG
  g_message("delay : %f", delay);
#endif
  
  pthread_mutex_lock(devout_mutex);

  default_tact_frames = (guint) (delay * devout->buffer_size);
  delay_tact_frames = (guint) (floor(delay) * devout->buffer_size);
  default_period = (1.0 / AGS_SOUNDCARD_DEFAULT_PERIOD) * (default_tact_frames);

  i = 0;
  
  devout->attack[0] = (guint) floor(0.25 * devout->buffer_size);
  next_attack = (((devout->attack[i] + default_tact_frames) / devout->buffer_size) - delay) * devout->buffer_size;

  if(next_attack >= devout->buffer_size){
    next_attack = devout->buffer_size - 1;
  }
  
  /* check if delay drops for next attack */
  if(next_attack < 0){
    devout->attack[i] = devout->attack[i] - ((gdouble) next_attack / 2.0);

    if(devout->attack[i] < 0){
      devout->attack[i] = 0;
    }
    
    if(devout->attack[i] >= devout->buffer_size){
      devout->attack[i] = devout->buffer_size - 1;
    }
    
    next_attack = next_attack + (next_attack / 2.0);

    if(next_attack < 0){
      next_attack = 0;
    }

    if(next_attack >= devout->buffer_size){
      next_attack = devout->buffer_size - 1;
    }
  }
  
  for(i = 1; i < (int)  2.0 * AGS_SOUNDCARD_DEFAULT_PERIOD; i++){
    devout->attack[i] = next_attack;
    next_attack = (((devout->attack[i] + default_tact_frames) / devout->buffer_size) - delay) * devout->buffer_size;

    if(next_attack >= devout->buffer_size){
      next_attack = devout->buffer_size - 1;
    }
    
    /* check if delay drops for next attack */
    if(next_attack < 0){
      devout->attack[i] = devout->attack[i] - ((gdouble) next_attack / 2.0);

      if(devout->attack[i] < 0){
	devout->attack[i] = 0;
      }

      if(devout->attack[i] >= devout->buffer_size){
	devout->attack[i] = devout->buffer_size - 1;
      }
    
      next_attack = next_attack + (next_attack / 2.0);
      
      if(next_attack < 0){
	next_attack = 0;
      }

      if(next_attack >= devout->buffer_size){
	next_attack = devout->buffer_size - 1;
      }
    }
    
#ifdef AGS_DEBUG
    g_message("%d", devout->attack[i]);
#endif
  }

  devout->attack[0] = devout->attack[i - 2];
  
  for(i = 0; i < (int) 2.0 * AGS_SOUNDCARD_DEFAULT_PERIOD - 1; i++){
    devout->delay[i] = ((gdouble) (default_tact_frames + devout->attack[i] - devout->attack[i + 1])) / (gdouble) devout->buffer_size;
    
#ifdef AGS_DEBUG
    g_message("%f", devout->delay[i]);
#endif
  }

  devout->delay[i] = ((gdouble) (default_tact_frames + devout->attack[i] - devout->attack[0])) / (gdouble) devout->buffer_size;

  pthread_mutex_unlock(devout_mutex);
}

/**
 * ags_devout_realloc_buffer:
 * @devout: the #AgsDevout
 *
 * Reallocate the internal audio buffer.
 *
 * Since: 2.0.0
 */
void
ags_devout_realloc_buffer(AgsDevout *devout)
{
  guint pcm_channels;
  guint buffer_size;
  guint word_size;
  
  pthread_mutex_t *devout_mutex;  

  if(!AGS_IS_DEVOUT(devout)){
    return;
  }

  /* get devout mutex */
  pthread_mutex_lock(ags_devout_get_class_mutex());
  
  devout_mutex = devout->obj_mutex;
  
  pthread_mutex_unlock(ags_devout_get_class_mutex());

  /* get word size */  
  pthread_mutex_lock(devout_mutex);

  pcm_channels = devout->pcm_channels;
  buffer_size = devout->buffer_size;
  
  switch(devout->format){
  case AGS_SOUNDCARD_SIGNED_8_BIT:
    {
      word_size = sizeof(gint8);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_16_BIT:
    {
      word_size = sizeof(gint16);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_24_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_32_BIT:
    {
      word_size = sizeof(gint32);
    }
    break;
  case AGS_SOUNDCARD_SIGNED_64_BIT:
    {
      word_size = sizeof(gint64);
    }
    break;
  default:
    g_warning("ags_devout_realloc_buffer(): unsupported word size");
    return;
  }  

  pthread_mutex_unlock(devout_mutex);

  //NOTE:JK: there is no lock applicable to buffer
  
  /* AGS_DEVOUT_BUFFER_0 */
  if(devout->buffer[0] != NULL){
    free(devout->buffer[0]);
  }
  
  devout->buffer[0] = (void *) malloc(pcm_channels * buffer_size * word_size);
  
  /* AGS_DEVOUT_BUFFER_1 */
  if(devout->buffer[1] != NULL){
    free(devout->buffer[1]);
  }

  devout->buffer[1] = (void *) malloc(pcm_channels * buffer_size * word_size);
  
  /* AGS_DEVOUT_BUFFER_2 */
  if(devout->buffer[2] != NULL){
    free(devout->buffer[2]);
  }

  devout->buffer[2] = (void *) malloc(pcm_channels * buffer_size * word_size);
  
  /* AGS_DEVOUT_BUFFER_3 */
  if(devout->buffer[3] != NULL){
    free(devout->buffer[3]);
  }
  
  devout->buffer[3] = (void *) malloc(pcm_channels * buffer_size * word_size);
}

/**
 * ags_devout_new:
 * @application_context: the #AgsApplicationContext
 *
 * Creates a new instance of #AgsDevout.
 *
 * Returns: the new #AgsDevout
 *
 * Since: 2.0.0
 */
AgsDevout*
ags_devout_new(GObject *application_context)
{
  AgsDevout *devout;

  devout = (AgsDevout *) g_object_new(AGS_TYPE_DEVOUT,
				      "application-context", application_context,
				      NULL);
  
  return(devout);
}
