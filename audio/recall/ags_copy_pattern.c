#include "ags_copy_pattern.h"

#include "../../object/ags_connectable.h"
#include "../../object/ags_run_connectable.h"

#include "../ags_audio.h"
#include "../ags_recycling.h"
#include "../ags_audio_signal.h"
#include "../ags_recall_id.h"
#include "../ags_recall_shared.h"

#include <stdlib.h>

GType ags_copy_pattern_get_type();
void ags_copy_pattern_class_init(AgsCopyPatternClass *copy_pattern);
void ags_copy_pattern_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_copy_pattern_run_connectable_interface_init(AgsRunConnectableInterface *run_connectable);
void ags_copy_pattern_init(AgsCopyPattern *copy_pattern);
void ags_copy_pattern_connect(AgsConnectable *connectable);
void ags_copy_pattern_disconnect(AgsConnectable *connectable);
void ags_copy_pattern_run_connect(AgsRunConnectable *run_connectable);
void ags_copy_pattern_run_disconnect(AgsRunConnectable *run_connectable);
void ags_copy_pattern_finalize(GObject *gobject);

void ags_copy_pattern_run_init_pre(AgsRecall *recall, guint audio_channel, gpointer data);

void ags_copy_pattern_done(AgsRecall *recall, gpointer data);
void ags_copy_pattern_cancel(AgsRecall *recall, gpointer data);
void ags_copy_pattern_remove(AgsRecall *recall, gpointer data);
AgsRecall* ags_copy_pattern_duplicate(AgsRecall *recall, AgsRecallID *recall_id);

void ags_copy_pattern_tic_callback(AgsDelay *delay, guint audio_channel,
				   AgsCopyPattern *copy_pattern);

static gpointer ags_copy_pattern_parent_class = NULL;
static AgsConnectableInterface* ags_copy_pattern_parent_connectable_interface;
static AgsRunConnectableInterface *ags_copy_pattern_parent_run_connectable_interface;

