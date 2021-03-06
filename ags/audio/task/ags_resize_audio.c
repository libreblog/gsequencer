/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2020 Joël Krähemann
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

#include <ags/audio/task/ags_resize_audio.h>

#include <ags/audio/ags_output.h>
#include <ags/audio/ags_input.h>

#include <ags/i18n.h>

void ags_resize_audio_class_init(AgsResizeAudioClass *resize_audio);
void ags_resize_audio_init(AgsResizeAudio *resize_audio);
void ags_resize_audio_set_property(GObject *gobject,
				   guint prop_id,
				   const GValue *value,
				   GParamSpec *param_spec);
void ags_resize_audio_get_property(GObject *gobject,
				   guint prop_id,
				   GValue *value,
				   GParamSpec *param_spec);
void ags_resize_audio_dispose(GObject *gobject);
void ags_resize_audio_finalize(GObject *gobject);

void ags_resize_audio_launch(AgsTask *task);

/**
 * SECTION:ags_resize_audio
 * @short_description: resize audio task
 * @title: AgsResizeAudio
 * @section_id:
 * @include: ags/audio/task/ags_resize_audio.h
 *
 * The #AgsResizeAudio task resizes #AgsAudio.
 */

static gpointer ags_resize_audio_parent_class = NULL;

enum{
  PROP_0,
  PROP_AUDIO,
  PROP_OUTPUT_PADS,
  PROP_INPUT_PADS,
  PROP_AUDIO_CHANNELS,
};

GType
ags_resize_audio_get_type()
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_resize_audio = 0;

    static const GTypeInfo ags_resize_audio_info = {
      sizeof(AgsResizeAudioClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_resize_audio_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsResizeAudio),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_resize_audio_init,
    };
    
    ags_type_resize_audio = g_type_register_static(AGS_TYPE_TASK,
						   "AgsResizeAudio",
						   &ags_resize_audio_info,
						   0);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_resize_audio);
  }

  return g_define_type_id__volatile;
}

void
ags_resize_audio_class_init(AgsResizeAudioClass *resize_audio)
{
  GObjectClass *gobject;
  AgsTaskClass *task;

  GParamSpec *param_spec;

  ags_resize_audio_parent_class = g_type_class_peek_parent(resize_audio);

  /* gobject */
  gobject = (GObjectClass *) resize_audio;

  gobject->set_property = ags_resize_audio_set_property;
  gobject->get_property = ags_resize_audio_get_property;

  gobject->dispose = ags_resize_audio_dispose;
  gobject->finalize = ags_resize_audio_finalize;

  /* properties */
  /**
   * AgsResizeAudio:audio:
   *
   * The assigned #AgsAudio
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_object("audio",
				   i18n_pspec("audio of resize audio"),
				   i18n_pspec("The audio of resize audio task"),
				   AGS_TYPE_AUDIO,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_AUDIO,
				  param_spec);

  /**
   * AgsResizeAudio:output-pads:
   *
   * The count of output pads to apply to audio.
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_uint("output-pads",
				 i18n_pspec("output pads"),
				 i18n_pspec("The count of output pads"),
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_OUTPUT_PADS,
				  param_spec);

  /**
   * AgsResizeAudio:input-pads:
   *
   * The count of input pads to apply to audio.
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_uint("input-pads",
				 i18n_pspec("input pads"),
				 i18n_pspec("The count of input pads"),
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_INPUT_PADS,
				  param_spec);

  /**
   * AgsResizeAudio:audio-channels:
   *
   * The count of audio channels to apply to audio.
   * 
   * Since: 3.0.0
   */
  param_spec = g_param_spec_uint("audio-channels",
				 i18n_pspec("audio channels"),
				 i18n_pspec("The count of audio channels"),
				 0,
				 G_MAXUINT,
				 0,
				 G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_AUDIO_CHANNELS,
				  param_spec);
  

  /* task */
  task = (AgsTaskClass *) resize_audio;

  task->launch = ags_resize_audio_launch;
}

void
ags_resize_audio_init(AgsResizeAudio *resize_audio)
{
  resize_audio->audio = NULL;
  resize_audio->output_pads = 0;
  resize_audio->input_pads = 0;
  resize_audio->audio_channels = 0;
}

