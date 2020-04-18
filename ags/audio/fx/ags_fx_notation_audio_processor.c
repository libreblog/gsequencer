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

#include <ags/audio/fx/ags_fx_notation_audio_processor.h>

void ags_fx_notation_audio_processor_class_init(AgsFxNotationAudioProcessorClass *fx_notation_audio_processor);
void ags_fx_notation_audio_processor_init(AgsFxNotationAudioProcessor *fx_notation_audio_processor);
void ags_fx_notation_audio_processor_dispose(GObject *gobject);
void ags_fx_notation_audio_processor_finalize(GObject *gobject);

void ags_fx_notation_audio_processor_real_run_inter(AgsRecall *recall);

void ags_fx_notation_audio_processor_real_key_on(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
						 AgsNote *note,
						 guint velocity);
void ags_fx_notation_audio_processor_real_key_off(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
						  AgsNote *note,
						  guint velocity);
void ags_fx_notation_audio_processor_real_key_pressure(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
						       AgsNote *note,
						       guint velocity);

void ags_fx_notation_audio_processor_real_play(AgsFxNotationAudioProcessor *fx_notation_audio_processor);
void ags_fx_notation_audio_processor_real_record(AgsFxNotationAudioProcessor *fx_notation_audio_processor);
void ags_fx_notation_audio_processor_real_feed(AgsFxNotationAudioProcessor *fx_notation_audio_processor);

void ags_fx_notation_audio_processor_real_counter_change(AgsFxNotationAudioProcessor *fx_notation_audio_processor);

void ags_fx_notation_audio_processor_notify_output_soundcard_callback(GObject *gobject,
								      GParamSpec *pspec,
								      gpointer user_data);

/**
 * SECTION:ags_fx_notation_audio_processor
 * @short_description: fx notation audio_processor
 * @title: AgsFxNotationAudioProcessor
 * @section_id:
 * @include: ags/audio/fx/ags_fx_notation_audio_processor.h
 *
 * The #AgsFxNotationAudioProcessor class provides ports to the effect processor.
 */

static gpointer ags_fx_notation_audio_processor_parent_class = NULL;

GType
ags_fx_notation_audio_processor_get_type()
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_fx_notation_audio_processor = 0;

    static const GTypeInfo ags_fx_notation_audio_processor_info = {
      sizeof (AgsFxNotationAudioProcessorClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_fx_notation_audio_processor_class_init,
      NULL, /* class_finalize */
      NULL, /* class_audio_processor */
      sizeof (AgsFxNotationAudioProcessor),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_fx_notation_audio_processor_init,
    };

    ags_type_fx_notation_audio_processor = g_type_register_static(AGS_TYPE_RECALL_AUDIO_PROCESSOR,
								  "AgsFxNotationAudioProcessor",
								  &ags_fx_notation_audio_processor_info,
								  0);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_fx_notation_audio_processor);
  }

  return g_define_type_id__volatile;
}

void
ags_fx_notation_audio_processor_class_init(AgsFxNotationAudioProcessorClass *fx_notation_audio_processor)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;
  
  ags_fx_notation_audio_processor_parent_class = g_type_class_peek_parent(fx_notation_audio_processor);

  /* GObjectClass */
  gobject = (GObjectClass *) fx_notation_audio_processor;

  gobject->dispose = ags_fx_notation_audio_processor_dispose;
  gobject->finalize = ags_fx_notation_audio_processor_finalize;

  /* AgsRecallClass */
  recall = (AgsRecallClass *) fx_notation_audio_processor;
  
  recall->run_inter = ags_fx_notation_audio_processor_real_run_inter;
  
  /* AgsFxNotationAudioProcessorClass */
  fx_notation_audio_processor->key_on = ags_fx_notation_audio_processor_real_key_on;
  fx_notation_audio_processor->key_off = ags_fx_notation_audio_processor_real_key_off;
  fx_notation_audio_processor->key_pressure = ags_fx_notation_audio_processor_real_key_pressure;

  fx_notation_audio_processor->play = ags_fx_notation_audio_processor_real_play;
  fx_notation_audio_processor->record = ags_fx_notation_audio_processor_real_record;
  fx_notation_audio_processor->feed = ags_fx_notation_audio_processor_real_feed;

  fx_notation_audio_processor->counter_change = ags_fx_notation_audio_processor_real_counter_change;
}

