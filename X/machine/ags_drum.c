#include "ags_drum.h"
#include "ags_drum_callbacks.h"

#include "../../object/ags_connectable.h"

#include "ags_drum_input_pad.h"
#include "ags_drum_input_line.h"
#include "ags_drum_output_pad.h"
#include "ags_drum_output_line.h"

#include "../ags_window.h"
#include "../ags_menu_bar.h"
#include "../ags_pad.h"
#include "../ags_line.h"

#include "../../audio/ags_audio.h"
#include "../../audio/ags_channel.h"
#include "../../audio/ags_input.h"
#include "../../audio/ags_output.h"
#include "../../audio/ags_recycling.h"
#include "../../audio/ags_pattern.h"
#include "../../audio/ags_audio_signal.h"
#include "../../audio/ags_recall.h"

#include "../../audio/recall/ags_delay.h"

#include <math.h>

GType ags_drum_get_type(void);
void ags_drum_class_init(AgsDrumClass *drum);
void ags_drum_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_drum_init(AgsDrum *drum);
void ags_drum_connect(AgsConnectable *connectable);
void ags_drum_destroy(GtkObject *drum);
void ags_drum_show(GtkWidget *widget);

void ags_drum_set_audio_channels(AgsAudio *audio,
				 guint audio_channels, guint audio_channels_old,
				 gpointer data);
void ags_drum_set_pads(AgsAudio *audio, GType type,
		       guint pads, guint pads_old,
		       gpointer data);

extern void ags_file_read_drum(AgsFile *file, AgsMachine *machine);
extern void ags_file_write_drum(AgsFile *file, AgsMachine *machine);

static AgsConnectableInterface *ags_drum_parent_connectable_interface;

const char *AGS_DRUM_INDEX = "AgsDrumIndex\0";

