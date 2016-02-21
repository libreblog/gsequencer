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

#include <ags/audio/task/ags_apply_synth.h>

#include <ags/object/ags_connectable.h>

#include <ags/audio/ags_audio.h>
#include <ags/audio/ags_audio_signal.h>
#include <ags/audio/ags_synths.h>

#include <ags/object/ags_config.h>

#include <math.h>

void ags_apply_synth_class_init(AgsApplySynthClass *apply_synth);
void ags_apply_synth_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_apply_synth_init(AgsApplySynth *apply_synth);
void ags_apply_synth_connect(AgsConnectable *connectable);
void ags_apply_synth_disconnect(AgsConnectable *connectable);
void ags_apply_synth_finalize(GObject *gobject);

void ags_apply_synth_launch(AgsTask *task);

/**
 * SECTION:ags_apply_synth
 * @short_description: apply synth to channel
 * @title: AgsApplySynth
 * @section_id:
 * @include: ags/synth/task/ags_apply_synth.h
 *
 * The #AgsApplySynth task apply the specified synth to channel.
 */

static gpointer ags_apply_synth_parent_class = NULL;
static AgsConnectableInterface *ags_apply_synth_parent_connectable_interface;

GType
ags_apply_synth_get_type()
{
  static GType ags_type_apply_synth = 0;

  if(!ags_type_apply_synth){
    static const GTypeInfo ags_apply_synth_info = {
      sizeof (AgsApplySynthClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_apply_synth_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsApplySynth),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_apply_synth_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_apply_synth_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_apply_synth = g_type_register_static(AGS_TYPE_TASK,
						  "AgsApplySynth\0",
						  &ags_apply_synth_info,
						  0);

    g_type_add_interface_static(ags_type_apply_synth,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
  }
  
  return (ags_type_apply_synth);
}

void
ags_apply_synth_class_init(AgsApplySynthClass *apply_synth)
{
  GObjectClass *gobject;
  AgsTaskClass *task;

  ags_apply_synth_parent_class = g_type_class_peek_parent(apply_synth);

  /* GObjectClass */
  gobject = (GObjectClass *) apply_synth;

  gobject->finalize = ags_apply_synth_finalize;

  /* AgsTaskClass */
  task = (AgsTaskClass *) apply_synth;

  task->launch = ags_apply_synth_launch;
}

void
ags_apply_synth_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_apply_synth_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_apply_synth_connect;
  connectable->disconnect = ags_apply_synth_disconnect;
}

void
ags_apply_synth_init(AgsApplySynth *apply_synth)
{
  apply_synth->start_channel = NULL;
  apply_synth->count = 0;

  apply_synth->wave = AGS_APPLY_SYNTH_INVALID;
  apply_synth->attack = 0;
  apply_synth->frame_count = 0;
  apply_synth->phase = 0;
  apply_synth->start = 0;
  apply_synth->volume = 1.0;

  apply_synth->loop_start = 0;
  apply_synth->loop_end = 0;
}

void
ags_apply_synth_connect(AgsConnectable *connectable)
{
  ags_apply_synth_parent_connectable_interface->connect(connectable);

  /* empty */
}

void
ags_apply_synth_disconnect(AgsConnectable *connectable)
{
  ags_apply_synth_parent_connectable_interface->disconnect(connectable);

  /* empty */
}

void
ags_apply_synth_finalize(GObject *gobject)
{
  G_OBJECT_CLASS(ags_apply_synth_parent_class)->finalize(gobject);

  /* empty */
}

void
ags_apply_synth_launch(AgsTask *task)
{
  AgsSoundcard *soundcard;
  AgsChannel *channel;
  AgsAudioSignal *audio_signal;

  AgsApplySynth *apply_synth;

  AgsConfig *config;
  
  GList *stream;

  gint wave;
  guint attack, frame_count, stop, phase, frequency;
  double volume;
  guint current_attack, current_frame_count, current_stop, current_phase[2], current_frequency;
  guint stream_start, last_frame_count;
  guint i, j;
  double factor;
  guint buffer_size;
  guint samplerate;
  gchar *str;
  
  double ags_apply_synth_calculate_factor(guint base_frequency, guint wished_frequency, guint wave){
    double factor;

    factor = ((1.0 / base_frequency) * (wished_frequency));

    return(factor);
  }
  void ags_apply_synth_launch_write(guint offset,
				    guint frequency, guint phase, guint frame_count,
				    double volume){
    switch(wave){
    case AGS_APPLY_SYNTH_SIN:
      ags_synth_sin(soundcard, (signed short *) stream->data,
		    offset, frequency, phase, frame_count,
		    volume);
      break;
    case AGS_APPLY_SYNTH_SAW:
      ags_synth_saw(soundcard, (signed short *) stream->data,
		    offset, frequency, phase, frame_count,
		    volume);
      break;
    case AGS_APPLY_SYNTH_SQUARE:
      ags_synth_square(soundcard, (signed short *) stream->data, offset, frequency, phase, frame_count, volume);
      break;
    case AGS_APPLY_SYNTH_TRIANGLE:
      ags_synth_triangle(soundcard, (signed short *) stream->data, offset, frequency, phase, frame_count, volume);
      break;
    default:
      g_warning("ags_apply_synth_launch_write: warning no wave selected\n\0");
    }
  }

  apply_synth = AGS_APPLY_SYNTH(task);
  channel = apply_synth->start_channel;
  soundcard = AGS_SOUNDCARD(channel->soundcard);

  config = ags_config_get_instance();
  
  str = ags_config_get_value(config,
			     AGS_CONFIG_SOUNDCARD,
			     "buffer-size\0");
  buffer_size = g_ascii_strtoull(str,
				 NULL,
				 10);
  free(str);
  
  str = ags_config_get_value(config,
			     AGS_CONFIG_SOUNDCARD,
			     "samplerate\0");
  samplerate = g_ascii_strtoull(str,
				NULL,
				10);
  free(str);
  
  wave = (gint) apply_synth->wave;
  g_message("wave = %d\n\0", wave);

  /* some settings */
  frequency = apply_synth->frequency;

  volume = (double) apply_synth->volume;

  /* settings which needs to be initialized for factorizing */
  attack = apply_synth->attack;
  frame_count = apply_synth->frame_count;
  stop = (guint) ceil((double)(attack + frame_count) / (double) buffer_size);
  phase = apply_synth->phase;

  //TODO:JK: 
  //  attack = attack % (guint) buffer_size;
  
  current_phase[0] = (phase + (buffer_size - attack) + i * buffer_size) % frequency;

  factor = 1.0;

  for(i = 0; channel != NULL; i++){
    /* calculate wished frequncy and phase */
    current_frequency = (guint) ((double) frequency * exp2((double)((apply_synth->start * -1.0) + (double)i) / 12.0));
    current_phase[0] = (guint) ((double) phase * ((double) frequency / (double) current_frequency));

    /* settings which needs to be factorized */
    factor = ags_apply_synth_calculate_factor(frequency, current_frequency, wave);

    current_attack = (guint) (factor * attack) % (guint) buffer_size;
    current_frame_count = frame_count * factor;
    current_stop = (guint) ceil((double)(current_attack + current_frame_count) / (double)buffer_size);
    current_phase[0] = ((guint)(factor * phase) + (buffer_size - current_attack) + i * buffer_size) % current_frequency;

    stream_start = (guint) floor((double)current_attack / (double) buffer_size);
    last_frame_count = (current_frame_count - buffer_size - current_attack) % buffer_size;

    /* create AgsAudioSignal */
    audio_signal = ags_audio_signal_get_template(channel->first_recycling->audio_signal);
    
    if(audio_signal->length < current_stop)
      ags_audio_signal_stream_resize(audio_signal, stop);

    audio_signal->loop_start = (guint) ((double) apply_synth->loop_start) * factor;
    audio_signal->loop_end = (guint) ((double) apply_synth->loop_end) * factor;
    

    /* fill in the stream */
    stream = g_list_nth(audio_signal->stream_beginning, stream_start);
    
    if(stream->next != NULL){
      g_message("freq = %u, phase = %u\n\0", current_frequency, current_phase[0]);
      ags_apply_synth_launch_write(attack,
				   current_frequency, current_phase[0], buffer_size - attack,
				   volume);
    }else{
      ags_apply_synth_launch_write(attack,
				   current_frequency, current_phase[0], frame_count,
				   volume);
      
      channel = channel->next;
      continue;
    }
    
    stream = stream->next;
    
    
    for(j = 1; stream->next != NULL; j++){
      current_phase[1] = (j * buffer_size + current_phase[0]) % (samplerate / current_frequency);
      g_message("freq = %u, phase = %u\n\0", current_frequency, current_phase[1]);
      
      ags_apply_synth_launch_write(0,
				   frequency, current_phase[1], buffer_size,
				   volume);
      
      stream = stream->next;
    }
    
    current_phase[1] = (current_phase[0] + (buffer_size - attack) + j * buffer_size) % current_frequency;
    ags_apply_synth_launch_write(0,
				 current_frequency, current_phase[1], last_frame_count,
				 volume);
    
    
    channel = channel->next;
  }
}

/**
 * ags_apply_synth_new:
 * @start_channel:
 * @count:
 * @wave:
 * @attack:
 * @frame_count:
 * @frequency:
 * @phase:
 * @start:
 * @volume:
 * @loop_start:
 * @loop_end:
 *
 * Creates an #AgsApplySynth.
 *
 * Returns: an new #AgsApplySynth.
 *
 * Since: 0.4
 */
AgsApplySynth*
ags_apply_synth_new(AgsChannel *start_channel, guint count,
		    guint wave,
		    guint attack, guint frame_count,
		    guint frequency, guint phase, guint start,
		    gdouble volume,
		    guint loop_start, guint loop_end)
{
  AgsApplySynth *apply_synth;

  apply_synth = (AgsApplySynth *) g_object_new(AGS_TYPE_APPLY_SYNTH,
					       NULL);

  apply_synth->start_channel = start_channel;
  apply_synth->count = count;
  apply_synth->wave = wave;
  apply_synth->attack = attack;
  apply_synth->frame_count = frame_count;
  apply_synth->frequency = frequency;
  apply_synth->phase = phase;
  apply_synth->start = start;
  apply_synth->volume = volume;
  apply_synth->loop_start = loop_start;
  apply_synth->loop_end = loop_end;

  return(apply_synth);
}