void
ags_fx_notation_audio_processor_init(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  g_signal_connect(fx_notation_audio_processor, "notify::output-soundcard",
		   G_CALLBACK(ags_fx_notation_audio_processor_notify_output_soundcard_callback), NULL);

  /* counter */
  fx_notation_audio_processor->delay_counter = 0.0;
  fx_notation_audio_processor->offset_counter = 0;

  /* timestamp */
  fx_notation_audio_processor->timestamp = ags_timestamp_new();
  g_object_ref(fx_notation_audio_processor->timestamp);
  
  fx_notation_audio_processor->timestamp->flags &= (~AGS_TIMESTAMP_UNIX);
  fx_notation_audio_processor->timestamp->flags |= AGS_TIMESTAMP_OFFSET;

  fx_notation_audio_processor->timestamp->timer.ags_offset.offset = 0;

  /* recording note */
  fx_notation_audio_processor->recording_note = NULL;

  /* feeding note */
  fx_notation_audio_processor->feeding_note = NULL;
}

void
ags_fx_notation_audio_processor_dispose(GObject *gobject)
{
  AgsFxNotationAudioProcessor *fx_notation_audio_processor;
  
  fx_notation_audio_processor = AGS_FX_NOTATION_AUDIO_PROCESSOR(gobject);
  
  /* call parent */
  G_OBJECT_CLASS(ags_fx_notation_audio_processor_parent_class)->dispose(gobject);
}

void
ags_fx_notation_audio_processor_finalize(GObject *gobject)
{
  AgsFxNotationAudioProcessor *fx_notation_audio_processor;
  
  fx_notation_audio_processor = AGS_FX_NOTATION_AUDIO_PROCESSOR(gobject);  

  /* timestamp */
  if(fx_notation_audio_processor->timestamp != NULL){
    g_object_unref((GObject *) fx_notation_audio_processor->timestamp);
  }

  /* recording note */
  if(fx_notation_audio_processor->recording_note != NULL){
    g_list_free_full(fx_notation_audio_processor->recording_note,
		     (GDestroyNotify) g_object_unref);
  }

  /* feeding note */
  if(fx_notation_audio_processor->feeding_note != NULL){
    g_list_free_full(fx_notation_audio_processor->feeding_note,
		     (GDestroyNotify) g_object_unref);
  }
  
  /* call parent */
  G_OBJECT_CLASS(ags_fx_notation_audio_processor_parent_class)->finalize(gobject);
}

void
ags_fx_notation_audio_processor_real_run_inter(AgsRecall *recall)
{
  AgsFxNotationAudioProcessor *fx_notation_audio_processor;

  gdouble delay_counter;
  
  GRecMutex *fx_notation_audio_processor_mutex;

  fx_notation_audio_processor = AGS_FX_NOTATION_AUDIO_PROCESSOR(recall);
  
  fx_notation_audio_processor_mutex = AGS_RECALL_GET_OBJ_MUTEX(fx_notation_audio_processor);

  /* get delay counter */
  g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
  delay_counter = fx_notation_audio_processor->delay_counter;

  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

  /* run */
  if(delay_counter == 0.0){
    ags_fx_notation_audio_processor_play(fx_notation_audio_processor);
  }

  ags_fx_notation_audio_processor_record(fx_notation_audio_processor);

  ags_fx_notation_audio_processor_feed(fx_notation_audio_processor);
  
  ags_fx_notation_audio_processor_counter_change(fx_notation_audio_processor);
  
  /* call parent */
  AGS_RECALL_CLASS(ags_fx_notation_audio_processor_parent_class)->run_inter(recall);
}