GType
ags_drum_get_type(void)
{
  static GType ags_type_drum = 0;

  if(!ags_type_drum){
    static const GTypeInfo ags_drum_info = {
      sizeof(AgsDrumClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_drum_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(AgsDrum),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_drum_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_drum_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };
    
    ags_type_drum = g_type_register_static(AGS_TYPE_MACHINE,
					    "AgsDrum\0", &ags_drum_info,
					    0);
    
    g_type_add_interface_static(ags_type_drum,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
  }

  return(ags_type_drum);
}

void
ags_drum_class_init(AgsDrumClass *drum)
{
  AgsMachineClass *machine = (AgsMachineClass *) drum;

  //  machine->read_file = ags_file_read_drum;
  //  machine->write_file = ags_file_write_drum;
}

void
ags_drum_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_drum_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->connect = ags_drum_connect;
}

void
ags_drum_init(AgsDrum *drum)
{
  AgsChannel *template;
  AgsDelay *delay;
  GtkVBox *vbox;
  GtkHBox *hbox;
  GtkToggleButton *toggle_button;
  GList *list;
  GtkFrame *frame;
  GtkTable *table0, *table1;
  GtkRadioButton *radio_button;
  guint stream_length;
  int i, j;

  g_signal_connect_after((GObject *) drum, "parent_set\0",
			 G_CALLBACK(ags_drum_parent_set_callback), (gpointer) drum);


  drum->machine.audio->flags |= (AGS_AUDIO_OUTPUT_HAS_RECYCLING |
				 AGS_AUDIO_INPUT_HAS_RECYCLING |
				 AGS_AUDIO_INPUT_TAKES_FILE |
				 AGS_AUDIO_SYNC |
				 AGS_AUDIO_ASYNC);

  drum->flags = 0;

  /* create AgsDelaySharedAudio */
  drum->delay_shared_audio = ags_delay_shared_audio_new(0);

  /* create AgsDelay in audio->play */
  delay = ags_delay_new(drum->delay_shared_audio);
  delay->recall.flags |= AGS_RECALL_TEMPLATE;

  //  ags_delay_connect(delay);
  g_signal_connect((GObject *) delay, "done\0",
		   G_CALLBACK(ags_drum_run_delay_done), drum);

  drum->machine.audio->play = g_list_prepend(drum->machine.audio->play, (gpointer) delay);

  /* create AgsDelay in audio->recall */
  delay = ags_delay_new(drum->delay_shared_audio);
  delay->recall.flags = AGS_RECALL_TEMPLATE;

  //  ags_delay_connect(delay);
  g_signal_connect((GObject *) delay, "done\0",
		   G_CALLBACK(ags_drum_run_delay_done), drum);

  drum->machine.audio->recall = g_list_prepend(drum->machine.audio->recall, (gpointer) delay);

  /* create AgsCopyPatternSharedData */
  drum->copy_pattern_shared_audio = ags_copy_pattern_shared_audio_new(NULL,
								      0, 0,
								      16, FALSE,
								      0);

  drum->copy_pattern_shared_audio_run = ags_copy_pattern_shared_audio_run_new(drum->copy_pattern_shared_audio,
									      delay, 0);

  /* create widgets */
  drum->vbox = (GtkVBox *) gtk_vbox_new(FALSE, 0);
  gtk_container_add((GtkContainer*) (gtk_container_get_children((GtkContainer *) drum))->data, (GtkWidget *) drum->vbox);

  hbox = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start((GtkBox *) drum->vbox, (GtkWidget *) hbox, FALSE, FALSE, 0);

  drum->input_pad = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  drum->machine.input = (GtkContainer *) drum->input_pad;
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) drum->input_pad, FALSE, FALSE, 0);

  drum->output_pad = (GtkVBox *) gtk_vbox_new(FALSE, 0);
  drum->machine.output = (GtkContainer *) drum->output_pad;
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) drum->output_pad, FALSE, FALSE, 0);

  drum->selected_pad = NULL;
  drum->selected_edit_button = NULL;

  hbox = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start((GtkBox *) drum->vbox, (GtkWidget *) hbox, FALSE, FALSE, 0);

  frame = (GtkFrame *) gtk_frame_new("kit\0");
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) frame, FALSE, FALSE, 0);

  vbox = (GtkVBox *) gtk_vbox_new(FALSE, 0);
  gtk_container_add((GtkContainer*) frame, (GtkWidget *) vbox);

  gtk_box_pack_start((GtkBox *) vbox,
		     (GtkWidget *) gtk_label_new("default\0"), 
		     FALSE, FALSE, 0);
 
  gtk_box_pack_start((GtkBox *) vbox,
		     (GtkWidget *) (drum->open = (GtkButton *) gtk_button_new_from_stock(GTK_STOCK_OPEN)),
		     FALSE, FALSE, 0);

  frame = (GtkFrame *) gtk_frame_new("Pattern\0");
  gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) frame, FALSE, FALSE, 0);

  table0 = (GtkTable *) gtk_table_new(16, 3, FALSE);
  gtk_container_add((GtkContainer*) frame, (GtkWidget *) table0);

  drum->loop_button = (GtkCheckButton *) gtk_check_button_new_with_label(g_strdup("loop\0"));
  gtk_table_attach_defaults(table0,
			    (GtkWidget *) drum->loop_button,
			    0, 1, 1, 2);

  drum->run = (GtkToggleButton *) gtk_toggle_button_new_with_label(g_strdup("run\0"));
  gtk_table_attach_defaults(table0,
			    (GtkWidget *) drum->run,
			    1, 2, 1, 2);

  table1 = (GtkTable *) gtk_table_new(3, 5, TRUE);
  gtk_table_attach_defaults(table0,
			    (GtkWidget *) table1,
			    2, 3, 0, 2);

  drum->selected1 = NULL;

  for(i = 0; i < 3; i++)
    for(j = 0; j < 4; j++){
      gtk_table_attach_defaults(table1,
				(GtkWidget *) (drum->index1[i * 4 + j] = (GtkToggleButton *) gtk_toggle_button_new()),
				j, j +1, i, i +1);
      g_object_set_data((GObject *) drum->index1[i * 4 + j], AGS_DRUM_INDEX, GUINT_TO_POINTER(i * 4 + j));
    }

  drum->selected1 = drum->index1[0];
  gtk_toggle_button_set_active(drum->index1[0], TRUE);

  drum->selected0 = NULL;

  for(j = 0; j < 4; j++){
    gtk_table_attach_defaults(table1,
			      (GtkWidget *) (drum->index0[j] = (GtkToggleButton *) gtk_toggle_button_new()),
			      j, j +1, 4, 5);
    g_object_set_data((GObject *) drum->index0[j], AGS_DRUM_INDEX, GUINT_TO_POINTER(j));
  }

  drum->selected0 = drum->index0[0];
  gtk_toggle_button_set_active(drum->index0[0], TRUE);

  drum->tic = (GtkOptionMenu *) gtk_option_menu_new();
  gtk_table_attach(table0,
		   (GtkWidget *) drum->tic,
		   5, 6, 0, 1,
		   GTK_EXPAND, GTK_EXPAND,
		   0, 0);

  gtk_option_menu_set_menu(drum->tic, (GtkWidget *) ags_tic_menu_new());
  gtk_option_menu_set_history(drum->tic, 6);

  hbox = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  gtk_table_attach(table0,
		   (GtkWidget *) hbox,
		   6, 7, 0, 1,
		   GTK_EXPAND, GTK_EXPAND,
		   0, 0);
  gtk_box_pack_start((GtkBox*) hbox, gtk_label_new(g_strdup("length\0")), FALSE, FALSE, 0);
  drum->length_spin = (GtkSpinButton *) gtk_spin_button_new_with_range(1.0, 64.0, 1.0);
  drum->length_spin->adjustment->value = 16.0;
  gtk_box_pack_start((GtkBox*) hbox, (GtkWidget *) drum->length_spin, FALSE, FALSE, 0);

  drum->bpm_handler = 0;

  drum->pattern =
    hbox = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  gtk_table_attach_defaults(table0, (GtkWidget *) hbox, 3, 15, 1, 2);

  for(i = 0; i < 16; i++){
    toggle_button = (GtkToggleButton *) gtk_toggle_button_new();
    gtk_widget_set_size_request((GtkWidget *) toggle_button, 24, 24);
    gtk_box_pack_start((GtkBox *) hbox, (GtkWidget *) toggle_button, FALSE, FALSE, 0);
  }

  drum->offset = (GtkVBox*) gtk_vbox_new(FALSE, 0);
  gtk_table_attach_defaults(table0, (GtkWidget *) drum->offset, 15, 16, 0, 3);

  radio_button = (GtkRadioButton *) gtk_radio_button_new_with_label(NULL, g_strdup("1-16\0"));
  gtk_box_pack_start((GtkBox*) drum->offset, (GtkWidget *) radio_button, FALSE, FALSE, 0);

  gtk_box_pack_start((GtkBox*) drum->offset,
		     (GtkWidget *) gtk_radio_button_new_with_label(radio_button->group, g_strdup("17-32\0")),
		     FALSE, FALSE, 0);

  gtk_box_pack_start((GtkBox*) drum->offset,
		     (GtkWidget *) gtk_radio_button_new_with_label(radio_button->group, g_strdup("33-48\0")),
		     FALSE, FALSE, 0);

  gtk_box_pack_start((GtkBox*) drum->offset,
		     (GtkWidget *) gtk_radio_button_new_with_label(radio_button->group, g_strdup("49-64\0")),
		     FALSE, FALSE, 0);
}