GType
ags_copy_pattern_get_type()
{
  static GType ags_type_copy_pattern = 0;

  if(!ags_type_copy_pattern){
    static const GTypeInfo ags_copy_pattern_info = {
      sizeof (AgsCopyPatternClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_copy_pattern_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsCopyPattern),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_copy_pattern_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_copy_pattern_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_run_connectable_interface_info = {
      (GInterfaceInitFunc) ags_copy_pattern_run_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_copy_pattern = g_type_register_static(AGS_TYPE_RECALL,
						   "AgsCopyPattern\0",
						   &ags_copy_pattern_info,
						   0);

    g_type_add_interface_static(ags_type_copy_pattern,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_copy_pattern,
				AGS_TYPE_RUN_CONNECTABLE,
				&ags_run_connectable_interface_info);
  }

  return(ags_type_copy_pattern);
}

void
ags_copy_pattern_class_init(AgsCopyPatternClass *copy_pattern)
{
  GObjectClass *gobject;
  AgsRecallClass *recall;

  ags_copy_pattern_parent_class = g_type_class_peek_parent(copy_pattern);

  gobject = (GObjectClass *) copy_pattern;

  gobject->finalize = ags_copy_pattern_finalize;

  recall = (AgsRecallClass *) copy_pattern;

  recall->duplicate = ags_copy_pattern_duplicate;
}

void
ags_copy_pattern_connectable_interface_init(AgsConnectableInterface *connectable)
{
  AgsConnectableInterface *ags_copy_pattern_connectable_parent_interface;

  ags_copy_pattern_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_copy_pattern_connect;
  connectable->disconnect = ags_copy_pattern_disconnect;
}

void
ags_copy_pattern_run_connectable_interface_init(AgsRunConnectableInterface *run_connectable)
{
  ags_copy_pattern_parent_run_connectable_interface = g_type_interface_peek_parent(run_connectable);

  run_connectable->connect = ags_copy_pattern_run_connect;
  run_connectable->disconnect = ags_copy_pattern_run_disconnect;
}

void
ags_copy_pattern_init(AgsCopyPattern *copy_pattern)
{
  copy_pattern->shared_channel = NULL;
  copy_pattern->shared_audio_run = NULL;
}

void
ags_copy_pattern_connect(AgsConnectable *connectable)
{
  AgsCopyPattern *copy_pattern;

  ags_copy_pattern_parent_connectable_interface->connect(connectable);

  /* AgsCopyPattern */
  copy_pattern = AGS_COPY_PATTERN(connectable);

  g_signal_connect((GObject *) copy_pattern, "run_init_pre\0",
		   G_CALLBACK(ags_copy_pattern_run_init_pre), NULL);

  g_signal_connect((GObject *) copy_pattern, "remove\0",
		   G_CALLBACK(ags_copy_pattern_remove), NULL);

  g_signal_connect((GObject *) copy_pattern, "cancel\0",
		   G_CALLBACK(ags_copy_pattern_cancel), NULL);
}

void
ags_copy_pattern_disconnect(AgsConnectable *connectable)
{
  ags_copy_pattern_parent_connectable_interface->disconnect(connectable);
}

void
ags_copy_pattern_run_connect(AgsRunConnectable *run_connectable)
{
  AgsCopyPattern *copy_pattern;

  ags_copy_pattern_parent_run_connectable_interface->connect(run_connectable);

  /* AgsCopyPattern */
  copy_pattern = AGS_COPY_PATTERN(run_connectable);

  copy_pattern->tic_handler =
    g_signal_connect(G_OBJECT(copy_pattern->shared_audio_run->delay), "tic\0",
		     G_CALLBACK(ags_copy_pattern_tic_callback), copy_pattern);
}

void
ags_copy_pattern_run_disconnect(AgsRunConnectable *run_connectable)
{
  AgsCopyPattern *copy_pattern;

  ags_copy_pattern_parent_run_connectable_interface->disconnect(run_connectable);

  /* AgsCopyPattern */
  copy_pattern = AGS_COPY_PATTERN(run_connectable);

  g_signal_handler_disconnect(G_OBJECT(copy_pattern), copy_pattern->tic_handler);
}

void
ags_copy_pattern_finalize(GObject *gobject)
{
  G_OBJECT_CLASS(ags_copy_pattern_parent_class)->finalize(gobject);
}

void
ags_copy_pattern_run_init_pre(AgsRecall *recall, guint audio_channel, gpointer data)
{
  AgsCopyPattern *copy_pattern;

  copy_pattern = AGS_COPY_PATTERN(recall);

  ags_recall_notify_dependency(AGS_RECALL(copy_pattern->shared_audio_run->delay),
			       AGS_RECALL_NOTIFY_CHANNEL_RUN, 1);
  copy_pattern->shared_audio_run->recall_ref++;
  fprintf(stdout, "ags_copy_pattern_run_init_pre\n\0");
}

void
ags_copy_pattern_done(AgsRecall *recall, gpointer data)
{
  AgsCopyPattern *copy_pattern;

  copy_pattern = AGS_COPY_PATTERN(recall);

  //  copy_pattern->shared_audio_run->delay->recall_ref--;
  //  copy_pattern->shared_audio_run->recall_ref--;
}

void
ags_copy_pattern_cancel(AgsRecall *recall, gpointer data)
{
  AgsCopyPattern *copy_pattern;

  copy_pattern = AGS_COPY_PATTERN(recall);

  ags_recall_notify_dependency(AGS_RECALL(copy_pattern->shared_audio_run->delay),
			       AGS_RECALL_NOTIFY_CHANNEL_RUN, -1);
  //  copy_pattern->shared_audio_run->recall_ref--;
}

void
ags_copy_pattern_remove(AgsRecall *recall, gpointer data)
{
  /* empty */
}

AgsRecall*
ags_copy_pattern_duplicate(AgsRecall *recall, AgsRecallID *recall_id)
{
  AgsAudio *audio;
  AgsCopyPattern *copy_pattern, *copy;
  GList *list;

  copy_pattern = (AgsCopyPattern *) recall;
  copy = (AgsCopyPattern *) AGS_RECALL_CLASS(ags_copy_pattern_parent_class)->duplicate(recall, recall_id);

  audio = AGS_AUDIO(copy_pattern->shared_channel->source->audio);
  list = ags_recall_shared_find_type_with_group_id(audio->recall_shared,
						   AGS_TYPE_COPY_PATTERN_SHARED_AUDIO_RUN, recall_id->group_id);

  if(list == NULL){
    AgsDelay *delay;
    AgsCopyPatternSharedAudio *copy_pattern_shared_audio;
    AgsCopyPatternSharedAudioRun *copy_pattern_shared_audio_run;
    AgsRecallID *parent_recall_id;

    parent_recall_id = ags_recall_id_find_group_id(copy_pattern->shared_channel->destination->recall_id,
						   recall_id->parent_group_id);

    if(parent_recall_id->parent_group_id == 0)
      delay = AGS_DELAY(ags_recall_find_type_with_group_id(audio->play, AGS_TYPE_DELAY, parent_recall_id->group_id)->data);
    else
      delay = AGS_DELAY(ags_recall_find_type_with_group_id(audio->recall, AGS_TYPE_DELAY, parent_recall_id->group_id)->data);
    
    copy_pattern_shared_audio = copy_pattern->shared_audio_run->copy_pattern_shared_audio;

    copy_pattern_shared_audio_run = ags_copy_pattern_shared_audio_run_new(copy_pattern_shared_audio,
									  delay, 0);
    AGS_RECALL_SHARED(copy_pattern_shared_audio_run)->group_id = recall_id->group_id;
    ags_run_connectable_connect(AGS_RUN_CONNECTABLE(copy_pattern_shared_audio_run));

    audio->recall_shared = 
      list = g_list_prepend(audio->recall_shared, copy_pattern_shared_audio_run);
  }

  copy->shared_audio_run = AGS_COPY_PATTERN_SHARED_AUDIO_RUN(list->data);

  copy->shared_channel = copy_pattern->shared_channel;

  return((AgsRecall *) copy);
}

void
ags_copy_pattern_tic_callback(AgsDelay *delay, guint audio_channel, AgsCopyPattern *copy_pattern)
{
  AgsChannel *output;
  AgsCopyPatternSharedAudio *copy_pattern_shared_audio;
  //  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  //  pthread_mutex_lock(&mutex);
  if(copy_pattern->shared_channel->source->audio_channel != audio_channel)
    return;

  copy_pattern_shared_audio = copy_pattern->shared_audio_run->copy_pattern_shared_audio;

  if(copy_pattern->shared_audio_run->bit == copy_pattern_shared_audio->length){
    //      pthread_mutex_unlock(&mutex);
    if(copy_pattern_shared_audio->loop){
	AgsRecycling *recycling;
	AgsAudioSignal *audio_signal;

	recycling = copy_pattern->shared_channel->destination->first_recycling;

	if(recycling != NULL){
	  while(recycling != copy_pattern->shared_channel->destination->last_recycling->next){
	    audio_signal = ags_audio_signal_new((GObject *) recycling,
						(GObject *) AGS_RECALL(copy_pattern)->recall_id);
	    audio_signal->devout = (GObject *) copy_pattern_shared_audio->devout;
	    ags_audio_signal_connect(audio_signal);
	    
	    ags_recycling_add_audio_signal(recycling,
					   audio_signal);
	    
	    recycling = recycling->next;
	  }
	}

	//	copy_pattern->shared_audio_run->bit = 0;
	goto ags_copy_pattern_pre0;
    }else{
      if((AGS_RECALL_PERSISTENT & (AGS_RECALL(delay)->flags)) != 0)
	AGS_RECALL(delay)->flags &= (~AGS_RECALL_PERSISTENT);

      if(copy_pattern->recall.child == NULL){
	//	fprintf(stdout, "copy_pattern->recall.recall == NULL\n\0");
	ags_recall_done((AgsRecall *) copy_pattern);

	ags_recall_notify_dependency(AGS_RECALL(delay),
				     AGS_RECALL_NOTIFY_CHANNEL_RUN, -1);
	copy_pattern->shared_audio_run->recall_ref--;
	//	copy_pattern->shared_audio_run->bit = 0;
      }
      //      pthread_mutex_unlock(&mutex);
    }
  }else{
    //    pthread_mutex_unlock(&mutex);
    ags_copy_pattern_pre0:
      
      if(ags_pattern_get_bit((AgsPattern *) copy_pattern->shared_channel->source->pattern->data,//copy_pattern->shared_channel->pattern,
			     copy_pattern_shared_audio->i, copy_pattern_shared_audio->j,
			     copy_pattern->shared_audio_run->bit)){
	AgsRecycling *recycling;
	AgsAudioSignal *audio_signal;
	
	fprintf(stdout, "ags_copy_pattern - playing: bit = %u; line = %u\n\0", copy_pattern->shared_audio_run->bit, copy_pattern->shared_channel->source->line);

	recycling = copy_pattern->shared_channel->source->first_recycling;
	
	if(recycling != NULL){
	  while(recycling != copy_pattern->shared_channel->source->last_recycling->next){
	    ags_audio_signal_new((GObject *) recycling,
				 (GObject *) AGS_RECALL(copy_pattern)->recall_id);
	    audio_signal->devout = (GObject *) copy_pattern_shared_audio->devout;
	    ags_audio_signal_connect(audio_signal);
	    
	    ags_recycling_add_audio_signal(recycling,
					   audio_signal);

	    recycling = recycling->next;
	  }
	}
      }
      
      //      printf("%u\n\0", copy_pattern->shared_audio_run->bit);
      //      copy_pattern->shared_audio_run->bit++;
  }
}

AgsCopyPattern*
ags_copy_pattern_new(AgsCopyPatternSharedAudioRun *shared_audio_run,
		     AgsCopyPatternSharedChannel *shared_channel)
{
  AgsCopyPattern *copy_pattern;

  copy_pattern = (AgsCopyPattern *) g_object_new(AGS_TYPE_COPY_PATTERN, NULL);

  copy_pattern->shared_audio_run = shared_audio_run;
  copy_pattern->shared_channel = shared_channel;

  return(copy_pattern);
}