void
ags_fx_notation_audio_processor_real_key_on(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
					    AgsNote *note,
					    guint velocity)
{
  AgsAudio *audio;
  AgsChannel *start_input;
  AgsChannel *input, *selected_input;
  AgsRecallID *recall_id;
  AgsRecyclingContext *recycling_context;
  AgsFxNotationAudio *fx_notation_audio;
  AgsPort *port;

  gdouble delay;
  guint64 offset_counter;
  guint input_pads;
  guint audio_channel;
  guint y;
  guint i;

  GValue value = {0,};

  GRecMutex *fx_notation_audio_processor_mutex;

  fx_notation_audio_processor_mutex = AGS_RECALL_GET_OBJ_MUTEX(fx_notation_audio_processor);
  
  g_object_get(fx_notation_audio_processor,
	       "audio", &audio,
	       "input", &start_input,
	       "recall-id", &recall_id,
	       "recall-audio", &fx_notation_audio,
	       "audio-channel", &audio_channel,
	       NULL);
  
  g_object_get(recall_id,
	       "recycling-context", &recycling_context,
	       NULL);

  g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
  offset_counter = fx_notation_audio_processor->offset_counter;

  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

  /* get delay */
  delay = AGS_SOUNDCARD_DEFAULT_DELAY;
  
  fx_notation_audio_mutex = NULL;

  if(fx_notation_audio != NULL){        
    g_object_get(fx_notation_audio,
		 "delay", &port,
		 NULL);

    if(port != NULL){
      g_value_init(&value,
		   G_TYPE_DOUBLE);
    
      ags_port_safe_read(port,
			 &value);

      delay = g_value_get_double(&value);
      g_value_unset(&value);

      g_object_unref(port);
    }
  }

  g_object_get(audio,
	       "input-pads", &input_pads,
	       NULL);
  
  g_object_get(note,
	       "y", &y,
	       NULL);
  
  input = ags_channel_nth(start_input,
			  audio_channel);

  if(ags_audio_test_behaviour_flags(audio, AGS_SOUND_BEHAVIOUR_REVERSE_MAPPING)){
    selected_input = ags_channel_pad_nth(input,
					 input_pads - y - 1);
  }else{
    selected_input = ags_channel_pad_nth(input,
					 y);
  }
  
  if(selected_input != NULL){
    AgsRecycling *first_recycling, *last_recycling;
    AgsRecycling *recycling, *next_recycling;
    AgsRecycling *end_recycling;
    AgsRecallID *child_recall_id;
    
    GObject *output_soundcard;

    GList *start_list, *list;

    g_object_get(selected_input,
		 "output-soundcard", &output_soundcard,
		 "first-recycling", &first_recycling,
		 "last-recycling", &last_recycling,
		 NULL);

    end_recycling = ags_recycling_next(last_recycling);

    /* get child recall id */
    g_object_get(selected_channel,
		 "recall-id", &start_list,
		 NULL);

    list = start_list;
    child_recall_id = NULL;

    while(child_recall_id == NULL &&
	  list != NULL){
      AgsRecallID *current_recall_id;
      AgsRecyclingContext *current_recycling_context, *current_parent_recycling_context;

      g_object_get(list->data,
		   "recycling-context", &current_recycling_context,
		   NULL);

      g_object_get(current_recycling_context,
		   "parent", &current_parent_recycling_context,
		   NULL);
	  
      if(current_parent_recycling_context == recycling_context){
	child_recall_id = (AgsRecallID *) list->data;
	g_object_ref(child_recall_id);
      }

      if(current_recycling_context != NULL){
	g_object_unref(current_recycling_context);
      }

      if(current_parent_recycling_context != NULL){
	g_object_unref(current_parent_recycling_context);
      }
      
      /* iterate */
      list = list->next;
    }
    
    while(recycling != end_recycling){
      /* create audio signal */
      audio_signal = ags_audio_signal_new((GObject *) output_soundcard,
					  (GObject *) recycling,
					  (GObject *) child_recall_id);
      g_object_set(audio_signal,
		   "note", note,
		   NULL);
	  
      if(ags_audio_test_behaviour_flags(audio, AGS_SOUND_BEHAVIOUR_PATTERN_MODE)){
	ags_recycling_create_audio_signal_with_defaults(recycling,
							audio_signal,
							0.0, 0);
      }else{
	guint note_x0, note_x1;
	guint buffer_size;

	note_x0 = (guint) offset_counter;

	g_object_get(note,
		     "x1", &note_x1,
		     NULL);

	buffer_size = audio_signal->buffer_size;
	
	/* create audio signal with frame count */
	ags_recycling_create_audio_signal_with_frame_count(recycling,
							   audio_signal,
							   (guint) (((gdouble) buffer_size * delay) * (gdouble) (note_x1 - note_x0)),
							   0.0, 0);
      }
	  
      audio_signal->stream_current = audio_signal->stream;
	    
      ags_connectable_connect(AGS_CONNECTABLE(audio_signal));
      ags_recycling_add_audio_signal(recycling,
				     audio_signal);

      /* iterate */
      next_recycling = ags_recycling_next(recycling);

      g_object_unref(recycling);

      recycling = next_recycling;
    }

    if(output_soundcard != NULL){
      g_object_unref(output_soundcard);
    }

    if(first_recycling != NULL){
      g_object_unref(first_recycling);
    }

    g_list_free_full(start_list,
		     g_object_unref);

    if(child_recall_id != NULL){
      g_object_unref(child_recall_id);
    }
  }

  if(audio != NULL){
    g_object_unref(audio);
  }
  
  if(start_input != NULL){
    g_object_unref(start_input);
  }

  if(input != NULL){
    g_object_unref(input);
  }

  if(selected_input != NULL){
    g_object_unref(selected_input);
  }

  if(recall_id != NULL){
    g_object_unref(recall_id);
  }
  
  if(fx_notation_audio != NULL){
    g_object_unref(fx_notation_audio);
  }
}