void
ags_drum_connect(AgsConnectable *connectable)
{
  AgsWindow *window;
  AgsDrum *drum;
  GList *list;
  int i;

  ags_drum_parent_connectable_interface->connect(connectable);

  /* AgsDrum */
  drum = AGS_DRUM(connectable);

  g_signal_connect((GObject *) drum, "destroy\0",
		   G_CALLBACK(ags_drum_destroy_callback), (gpointer) drum);

  g_signal_connect((GObject *) drum, "show\0",
		   G_CALLBACK(ags_drum_show_callback), (gpointer) drum);


  g_signal_connect((GObject *) drum->open, "clicked\0",
		   G_CALLBACK(ags_drum_open_callback), (gpointer) drum);

  g_signal_connect((GObject *) drum->loop_button, "clicked\0",
		   G_CALLBACK(ags_drum_loop_button_callback), (gpointer) drum);

  g_signal_connect((GObject *) drum->run, "clicked\0",
		   G_CALLBACK(ags_drum_run_callback), (gpointer) drum);

  g_signal_connect((GObject *) drum->tic, "changed\0",
		   G_CALLBACK(ags_drum_tic_callback), (gpointer) drum);

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) drum);
  drum->bpm_handler = g_signal_connect((GObject *) window->navigation->bpm, "value-changed\0",
				       G_CALLBACK(ags_drum_bpm_callback), (gpointer) drum);

  g_signal_connect_after((GObject *) drum->length_spin, "value-changed\0",
			 G_CALLBACK(ags_drum_length_spin_callback), (gpointer) drum);

  for(i = 0; i < 12; i++){
    g_signal_connect(G_OBJECT(drum->index1[i]), "clicked\0",
		     G_CALLBACK(ags_drum_index1_callback), (gpointer) drum);
  }

  for(i = 0; i < 4; i++){
    g_signal_connect(G_OBJECT(drum->index0[i]), "clicked\0",
		     G_CALLBACK(ags_drum_index0_callback), (gpointer) drum);
  }

  list = gtk_container_get_children((GtkContainer *) drum->pattern);

  while(list != NULL){
    g_signal_connect(G_OBJECT(list->data), "clicked\0",
		     G_CALLBACK(ags_drum_pad_callback), (gpointer) drum);

    list = list->next;
  }

  list = gtk_container_get_children((GtkContainer *) drum->offset);

  while(list != NULL){
    g_signal_connect(G_OBJECT(list->data), "clicked\0",
		     G_CALLBACK(ags_drum_offset_callback), (gpointer) drum);
		   
    list = list->next;
  }

  /* AgsAudio */
  g_signal_connect_after(G_OBJECT(drum->machine.audio), "set_audio_channels\0",
			 G_CALLBACK(ags_drum_set_audio_channels), NULL);

  g_signal_connect_after(G_OBJECT(drum->machine.audio), "set_pads\0",
			 G_CALLBACK(ags_drum_set_pads), NULL);
}