void
ags_resize_audio_set_property(GObject *gobject,
			      guint prop_id,
			      const GValue *value,
			      GParamSpec *param_spec)
{
  AgsResizeAudio *resize_audio;

  resize_audio = AGS_RESIZE_AUDIO(gobject);

  switch(prop_id){
  case PROP_AUDIO:
    {
      AgsAudio *audio;

      audio = (AgsAudio *) g_value_get_object(value);

      if(resize_audio->audio == audio){
	return;
      }

      if(resize_audio->audio != NULL){
	g_object_unref(resize_audio->audio);
      }

      if(audio != NULL){
	g_object_ref(audio);
      }

      resize_audio->audio = audio;
    }
    break;
  case PROP_OUTPUT_PADS:
    {
      resize_audio->output_pads = g_value_get_uint(value);
    }
    break;
  case PROP_INPUT_PADS:
    {
      resize_audio->input_pads = g_value_get_uint(value);
    }
    break;
  case PROP_AUDIO_CHANNELS:
    {
      resize_audio->audio_channels = g_value_get_uint(value);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_resize_audio_get_property(GObject *gobject,
			      guint prop_id,
			      GValue *value,
			      GParamSpec *param_spec)
{
  AgsResizeAudio *resize_audio;

  resize_audio = AGS_RESIZE_AUDIO(gobject);

  switch(prop_id){
  case PROP_AUDIO:
    {
      g_value_set_object(value, resize_audio->audio);
    }
    break;
  case PROP_OUTPUT_PADS:
    {
      g_value_set_uint(value, resize_audio->output_pads);
    }
    break;
  case PROP_INPUT_PADS:
    {
      g_value_set_uint(value, resize_audio->input_pads);
    }
    break;
  case PROP_AUDIO_CHANNELS:
    {
      g_value_set_uint(value, resize_audio->audio_channels);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_resize_audio_dispose(GObject *gobject)
{
  AgsResizeAudio *resize_audio;

  resize_audio = AGS_RESIZE_AUDIO(gobject);

  if(resize_audio->audio != NULL){
    g_object_unref(resize_audio->audio);

    resize_audio->audio = NULL;
  }
  
  /* call parent */
  G_OBJECT_CLASS(ags_resize_audio_parent_class)->dispose(gobject);
}

void
ags_resize_audio_finalize(GObject *gobject)
{
  AgsResizeAudio *resize_audio;

  resize_audio = AGS_RESIZE_AUDIO(gobject);

  if(resize_audio->audio != NULL){
    g_object_unref(resize_audio->audio);
  }

  /* call parent */
  G_OBJECT_CLASS(ags_resize_audio_parent_class)->finalize(gobject);
}

void
ags_resize_audio_launch(AgsTask *task)
{
  AgsAudio *audio;
  
  AgsResizeAudio *resize_audio;

  guint audio_channels_old;
  guint input_pads_old, output_pads_old;

  resize_audio = AGS_RESIZE_AUDIO(task);

  g_return_if_fail(AGS_IS_AUDIO(resize_audio->audio));
  
  audio = resize_audio->audio;

  /* get some fields */
  g_object_get(audio,
	       "audio-channels", &audio_channels_old,
	       "output-pads", &output_pads_old,
	       "input-pads", &input_pads_old,
	       NULL);

  /* resize audio - audio channels */
  if(audio_channels_old != resize_audio->audio_channels){
    ags_audio_set_audio_channels(audio,
				 resize_audio->audio_channels, audio_channels_old);
  }
  
  /* resize audio - output */
  if(output_pads_old != resize_audio->output_pads){    
    ags_audio_set_pads(audio,
		       AGS_TYPE_OUTPUT,
		       resize_audio->output_pads, output_pads_old);
  }

  /* resize audio - input */
  if(input_pads_old != resize_audio->input_pads){
    ags_audio_set_pads(audio,
		       AGS_TYPE_INPUT,
		       resize_audio->input_pads, input_pads_old);
  }
}
  
/**
 * ags_resize_audio_new:
 * @audio: the #AgsAudio to resize
 * @output_pads: output pads
 * @input_pads: input pads
 * @audio_channels: audio channels
 *
 * Creates an #AgsResizeAudio.
 *
 * Returns: an new #AgsResizeAudio.
 *
 * Since: 3.0.0
 */
AgsResizeAudio*
ags_resize_audio_new(AgsAudio *audio,
		     guint output_pads,
		     guint input_pads,
		     guint audio_channels)
{
  AgsResizeAudio *resize_audio;

  resize_audio = (AgsResizeAudio *) g_object_new(AGS_TYPE_RESIZE_AUDIO,
						 "audio", audio,
						 "output-pads", output_pads,
						 "input-pads", input_pads,
						 "audio-channels", audio_channels,
						 NULL);

  return(resize_audio);
}