void
ags_fx_notation_audio_processor_key_on(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
				       AgsNote *note,
				       guint velocity)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->key_on(fx_notation_audio_processor,
										 note,
										 velocity);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_real_key_off(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
					     AgsNote *note,
					     guint velocity)
{
  //TODO:JK: implement me
}

void
ags_fx_notation_audio_processor_key_off(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
					AgsNote *note,
					guint velocity)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->key_off(fx_notation_audio_processor,
										  note,
										  velocity);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_real_key_pressure(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
						  AgsNote *note,
						  guint velocity)
{
  //TODO:JK: implement me
}

void
ags_fx_notation_audio_processor_key_pressure(AgsFxNotationAudioProcessor *fx_notation_audio_processor,
					     AgsNote *note,
					     guint velocity)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->key_pressure(fx_notation_audio_processor,
										       note,
										       velocity);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_real_play(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  AgsAudio *audio;

  AgsTimestamp *timestamp;
  
  GList *start_notation, *notation;

  guint64 offset_counter;
  guint audio_channel;
  
  GRecMutex *fx_notation_audio_processor_mutex;

  fx_notation_audio_processor_mutex = AGS_RECALL_GET_OBJ_MUTEX(fx_notation_audio_processor);
  
  g_object_get(fx_notation_audio_processor,
	       "audio", &audio,
	       "audio-channel", &audio_channel,
	       NULL);

  if(audio == NULL){
    return;
  }
  
  g_object_get(audio,
	       "notation", &start_notation,
	       NULL);
  
  /* timestamp and offset counter */
  g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
  timestamp = fx_notation_audio_processor->timestamp;
  
  offset_counter = fx_notation_audio_processor->offset_counter;

  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

  ags_timestamp_set_ags_offset(timestamp,
			       AGS_NOTATION_DEFAULT_OFFSET * floor(offset_counter / AGS_NOTATION_DEFAULT_OFFSET));

  /* find near timestamp */
  notation = ags_notation_find_near_timestamp(start_notation, audio_channel,
					      timestamp);

  if(notation != NULL){
    GList *start_note, *note;
    
    start_note = ags_notation_find_offset(notation->data,
					  offset_counter,
					  FALSE);

    note = start_note;

    while(note != NULL){
      ags_fx_notation_audio_processor_key_on(fx_notation_audio_processor,
					     note->data,
					     AGS_FX_NOTATION_AUDIO_PROCESSOR_DEFAULT_KEY_ON_VELOCITY);

      /* iterate */
      note = note->next;
    }

    g_list_free_full(start_note,
		     (GDestroyNotify) g_object_unref);
  }

  g_object_unref(audio);
  
  g_list_free_full(start_notation,
		   (GDestroyNotify) g_object_unref);  
}