void
ags_drum_destroy(GtkObject *object)
{
  AgsWindow *window;
  AgsDrum *drum;

  drum = AGS_DRUM(object);

  window = AGS_WINDOW(gtk_widget_get_ancestor((GtkWidget *) drum, AGS_TYPE_WINDOW));
  g_signal_handler_disconnect((GObject *) window->navigation->bpm, drum->bpm_handler);
}

void
ags_drum_show(GtkWidget *widget)
{
}

void
ags_drum_set_audio_channels(AgsAudio *audio,
			    guint audio_channels, guint audio_channels_old,
			    gpointer data)
{
  AgsDrum *drum;
  GList *list_output_pad, *list_output_pad_next, *list_input_pad, *list_input_pad_next;
  guint i, j;

  drum = (AgsDrum *) audio->machine;

  if(audio_channels > audio_channels_old){
    AgsDrumInputPad *drum_input_pad;
    AgsDrumOutputPad *drum_output_pad;
    AgsChannel *channel;
    GList *list_pad;

    /* AgsInput */
    channel = audio->input;
    list_input_pad = gtk_container_get_children((GtkContainer *) drum->input_pad);

    for(i = 0; i < audio->input_pads; i++){
      if(audio_channels_old == 0){
      /* create AgsPad's if necessary */
	drum_input_pad = ags_drum_input_pad_new(channel);
	gtk_box_pack_start((GtkBox *) drum->input_pad,
			   (GtkWidget *) drum_input_pad,
			   FALSE, FALSE,
			   0);
	ags_pad_resize_lines((AgsPad *) drum_input_pad, AGS_TYPE_DRUM_INPUT_LINE,
			     AGS_AUDIO(channel->audio)->audio_channels, 0);
	gtk_option_menu_set_history(drum_input_pad->pad.option, 0);

	if(GTK_WIDGET_VISIBLE((GtkWidget *) drum)){
	  ags_connectable_connect(AGS_CONNECTABLE(drum_input_pad));
	  gtk_widget_show_all((GtkWidget *) drum_input_pad);
	}
      }else{
	drum_input_pad = AGS_DRUM_INPUT_PAD(list_input_pad->data);

	ags_pad_resize_lines((AgsPad *) drum_input_pad, AGS_TYPE_DRUM_INPUT_LINE,
			     audio_channels, audio_channels_old);
      }

      channel = channel->next_pad;

      if(audio_channels_old != 0)
	list_input_pad = list_input_pad->next;
    }

    /* AgsOutput */
    channel = audio->output;
    list_output_pad = gtk_container_get_children((GtkContainer *) drum->output_pad);

    for(i = 0; i < audio->output_pads; i++){
      if(audio_channels_old == 0){
	/* create AgsPad's if necessary */
	drum_output_pad = ags_drum_output_pad_new(channel);
	gtk_box_pack_start((GtkBox *) drum->output_pad,
			   (GtkWidget *) drum_output_pad,
			   FALSE, FALSE,
			   0);
	ags_pad_resize_lines((AgsPad *) drum_output_pad, AGS_TYPE_DRUM_OUTPUT_LINE,
			     AGS_AUDIO(channel->audio)->audio_channels, 0);
	gtk_option_menu_set_history(drum_output_pad->pad.option, 0);

	if(GTK_WIDGET_VISIBLE((GtkWidget *) drum)){
	  ags_connectable_connect(AGS_CONNECTABLE(drum_input_pad));
	  //	  ags_drum_output_pad_connect(drum_output_pad);
	  gtk_widget_show_all((GtkWidget *) drum_output_pad);
	}
      }else{
	drum_output_pad = AGS_DRUM_OUTPUT_PAD(list_output_pad->data);

	ags_pad_resize_lines((AgsPad *) drum_output_pad, AGS_TYPE_DRUM_INPUT_LINE,
			     audio_channels, audio_channels_old);
      }

      channel = channel->next_pad;

      if(audio_channels_old != 0)
	list_output_pad = list_output_pad->next;
    }
  }else if(audio_channels < audio_channels_old){
    GList *list_line, *list_line_next;

    list_output_pad = gtk_container_get_children((GtkContainer *) drum->output_pad);
    list_input_pad = gtk_container_get_children((GtkContainer *) drum->input_pad);

    if(audio_channels == 0){
      /* AgsInput */
      while(list_input_pad != NULL){
	list_input_pad_next = list_input_pad->next;

	gtk_widget_destroy(GTK_WIDGET(list_input_pad->data));

	list_input_pad->next = list_input_pad_next;
      }

      /* AgsOutput */
      while(list_output_pad != NULL){
	list_output_pad_next = list_output_pad->next;

	gtk_widget_destroy(GTK_WIDGET(list_output_pad->data));

	list_output_pad->next = list_output_pad_next;
      }
    }else{
      /* AgsInput */
      for(i = 0; list_input_pad != NULL; i++){
	list_line = g_list_nth(gtk_container_get_children((GtkContainer *) gtk_option_menu_get_menu(AGS_PAD(list_input_pad->data)->option)), audio_channels);

	for(j = audio_channels; j < audio_channels_old; j++){
	  list_line_next = list_line->next;

	  gtk_widget_destroy((GtkWidget *) list_line->data);

	  list_line = list_line_next;
	}

	list_input_pad = list_input_pad->next;
      }

      /* AgsOutput */
      for(i = 0; list_output_pad != NULL; i++){
	list_line = g_list_nth(gtk_container_get_children((GtkContainer *) gtk_option_menu_get_menu(AGS_PAD(list_output_pad->data)->option)), audio_channels);

	for(j = audio_channels; j < audio_channels_old; j++){
	  list_line_next = list_line->next;

	  gtk_widget_destroy((GtkWidget *) list_line->data);

	  list_line = list_line_next;
	}

	list_output_pad = list_output_pad->next;
      }
    }
  }
}

void
ags_drum_set_pads(AgsAudio *audio, GType type,
		  guint pads, guint pads_old,
		  gpointer data)
{
  AgsDrum *drum;
  AgsChannel *channel;
  GList *list, *list_next;
  guint i, j;

  drum = (AgsDrum *) audio->machine;

  if(type == AGS_TYPE_INPUT){
    AgsDrumInputPad *drum_input_pad;

    if(pads_old < pads){
      channel = ags_channel_nth(audio->input, pads_old * audio->audio_channels);

      for(i = pads_old; i < pads; i++){
	drum_input_pad = ags_drum_input_pad_new(channel);
	gtk_box_pack_start((GtkBox *) drum->input_pad,
			   (GtkWidget *) drum_input_pad, FALSE, FALSE, 0);
	ags_pad_resize_lines((AgsPad *) drum_input_pad, AGS_TYPE_DRUM_INPUT_LINE,
			     AGS_AUDIO(channel->audio)->audio_channels, 0);
	gtk_option_menu_set_history(drum_input_pad->pad.option, 0);

	if(GTK_WIDGET_VISIBLE((GtkWidget *) drum)){
	  ags_connectable_connect(AGS_CONNECTABLE(drum_input_pad));
	  gtk_widget_show_all((GtkWidget *) drum_input_pad);
	}

	channel = channel->next_pad;
      }

      if(pads_old == 0){
	GtkToggleButton *selected_edit_button;

	drum->selected_pad = AGS_DRUM_INPUT_PAD(gtk_container_get_children((GtkContainer *) drum->input_pad)->data);
	drum->selected_edit_button = drum->selected_pad->edit;
	gtk_toggle_button_set_active((GtkToggleButton *) drum->selected_edit_button, TRUE);
      }
    }else{
      /* destroy AgsPad's */
      if(pads == 0){
	drum->selected_pad = NULL;
	drum->selected_edit_button = NULL;
      }else{
	drum_input_pad = AGS_DRUM_INPUT_PAD(gtk_widget_get_ancestor(GTK_WIDGET(drum->selected_edit_button), AGS_TYPE_PAD));

	if(drum_input_pad->pad.channel->pad > pads){
	  drum->selected_pad = AGS_DRUM_INPUT_PAD(gtk_container_get_children((GtkContainer *) drum->input_pad)->data);
	  drum->selected_edit_button = drum->selected_pad->edit;
	  gtk_toggle_button_set_active((GtkToggleButton *) drum->selected_edit_button, TRUE);
	}
      }

      list = gtk_container_get_children((GtkContainer *) drum->input_pad);
      list = g_list_nth(list, pads);

      while(list != NULL){
	list_next = list->next;

	gtk_widget_destroy((GtkWidget *) list->data);

	list = list_next;
      }
    }
  }else{
    AgsDrumOutputPad *drum_output_pad;

    if(pads_old < pads){
      /* create AgsDrumOutputPad */
      channel = ags_channel_nth(audio->output, pads_old * audio->audio_channels);

      for(i = pads_old; i < audio->output_pads; i++){
	drum_output_pad = ags_drum_output_pad_new(channel);
	gtk_box_pack_start((GtkBox *) drum->output_pad, (GtkWidget *) drum_output_pad, FALSE, FALSE, 0);
	ags_pad_resize_lines((AgsPad *) drum_output_pad, AGS_TYPE_DRUM_OUTPUT_LINE,
			     AGS_AUDIO(channel->audio)->audio_channels, 0);
	gtk_option_menu_set_history(drum_output_pad->pad.option, 0);

	if(GTK_WIDGET_VISIBLE((GtkWidget *) drum)){
	  ags_connectable_connect(AGS_CONNECTABLE(drum_output_pad));
	  gtk_widget_show_all((GtkWidget *) drum_output_pad);
	}

	channel = channel->next_pad;
      }

      /* update recalls in AgsDrumInputPads */
      list = gtk_container_get_children(GTK_CONTAINER(drum->input_pad));

      for(i = 0; i < audio->input_pads; i++){


	list = list->next;
      }
    }else if(pads_old > pads){
      /* destroy AgsPad's */
      list = gtk_container_get_children(GTK_CONTAINER(drum->output_pad));
      list = g_list_nth(list, pads);

      while(list != NULL){
	list_next = list->next;

	gtk_widget_destroy(GTK_WIDGET(list->data));

	list = list_next;
      }
    }
  }
  
}