void
ags_fx_notation_audio_processor_play(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->play(fx_notation_audio_processor);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_real_record(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  AgsAudio *audio;
  AgsNotation *current_notation;
  
  AgsTimestamp *timestamp;
  
  GObject *input_sequencer;

  GList *start_notation, *notation;
  GList *start_note, *note;

  guchar *midi_buffer;

  guint input_pads;
  guint audio_start_mapping;
  guint midi_start_mapping, midi_end_mapping;
  guint midi_channel;
  guint64 offset_counter;
  guint audio_channel;
  gboolean reverse_mapping;
  gboolean pattern_mode;

  GRecMutex *fx_notation_audio_processor_mutex;

  fx_notation_audio_processor_mutex = AGS_RECALL_GET_OBJ_MUTEX(fx_notation_audio_processor);

  g_object_get(fx_notation_audio_processor,
	       "audio", &audio,
	       "audio-channel", &audio_channel,
	       NULL);

  if(audio == NULL){
    return;
  }

  g_object_get(audio,
	       "input-sequencer", &input_sequencer,
	       "input-pads", &input_pads,
	       "audio-start-mapping", &audio_start_mapping,
	       "midi-start-mapping", &midi_start_mapping,
	       "midi-end-mapping", &midi_end_mapping,
	       "midi-channel", &midi_channel,
	       "midi-channel", &midi_channel,
	       NULL);

  if(input_sequencer == NULL){
    g_object_unref(audio);
    
    return;
  }

  current_notation = NULL;
  
  start_notation = NULL;
  start_note = NULL;

  g_object_get(audio,
	       "notation", &start_notation,
	       NULL);

  /* timestamp and offset counter */
  g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
  timestamp = fx_notation_audio_processor->timestamp;
  
  offset_counter = fx_notation_audio_processor->offset_counter;

  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

  ags_timestamp_set_ags_offset(timestamp,
			       AGS_NOTATION_DEFAULT_OFFSET * floor(offset_counter / AGS_NOTATION_DEFAULT_OFFSET));

  /* test flags */
  reverse_mapping = ags_audio_test_behaviour_flags(audio,
						   AGS_SOUND_BEHAVIOUR_REVERSE_MAPPING);
  
  pattern_mode = ags_audio_test_behaviour_flags(audio,
						AGS_SOUND_BEHAVIOUR_PATTERN_MODE);
  
  /* find near timestamp */
  notation = ags_notation_find_near_timestamp(start_notation, audio_channel,
					      timestamp);

  if(notation != NULL){
    current_notation = notation->data;
  }
 
  /* retrieve buffer */
  midi_buffer = ags_sequencer_get_buffer(AGS_SEQUENCER(input_sequencer),
					 &buffer_length);
  
  ags_sequencer_lock_buffer(AGS_SEQUENCER(input_sequencer),
			    midi_buffer);

  if(midi_buffer != NULL){
    guchar *midi_iter;
    
    /* parse bytes */
    midi_iter = midi_buffer;

    if(ags_midi_util_is_key_on(midi_iter)){
      /* check midi channel */
      if(midi_channel == (0x0f & midi_iter[0])){
	AgsNote *current_note;

	GList *start_recording_note, *recording_note;
	
	gint y;

	current_note = NULL;
	y = -1;
	
	/* check mapping */
	if((0x7f & midi_iter[1]) >= midi_start_mapping &&
	   (0x7f & midi_iter[1]) <= midi_end_mapping){
	  /* check channel */
	  if(!reverse_mapping){
	    y = audio_start_mapping + ((0x7f & midi_iter[1]) - midi_start_mapping);
	  }else{
	    y = input_pads - (audio_start_mapping + ((0x7f & midi_iter[1]) - midi_start_mapping)) - 1;
	  }
	}

	if(y >= 0 &&
	   y < input_pads){
	  g_rec_mutex_lock(fx_notation_audio_processor_mutex);

	  start_recording_note = g_list_copy_deep(fx_notation_audio_processor->recording_note,
						  (GCopyFunc) g_object_ref,
						  NULL);
	  
	  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

	  recording_note = start_recording_note;

	  while(recording_note != NULL){
	    guint current_y;
	    
	    g_object_get(recording_note->data,
			 "y", &current_y,
			 NULL);
	    
	    if(current_y == y){
	      current_note = recording_note->data;
	      
	      break;
	    }
	    
	    /* iterate */
	    recording_note = recording_note->next;
	  }
	  
	  if(current_note == NULL){
	    current_note = ags_note_new();
	    
	    current_note->x[0] = offset_counter;
	    current_note->x[1] = offset_counter + 1;
	      
	    current_note->y = y;
		
	    if(!pattern_mode){
	      fx_notation_audio_processor->recording_note = g_list_prepend(fx_notation_audio_processor->recording_note,
									   current_note);
	      g_object_ref(current_note);

	      ags_note_set_flags(current_note,
				 AGS_NOTE_FEED);
	    }

	    g_object_ref(current_note);
	    start_note = g_list_prepend(start_note,
					current_note);
	    
	    /* check notation */
	    if(current_notation == NULL){
	      current_notation = ags_notation_new((GObject *) audio,
						  audio_channel);

	      ags_timestamp_set_ags_offset(current_notation->timestamp,
					   ags_timestamp_get_ags_offset(timestamp));

	      ags_audio_add_notation(audio,
				     (GObject *) current_notation);
	    }

	    /* add note */
	    ags_notation_add_note(current_notation,
				  current_note,
				  FALSE);
	  }else{
	    if((0x7f & (midi_iter[2])) == 0){
	      /* note-off */
	      ags_note_unset_flags(current_note,
				   AGS_NOTE_FEED);

	      fx_notation_audio_processor->recording_note = g_list_remove(fx_notation_audio_processor->recording_note,
									  current_note);
	      g_object_unref(current_note);
	    }
	  }

	  g_list_free_full(start_recording_note,
			   (GDestroyNotify) g_object_unref);
	}
      }
	
      midi_iter += 3;
    }else if(ags_midi_util_is_key_off(midi_iter)){
      /* check midi channel */
      if(midi_channel == (0x0f & midi_iter[0])){
	AgsNote *current_note;

	GList *start_recording_note, *recording_note;
	
	gint y;

	current_note = NULL;
	y = -1;
	
	/* check mapping */
	if((0x7f & midi_iter[1]) >= midi_start_mapping &&
	   (0x7f & midi_iter[1]) <= midi_end_mapping){
	  /* check channel */
	  if(!reverse_mapping){
	    y = audio_start_mapping + ((0x7f & midi_iter[1]) - midi_start_mapping);
	  }else{
	    y = input_pads - (audio_start_mapping + ((0x7f & midi_iter[1]) - midi_start_mapping)) - 1;
	  }
	}

	if(y >= 0 &&
	   y < input_pads){
	  g_rec_mutex_lock(fx_notation_audio_processor_mutex);

	  start_recording_note = g_list_copy_deep(fx_notation_audio_processor->recording_note,
						  (GCopyFunc) g_object_ref,
						  NULL);
	  
	  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

	  recording_note = start_recording_note;

	  while(recording_note != NULL){
	    guint current_y;
	    
	    g_object_get(recording_note->data,
			 "y", &current_y,
			 NULL);
	    
	    if(current_y == y){
	      current_note = recording_note->data;
	      
	      break;
	    }
	    
	    /* iterate */
	    recording_note = recording_note->next;
	  }
	  
	  if(current_note != NULL){
	    ags_note_unset_flags(current_note,
				 AGS_NOTE_FEED);
	      
	    fx_notation_audio_processor->recording_note = g_list_remove(fx_notation_audio_processor->recording_note,
									current_note);
	    g_object_unref(current_note);
	  }
	}
      }
      
      midi_iter += 3;
    }else if(ags_midi_util_is_key_pressure(midi_iter)){
      midi_iter += 3;
    }else if(ags_midi_util_is_change_parameter(midi_iter)){
      /* change parameter */
      //TODO:JK: implement me	  
	  
      midi_iter += 3;
    }else if(ags_midi_util_is_pitch_bend(midi_iter)){
      /* change parameter */
      //TODO:JK: implement me	  
	  
      midi_iter += 3;
    }else if(ags_midi_util_is_change_program(midi_iter)){
      /* change program */
      //TODO:JK: implement me	  
	  
      midi_iter += 2;
    }else if(ags_midi_util_is_change_pressure(midi_iter)){
      /* change pressure */
      //TODO:JK: implement me	  
	  
      midi_iter += 2;
    }else if(ags_midi_util_is_sysex(midi_iter)){
      guint n;
	  
      /* sysex */
      n = 0;
	  
      while(midi_iter[n] != 0xf7){
	n++;
      }

      //TODO:JK: implement me	  
	  
      midi_iter += (n + 1);
    }else if(ags_midi_util_is_song_position(midi_iter)){
      /* song position */
      //TODO:JK: implement me	  
	  
      midi_iter += 3;
    }else if(ags_midi_util_is_song_select(midi_iter)){
      /* song select */
      //TODO:JK: implement me	  
	  
      midi_iter += 2;
    }else if(ags_midi_util_is_tune_request(midi_iter)){
      /* tune request */
      //TODO:JK: implement me	  
	  
      midi_iter += 1;
    }else if(ags_midi_util_is_meta_event(midi_iter)){
      /* meta event */
      //TODO:JK: implement me	  
	  
      midi_iter += (3 + midi_iter[2]);
    }else{
      g_warning("ags_fx_notation_audio_processor.c - unexpected byte %x", midi_iter[0]);
	  
      midi_iter++;
    }
  }
  
  ags_sequencer_unlock_buffer(AGS_SEQUENCER(input_sequencer),
			      midi_buffer);

  note = start_note;

  while(note != NULL){
    ags_fx_notation_audio_processor_key_on(fx_notation_audio_processor,
					   note->data,
					   AGS_FX_NOTATION_AUDIO_PROCESSOR_DEFAULT_KEY_ON_VELOCITY);
    
    note = note->next;
  }
  
  /*  */
  g_object_unref(audio);
  
  g_object_unref(input_sequencer);

  g_list_free_full(start_notation,
		   (GDestroyNotify) g_object_unref);

  g_list_free_full(start_note,
		   (GDestroyNotify) g_object_unref);
}

void
ags_fx_notation_audio_processor_record(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->record(fx_notation_audio_processor);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_real_feed(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  AgsFxNotationAudio *fx_notation_audio;

  //TODO:JK: implement me
}

void
ags_fx_notation_audio_processor_feed(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->feed(fx_notation_audio_processor);
  g_object_unref(fx_notation_audio_processor);
}

void
ags_fx_notation_audio_processor_notify_output_soundcard_callback(GObject *gobject,
								 GParamSpec *pspec,
								 gpointer user_data)
{
  AgsFxNotationAudioProcessor *fx_notation_audio_processor;

  fx_notation_audio_processor = AGS_FX_NOTATION_AUDIO_PROCESSOR(gobject);
  
  //TODO:JK: implement me
}

void
ags_fx_notation_audio_processor_real_counter_change(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  AgsFxNotationAudio *fx_notation_audio;

  gdouble delay;
  gdouble delay_counter;
  guint64 offset_counter;
  gboolean loop;
  guint64 loop_start, loop_end;
  
  GValue value = {0,};

  GRecMutex *fx_notation_audio_processor_mutex;
  
  fx_notation_audio_processor = NULL;
  fx_notation_audio_processor_mutex = AGS_RECALL_GET_OBJ_MUTEX(fx_notation_audio_processor);

  g_object_get(fx_notation_audio_processor,
	       "recall-audio", &fx_notation_audio,
	       NULL);

  g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
  delay_counter = fx_notation_audio_processor->delay_counter;
  offset_counter = fx_notation_audio_processor->offset_counter;

  g_rec_mutex_unlock(fx_notation_audio_processor_mutex);

  delay = AGS_SOUNDCARD_DEFAULT_DELAY;

  loop = FALSE;

  loop_start = AGS_FX_NOTATION_AUDIO_DEFAULT_LOOP_START;
  loop_end = AGS_FX_NOTATION_AUDIO_DEFAULT_LOOP_END;
  
  if(fx_notation_audio != NULL){
    AgsPort *port;

    /* delay */
    g_object_get(fx_notation_audio,
		 "delay", &port,
		 NULL);

    if(port != NULL){
      g_value_init(&value,
		   G_TYPE_DOUBLE);
    
      ags_port_safe_read(port,
			 &value);

      delay = g_value_get_double(&value);
      g_value_unset(&value);

      g_object_unref(port);
    }

    /* loop */
    g_object_get(fx_notation_audio,
		 "loop", &port,
		 NULL);

    if(port != NULL){
      g_value_init(&value,
		   G_TYPE_BOOLEAN);
    
      ags_port_safe_read(port,
			 &value);

      loop = g_value_get_boolean(&value);
      g_value_unset(&value);

      g_object_unref(port);
    }

    /* loop-start */
    g_object_get(fx_notation_audio,
		 "loop-start", &port,
		 NULL);

    if(port != NULL){
      g_value_init(&value,
		   G_TYPE_UINT4);
    
      ags_port_safe_read(port,
			 &value);

      loop_start = g_value_get_uint64(&value);
      g_value_unset(&value);

      g_object_unref(port);
    }

    /* loop-end */
    g_object_get(fx_notation_audio,
		 "loop-end", &port,
		 NULL);

    if(port != NULL){
      g_value_init(&value,
		   G_TYPE_UINT4);
    
      ags_port_safe_read(port,
			 &value);

      loop_end = g_value_get_uint64(&value);
      g_value_unset(&value);

      g_object_unref(port);
    }
  }

  if(delay + 1.0 >= delay){
    g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
    fx_notation_audio_processor->delay_counter = 0.0;

    if(loop &&
       fx_notation_audio_processor->offset_counter + 1 >= loop_end){
      fx_notation_audio_processor->offset_counter = loop_start;
    }else{
      fx_notation_audio_processor->offset_counter += 1;
    }
    
    g_rec_mutex_unlock(fx_notation_audio_processor_mutex);
  }else{
    g_rec_mutex_lock(fx_notation_audio_processor_mutex);
    
    fx_notation_audio_processor->delay_counter += 1.0;

    g_rec_mutex_unlock(fx_notation_audio_processor_mutex);
  }
  
  if(fx_notation_audio != NULL){
    g_object_unref(fx_notation_audio);
  }
}

void
ags_fx_notation_audio_processor_counter_change(AgsFxNotationAudioProcessor *fx_notation_audio_processor)
{
  g_return_if_fail(AGS_IS_FX_NOTATION_AUDIO_PROCESSOR(fx_notation_audio_processor));

  g_object_ref(fx_notation_audio_processor);
  AGS_FX_NOTATION_AUDIO_PROCESSOR_GET_CLASS(fx_notation_audio_processor)->counter_change(fx_notation_audio_processor);
  g_object_unref(fx_notation_audio_processor);
}

/**
 * ags_fx_notation_audio_processor_new:
 * @audio: the #AgsAudio
 *
 * Create a new instance of #AgsFxNotationAudioProcessor
 *
 * Returns: the new #AgsFxNotationAudioProcessor
 *
 * Since: 3.3.0
 */
AgsFxNotationAudioProcessor*
ags_fx_notation_audio_processor_new(AgsAudio *audio)
{
  AgsFxNotationAudioProcessor *fx_notation_audio_processor;

  fx_notation_audio_processor = (AgsFxNotationAudioProcessor *) g_object_new(AGS_TYPE_FX_NOTATION_AUDIO_PROCESSOR,
									     "audio", audio,
									     NULL);

  return(fx_notation_audio_processor);
}