void
ags_drum_set_pattern(AgsDrum *drum)
{
  GList *list;
  guint index0, index1, offset;
  guint i;

  if(drum->selected_pad == NULL || drum->selected_pad->pad.selected_line == NULL){
    printf("no selected pad or line\n\0");
    return;
  }

  for(i = 0; i < 4 && drum->selected0 != drum->index0[i]; i++);

  index0 = i;

  for(i = 0; i < 12 && drum->selected1 != drum->index1[i]; i++);
  
  index1 = i;

  list = gtk_container_get_children((GtkContainer *) drum->offset);

  for(i = 0; i < 4 && ! GTK_TOGGLE_BUTTON(list->data)->active; i++)
    list = list->next;

  offset = i * 16;

  list = gtk_container_get_children((GtkContainer *) drum->pattern);

  drum->flags |= AGS_DRUM_BLOCK_PATTERN;

  for(i = 0; i < 16; i++){
    if(ags_pattern_get_bit((AgsPattern *) drum->selected_pad->pad.selected_line->channel->pattern->data, index0, index1, offset + i))
      gtk_toggle_button_set_active((GtkToggleButton *) list->data, TRUE);
    else
      gtk_toggle_button_set_active((GtkToggleButton *) list->data, FALSE);

    list = list->next;
  }

  drum->flags &= (~AGS_DRUM_BLOCK_PATTERN);
}

AgsDrum*
ags_drum_new()
{
  AgsDrum *drum;

  drum = (AgsDrum *) g_object_new(AGS_TYPE_DRUM, NULL);

  return(drum);
}
