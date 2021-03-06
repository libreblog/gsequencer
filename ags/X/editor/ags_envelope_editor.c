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

#include <ags/X/editor/ags_envelope_editor.h>
#include <ags/X/editor/ags_envelope_editor_callbacks.h>

#include <ags/X/ags_window.h>
#include <ags/X/ags_machine.h>

#include <ags/X/editor/ags_envelope_dialog.h>

#include <math.h>
#include <complex.h>

#include <ags/i18n.h>

void ags_envelope_editor_class_init(AgsEnvelopeEditorClass *envelope_editor);
void ags_envelope_editor_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_envelope_editor_applicable_interface_init(AgsApplicableInterface *applicable);
void ags_envelope_editor_init(AgsEnvelopeEditor *envelope_editor);
void ags_envelope_editor_connect(AgsConnectable *connectable);
void ags_envelope_editor_disconnect(AgsConnectable *connectable);
void ags_envelope_editor_set_update(AgsApplicable *applicable, gboolean update);
void ags_envelope_editor_apply(AgsApplicable *applicable);
void ags_envelope_editor_reset(AgsApplicable *applicable);
gboolean ags_envelope_editor_delete_event(GtkWidget *widget, GdkEventAny *event);

gchar* ags_envelope_editor_x_label_func(gdouble value,
					gpointer data);
gchar* ags_envelope_editor_y_label_func(gdouble value,
					gpointer data);

/**
 * SECTION:ags_envelope_editor
 * @short_description: Edit envelope of notes
 * @title: AgsEnvelopeEditor
 * @section_id:
 * @include: ags/X/ags_envelope_editor.h
 *
 * #AgsEnvelopeEditor is a composite widget to edit envelope controls
 * of selected #AgsNote.
 */

static gpointer ags_envelope_editor_parent_class = NULL;

GType
ags_envelope_editor_get_type(void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if(g_once_init_enter (&g_define_type_id__volatile)){
    GType ags_type_envelope_editor = 0;

    static const GTypeInfo ags_envelope_editor_info = {
      sizeof (AgsEnvelopeEditorClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_envelope_editor_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsEnvelopeEditor),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_envelope_editor_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_envelope_editor_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_applicable_interface_info = {
      (GInterfaceInitFunc) ags_envelope_editor_applicable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_envelope_editor = g_type_register_static(GTK_TYPE_VBOX,
						      "AgsEnvelopeEditor", &ags_envelope_editor_info,
						      0);

    g_type_add_interface_static(ags_type_envelope_editor,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_envelope_editor,
				AGS_TYPE_APPLICABLE,
				&ags_applicable_interface_info);

    g_once_init_leave(&g_define_type_id__volatile, ags_type_envelope_editor);
  }

  return g_define_type_id__volatile;
}

void
ags_envelope_editor_class_init(AgsEnvelopeEditorClass *envelope_editor)
{
  ags_envelope_editor_parent_class = g_type_class_peek_parent(envelope_editor);
}

void
ags_envelope_editor_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_envelope_editor_connect;
  connectable->disconnect = ags_envelope_editor_disconnect;
}

void
ags_envelope_editor_applicable_interface_init(AgsApplicableInterface *applicable)
{
  applicable->set_update = ags_envelope_editor_set_update;
  applicable->apply = ags_envelope_editor_apply;
  applicable->reset = ags_envelope_editor_reset;
}

void
ags_envelope_editor_init(AgsEnvelopeEditor *envelope_editor)
{
  GtkFrame *frame;
  GtkHBox *hbox;
  GtkVBox *control;
  GtkTable *table;
  GtkLabel *label;
  AgsCartesian *cartesian;
  
  AgsPlot *plot;

  gdouble width, height;
  gdouble default_width, default_height;
  gdouble offset;
  
  envelope_editor->flags = 0;

  envelope_editor->version = AGS_ENVELOPE_EDITOR_DEFAULT_VERSION;
  envelope_editor->build_id = AGS_ENVELOPE_EDITOR_DEFAULT_BUILD_ID;

  /* enabled */
  envelope_editor->enabled = (GtkCheckButton *) gtk_check_button_new_with_label(i18n("enabled"));
  gtk_box_pack_start((GtkBox *) envelope_editor,
		     (GtkWidget *) envelope_editor->enabled,
		     FALSE, FALSE,
		     0);

  /* rename dialog */
  envelope_editor->rename = NULL;

  /* frame - preset */
  frame = (GtkFrame *) gtk_frame_new(i18n("preset"));
  gtk_box_pack_start((GtkBox *) envelope_editor,
		     (GtkWidget *) frame,
		     FALSE, FALSE,
		     0);

  hbox = (GtkHBox *) gtk_hbox_new(FALSE,
				  0);
  gtk_container_add((GtkContainer *) frame,
		    (GtkWidget *) hbox);
  
  envelope_editor->preset = (GtkComboBoxText *) gtk_combo_box_text_new();
  gtk_box_pack_start((GtkBox *) hbox,
		     (GtkWidget *) envelope_editor->preset,
		     FALSE, FALSE,
		     0);

  envelope_editor->add = (GtkButton *) gtk_button_new_from_stock(GTK_STOCK_ADD);
  gtk_box_pack_start((GtkBox *) hbox,
		     (GtkWidget *) envelope_editor->add,
		     FALSE, FALSE,
		     0);

  envelope_editor->remove = (GtkButton *) gtk_button_new_from_stock(GTK_STOCK_REMOVE);
  gtk_box_pack_start((GtkBox *) hbox,
		     (GtkWidget *) envelope_editor->remove,
		     FALSE, FALSE,
		     0);

  /* cartesian */
  cartesian = 
    envelope_editor->cartesian = ags_cartesian_new();

  cartesian->x_label_func = ags_envelope_editor_x_label_func;
  cartesian->y_label_func = ags_envelope_editor_y_label_func;

  ags_cartesian_fill_label(cartesian,
			   TRUE);
  ags_cartesian_fill_label(cartesian,
			   FALSE);  

  /* cartesian - plot */
  plot = ags_plot_alloc(5, 0, 0);
  plot->join_points = TRUE;

  plot->point_color[0][0] = 0.125;
  plot->point_color[0][1] = 0.5;
  plot->point_color[0][2] = 1.0;

  plot->point_color[1][0] = 0.125;
  plot->point_color[1][1] = 0.5;
  plot->point_color[1][2] = 1.0;

  plot->point_color[2][0] = 0.125;
  plot->point_color[2][1] = 0.5;
  plot->point_color[2][2] = 1.0;

  plot->point_color[3][0] = 0.125;
  plot->point_color[3][1] = 0.5;
  plot->point_color[3][2] = 1.0;

  plot->point_color[4][0] = 0.125;
  plot->point_color[4][1] = 0.5;
  plot->point_color[4][2] = 1.0;

  width = cartesian->x_end - cartesian->x_start;
  height = cartesian->y_end - cartesian->y_start;
  
  default_width = cartesian->x_step_width * cartesian->x_scale_step_width;
  default_height = cartesian->y_step_height * cartesian->y_scale_step_height;

  plot->point[0][0] = 0.0;
  plot->point[0][1] = default_height * 1.0;

  plot->point[1][0] = default_width * 0.25;
  plot->point[1][1] = default_height * 1.0;

  offset = default_width * 0.25;
  
  plot->point[2][0] = offset + default_width * 0.25;
  plot->point[2][1] = default_height * 1.0;

  offset += default_width * 0.25;

  plot->point[3][0] = offset + default_width * 0.25;
  plot->point[3][1] = default_height * 1.0;

  offset += default_width * 0.25;

  plot->point[4][0] = offset + default_width * 0.25;
  plot->point[4][1] = default_height * 1.0;

  ags_cartesian_add_plot(cartesian,
			 plot);

  /* cartesian - size, pack and redraw */
  gtk_widget_set_size_request((GtkWidget *) cartesian,
			      (gint) width + 2.0 * cartesian->x_margin, (gint) height + 2.0 * cartesian->y_margin);
  gtk_box_pack_start((GtkBox *) envelope_editor,
		     GTK_WIDGET(cartesian),
		     FALSE, FALSE,
		     0);

  gtk_widget_queue_draw((GtkWidget *) cartesian);

  /* table */
  table = (GtkTable *) gtk_table_new(5, 2,
				     FALSE);
  gtk_box_pack_start((GtkBox *) envelope_editor,
		     GTK_WIDGET(table),
		     FALSE, FALSE,
		     0);

  /* attack */
  label = (GtkLabel *) g_object_new(GTK_TYPE_LABEL,
				    "label", i18n("attack"),
				    "xalign", 0.0,
				    "yalign", 1.0,
				    NULL);
  gtk_table_attach(table,
		   GTK_WIDGET(label),
		   0, 1,
		   0, 1,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  control = (GtkVBox *) gtk_vbox_new(FALSE,
				     0);
  gtk_table_attach(table,
		   GTK_WIDGET(control),
		   1, 2,
		   0, 1,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
		   0, 0);
  
  envelope_editor->attack_x = (GtkHScale *) gtk_hscale_new_with_range(0.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->attack_x,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->attack_x,
		      0.25);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->attack_x,
		     FALSE, FALSE,
		     0);

  envelope_editor->attack_y = (GtkHScale *) gtk_hscale_new_with_range(-1.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->attack_y,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->attack_y,
		      0.0);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->attack_y,
		     FALSE, FALSE,
		     0);

  /* decay */
  label = (GtkLabel *) g_object_new(GTK_TYPE_LABEL,
				    "label", i18n("decay"),
				    "xalign", 0.0,
				    "yalign", 1.0,
				    NULL);
  gtk_table_attach(table,
		   GTK_WIDGET(label),
		   0, 1,
		   1, 2,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  control = (GtkVBox *) gtk_vbox_new(FALSE,
				     0);
  gtk_table_attach(table,
		   GTK_WIDGET(control),
		   1, 2,
		   1, 2,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
		   0, 0);

  envelope_editor->decay_x = (GtkHScale *) gtk_hscale_new_with_range(0.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->decay_x,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->decay_x,
		      0.25);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->decay_x,
		     FALSE, FALSE,
		     0);

  envelope_editor->decay_y = (GtkHScale *) gtk_hscale_new_with_range(-1.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->decay_y,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->decay_y,
		      0.0);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->decay_y,
		     FALSE, FALSE,
		     0);

  /* sustain */
  label = (GtkLabel *) g_object_new(GTK_TYPE_LABEL,
				    "label", i18n("sustain"),
				    "xalign", 0.0,
				    "yalign", 1.0,
				    NULL);
  gtk_table_attach(table,
		   GTK_WIDGET(label),
		   0, 1,
		   2, 3,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  control = (GtkVBox *) gtk_vbox_new(FALSE,
				     0);
  gtk_table_attach(table,
		   GTK_WIDGET(control),
		   1, 2,
		   2, 3,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
		   0, 0);

  envelope_editor->sustain_x = (GtkHScale *) gtk_hscale_new_with_range(0.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->sustain_x,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->sustain_x,
		      0.25);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->sustain_x,
		     FALSE, FALSE,
		     0);

  envelope_editor->sustain_y = (GtkHScale *) gtk_hscale_new_with_range(-1.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->sustain_y,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->sustain_y,
		      0.0);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->sustain_y,
		     FALSE, FALSE,
		     0);

  /* release */
  label = (GtkLabel *) g_object_new(GTK_TYPE_LABEL,
				    "label", i18n("release"),
				    "xalign", 0.0,
				    "yalign", 1.0,
				    NULL);
  gtk_table_attach(table,
		   GTK_WIDGET(label),
		   0, 1,
		   3, 4,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  control = (GtkVBox *) gtk_vbox_new(FALSE,
				     0);
  gtk_table_attach(table,
		   GTK_WIDGET(control),
		   1, 2,
		   3, 4,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
		   0, 0);

  envelope_editor->release_x = (GtkHScale *) gtk_hscale_new_with_range(0.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->release_x,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->release_x,
		      0.25);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->release_x,
		     FALSE, FALSE,
		     0);

  envelope_editor->release_y = (GtkHScale *) gtk_hscale_new_with_range(-1.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->release_y,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->release_y,
		      0.0);
  gtk_box_pack_start((GtkBox *) control,
		     (GtkWidget *) envelope_editor->release_y,
		     FALSE, FALSE,
		     0);

  /* ratio */
  label = (GtkLabel *) g_object_new(GTK_TYPE_LABEL,
				    "label", i18n("ratio"),
				    "xalign", 0.0,
				    "yalign", 1.0,
				    NULL);
  gtk_table_attach(table,
		   GTK_WIDGET(label),
		   0, 1,
		   4, 5,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  envelope_editor->ratio = (GtkHScale *) gtk_hscale_new_with_range(0.0, 1.0, 0.001);
  gtk_scale_set_draw_value((GtkScale *) envelope_editor->ratio,
			   TRUE);
  gtk_range_set_value((GtkRange *) envelope_editor->ratio,
		      1.0);
  gtk_table_attach(table,
		   GTK_WIDGET(envelope_editor->ratio),
		   1, 2,
		   4, 5,
		   GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
		   0, 0);
}

void
ags_envelope_editor_connect(AgsConnectable *connectable)
{
  AgsEnvelopeEditor *envelope_editor;

  envelope_editor = AGS_ENVELOPE_EDITOR(connectable);

  if((AGS_ENVELOPE_EDITOR_CONNECTED & (envelope_editor->flags)) != 0){
    return;
  }

  envelope_editor->flags |= AGS_ENVELOPE_EDITOR_CONNECTED;

  /* preset */
  g_signal_connect((GObject *) envelope_editor->preset, "changed",
		   G_CALLBACK(ags_envelope_editor_preset_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->add, "clicked",
		   G_CALLBACK(ags_envelope_editor_preset_add_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->remove, "clicked",
		   G_CALLBACK(ags_envelope_editor_preset_remove_callback), (gpointer) envelope_editor);

  /* attack x,y */
  g_signal_connect((GObject *) envelope_editor->attack_x, "value-changed",
		   G_CALLBACK(ags_envelope_editor_attack_x_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->attack_y, "value-changed",
		   G_CALLBACK(ags_envelope_editor_attack_y_callback), (gpointer) envelope_editor);

  /* decay x,y */
  g_signal_connect((GObject *) envelope_editor->decay_x, "value-changed",
		   G_CALLBACK(ags_envelope_editor_decay_x_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->decay_y, "value-changed",
		   G_CALLBACK(ags_envelope_editor_decay_y_callback), (gpointer) envelope_editor);

  /* sustain x,y */
  g_signal_connect((GObject *) envelope_editor->sustain_x, "value-changed",
		   G_CALLBACK(ags_envelope_editor_sustain_x_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->sustain_y, "value-changed",
		   G_CALLBACK(ags_envelope_editor_sustain_y_callback), (gpointer) envelope_editor);

  /* release x,y */
  g_signal_connect((GObject *) envelope_editor->release_x, "value-changed",
		   G_CALLBACK(ags_envelope_editor_release_x_callback), (gpointer) envelope_editor);

  g_signal_connect((GObject *) envelope_editor->release_y, "value-changed",
		   G_CALLBACK(ags_envelope_editor_release_y_callback), (gpointer) envelope_editor);

  /* ratio */
  g_signal_connect((GObject *) envelope_editor->ratio, "value-changed",
		   G_CALLBACK(ags_envelope_editor_ratio_callback), (gpointer) envelope_editor);
}

void
ags_envelope_editor_disconnect(AgsConnectable *connectable)
{
  AgsEnvelopeEditor *envelope_editor;

  envelope_editor = AGS_ENVELOPE_EDITOR(connectable);

  if((AGS_ENVELOPE_EDITOR_CONNECTED & (envelope_editor->flags)) == 0){
    return;
  }

  envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_CONNECTED);

  /* preset */
  g_object_disconnect((GObject *) envelope_editor->preset,
		      "any_signal::changed",
		      G_CALLBACK(ags_envelope_editor_preset_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->add,
		      "any_signal::clicked",
		      G_CALLBACK(ags_envelope_editor_preset_add_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->remove,
		      "any_signal::clicked",
		      G_CALLBACK(ags_envelope_editor_preset_remove_callback),
		      (gpointer) envelope_editor,
		      NULL);

  /* attack x,y */
  g_object_disconnect((GObject *) envelope_editor->attack_x,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_attack_x_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->attack_y,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_attack_y_callback),
		      (gpointer) envelope_editor,
		      NULL);

  /* decay x,y */
  g_object_disconnect((GObject *) envelope_editor->decay_x,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_decay_x_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->decay_y,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_decay_y_callback),
		      (gpointer) envelope_editor,
		      NULL);

  /* sustain x,y */
  g_object_disconnect((GObject *) envelope_editor->sustain_x,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_sustain_x_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->sustain_y,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_sustain_y_callback),
		      (gpointer) envelope_editor,
		      NULL);

  /* release x,y */
  g_object_disconnect((GObject *) envelope_editor->release_x,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_release_x_callback),
		      (gpointer) envelope_editor,
		      NULL);

  g_object_disconnect((GObject *) envelope_editor->release_y,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_release_y_callback),
		      (gpointer) envelope_editor,
		      NULL);

  /* ratio */
  g_object_disconnect((GObject *) envelope_editor->ratio,
		      "any_signal::value-changed",
		      G_CALLBACK(ags_envelope_editor_ratio_callback),
		      (gpointer) envelope_editor,
		      NULL);
}

void
ags_envelope_editor_set_update(AgsApplicable *applicable, gboolean update)
{
  /* empty */
}

void
ags_envelope_editor_apply(AgsApplicable *applicable)
{
  AgsEnvelopeDialog *envelope_dialog;
  AgsEnvelopeEditor *envelope_editor;
  
  AgsMachine *machine;

  AgsAudio *audio;
  
  GList *start_notation, *notation;
  GList *start_selection, *selection;

  double attack_x, attack_y;
  double decay_x, decay_y;
  double sustain_x, sustain_y;
  double release_x, release_y;
  double ratio;
  
  complex z;
  
  envelope_editor = AGS_ENVELOPE_EDITOR(applicable);
  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  machine = envelope_dialog->machine;

  audio = machine->audio;

  /* get z */
  attack_x = gtk_range_get_value(GTK_RANGE(envelope_editor->attack_x));
  attack_y = gtk_range_get_value(GTK_RANGE(envelope_editor->attack_y));

  decay_x = gtk_range_get_value(GTK_RANGE(envelope_editor->decay_x));
  decay_y = gtk_range_get_value(GTK_RANGE(envelope_editor->decay_y));

  sustain_x = gtk_range_get_value(GTK_RANGE(envelope_editor->sustain_x));
  sustain_y = gtk_range_get_value(GTK_RANGE(envelope_editor->sustain_y));

  release_x = gtk_range_get_value(GTK_RANGE(envelope_editor->release_x));
  release_y = gtk_range_get_value(GTK_RANGE(envelope_editor->release_y));

  ratio = gtk_range_get_value(GTK_RANGE(envelope_editor->ratio));

  /* notation */
  g_object_get(audio,
	       "notation", &start_notation,
	       NULL);

  notation = start_notation;

  /* set attack, decay, sustain and release */
  while(notation != NULL){
    GRecMutex *notation_mutex;

    /* get notation mutex */
    notation_mutex = AGS_NOTATION_GET_OBJ_MUTEX(notation->data);

    /**/
    g_rec_mutex_lock(notation_mutex);

    selection =
      start_selection = g_list_copy_deep(AGS_NOTATION(notation->data)->selection,
					 (GCopyFunc) g_object_ref,
					 NULL);

    g_rec_mutex_unlock(notation_mutex);

    while(selection != NULL){
      AgsNote *current_note;

      GRecMutex *note_mutex;
      
      current_note = AGS_NOTE(selection->data);

      /* get note mutex */
      note_mutex = AGS_NOTE_GET_OBJ_MUTEX(current_note);

      /* apply */
      g_rec_mutex_lock(note_mutex);

      current_note->flags |= AGS_NOTE_ENVELOPE;
      
      z = attack_x + I * attack_y;
      ags_complex_set(&(current_note->attack),
		      z);

      z = decay_x + I * decay_y;
      ags_complex_set(&(current_note->decay),
		      z);

      z = sustain_x + I * sustain_y;
      ags_complex_set(&(current_note->sustain),
		      z);

      z = release_x + I * release_y;
      ags_complex_set(&(current_note->release),
		      z);

      z = 0.0 + I * ratio;
      ags_complex_set(&(current_note->ratio),
		      z);
      
      g_rec_mutex_unlock(note_mutex);

      /* iterate */
      selection = selection->next;
    }

    g_list_free_full(start_selection,
		     g_object_unref);
    
    notation = notation->next;
  }

  g_list_free_full(start_notation,
		   g_object_unref);
}

void
ags_envelope_editor_reset(AgsApplicable *applicable)
{
  AgsEnvelopeEditor *envelope_editor;

  envelope_editor = AGS_ENVELOPE_EDITOR(applicable);
  
  ags_envelope_editor_load_preset(envelope_editor);
}

gchar*
ags_envelope_editor_x_label_func(gdouble value,
				 gpointer data)
{
  gchar *format;
  gchar *str;
  
  format = g_strdup_printf("%%.%df",
			   (guint) ceil(AGS_CARTESIAN(data)->y_label_precision));

  str = g_strdup_printf(format,
			value / 10.0);
  g_free(format);

  return(str);
}

gchar*
ags_envelope_editor_y_label_func(gdouble value,
				 gpointer data)
{
  gchar *format;
  gchar *str;
  
  format = g_strdup_printf("%%.%df",
			   (guint) ceil(AGS_CARTESIAN(data)->y_label_precision));

  str = g_strdup_printf(format,
			value / 10.0);
  g_free(format);

  return(str);
}

/**
 * ags_envelope_editor_get_active_preset:
 * @envelope_editor: the #AgsEnvelopeEditor
 * 
 * Get active preset.
 * 
 * Returns: the matching #AgsPreset, if none selected %NULL
 * 
 * Since: 3.0.0
 */
AgsPreset*
ags_envelope_editor_get_active_preset(AgsEnvelopeEditor *envelope_editor)
{
  AgsEnvelopeDialog *envelope_dialog;

  AgsMachine *machine;

  AgsAudio *audio;
  AgsPreset *current;

  GList *start_preset, *preset;

  gchar *preset_name;
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor)){
    return(NULL);
  }
    
  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  machine = envelope_dialog->machine;

  audio = machine->audio;

  /* preset name */
  preset_name = gtk_combo_box_text_get_active_text(envelope_editor->preset);
  
  /* find preset */
  g_object_get(audio,
	       "preset", &start_preset,
	       NULL);
  
  preset = start_preset;  
  current = NULL;
  
  preset = ags_preset_find_name(preset,
				preset_name);

  g_free(preset_name);
  
  if(preset != NULL){
    current = preset->data;
  }

  g_list_free_full(start_preset,
		   g_object_unref);
  
  return(current);
}

/**
 * ags_envelope_editor_load_preset:
 * @envelope_editor: the #AgsPatternEnvelope
 *
 * Load preset.
 * 
 * Since: 3.0.0
 */
void
ags_envelope_editor_load_preset(AgsEnvelopeEditor *envelope_editor)
{
  AgsEnvelopeDialog *envelope_dialog;

  AgsMachine *machine;

  GtkTreeModel *model;

  AgsAudio *audio;
    
  GList *start_preset, *preset;
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor)){
    return;
  }

  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  machine = envelope_dialog->machine;

  audio = machine->audio;

  /* get model */
  model = GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(envelope_editor->preset)));

  /* clear old */
  gtk_list_store_clear(GTK_LIST_STORE(model));

  /* create new */
  g_object_get(audio,
	       "preset", &start_preset,
	       NULL);
  
  preset = start_preset;

  while(preset != NULL){
    if(AGS_PRESET(preset->data)->preset_name != NULL){
      gtk_combo_box_text_append_text(envelope_editor->preset,
				     AGS_PRESET(preset->data)->preset_name);
    }

    preset = preset->next;
  }

  g_list_free_full(start_preset,
		   g_object_unref);
}

/**
 * ags_envelope_editor_add_preset:
 * @envelope_editor: the #AgsPatternEnvelope
 * @preset_name: the preset name
 *
 * Add preset.
 * 
 * Since: 3.0.0
 */
void
ags_envelope_editor_add_preset(AgsEnvelopeEditor *envelope_editor,
			       gchar *preset_name)
{
  AgsEnvelopeDialog *envelope_dialog;

  AgsMachine *machine;

  AgsAudio *audio;
  AgsPreset *preset;

  GList *start_preset;
  
  AgsComplex *val;
  
  GValue value = {0,};
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor) ||
     preset_name == NULL){
    return;
  }
  
  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  machine = envelope_dialog->machine;

  audio = machine->audio;

  g_object_get(audio,
	       "preset", &start_preset,
	       NULL);
  
  /* check if already present */
  if(ags_preset_find_name(start_preset,
			  preset_name) != NULL){    
    g_list_free_full(start_preset,
		     g_object_unref);
    
    return;
  }

  /* create preset */
  preset = g_object_new(AGS_TYPE_PRESET,
			"scope", "ags-envelope",
			"preset-name", preset_name,
			NULL);
  ags_audio_add_preset(audio,
		       (GObject *) preset);

  /* preset - attack */
  val = ags_complex_alloc();
  ags_complex_set(val,
		  0.25 + I * 0.0);
  
  g_value_init(&value,
	       AGS_TYPE_COMPLEX);
  g_value_set_boxed(&value,
		    val);

  ags_preset_add_parameter(preset,
			   "attack", &value);

  /* preset - decay */
  val = ags_complex_alloc();
  ags_complex_set(val,
		  0.25 + I * 0.0);
  
  g_value_reset(&value);
  g_value_set_boxed(&value,
		    val);

  ags_preset_add_parameter(preset,
			   "decay", &value);

  /* preset - sustain */
  val = ags_complex_alloc();
  ags_complex_set(val,
		  0.25 + I * 0.0);
  
  g_value_reset(&value);
  g_value_set_boxed(&value,
		    val);

  ags_preset_add_parameter(preset,
			   "sustain", &value);

  /* preset - release */
  val = ags_complex_alloc();
  ags_complex_set(val,
		  0.25 + I * 0.0);
  
  g_value_reset(&value);
  g_value_set_boxed(&value,
		    val);

  ags_preset_add_parameter(preset,
			   "release", &value);

  /* preset - ratio */
  val = ags_complex_alloc();
  ags_complex_set(val,
		  0.0 + I * 1.0);
  
  g_value_reset(&value);
  g_value_set_boxed(&value,
		    val);

  ags_preset_add_parameter(preset,
			   "ratio", &value);

  g_list_free(start_preset);
}

/**
 * ags_envelope_editor_remove_preset:
 * @envelope_editor: the #AgsPatternEnvelope
 * @nth: the nth preset to remove
 *
 * Remove preset.
 * 
 * Since: 3.0.0
 */
void
ags_envelope_editor_remove_preset(AgsEnvelopeEditor *envelope_editor,
				  guint nth)
{
  AgsEnvelopeDialog *envelope_dialog;

  AgsMachine *machine;

  AgsAudio *audio;
  AgsPreset *preset;
  
  GList *start_preset;
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor)){
    return;
  }
  
  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  machine = envelope_dialog->machine;

  audio = machine->audio;
  
  /* remove preset */
  g_object_get(audio,
	       "preset", &start_preset,
	       NULL);
  preset = g_list_nth_data(start_preset,
			   nth);

  g_list_free_full(start_preset,
		   g_object_unref);

  ags_audio_remove_preset(audio,
			  (GObject *) preset);
}

/**
 * ags_envelope_editor_reset_control:
 * @envelope_editor: the #AgsEnvelopeEditor
 * 
 * Reset controls.
 * 
 * Since: 3.0.0
 */
void
ags_envelope_editor_reset_control(AgsEnvelopeEditor *envelope_editor)
{
  AgsEnvelopeDialog *envelope_dialog;

  AgsPreset *preset;
  
  AgsComplex *val;
  
  complex z;
  
  GValue value = {0,};

  GError *error;
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor)){
    return;
  }

  /* disable update */
  envelope_editor->flags |= AGS_ENVELOPE_EDITOR_NO_UPDATE;
  
  /* check preset */
  preset = ags_envelope_editor_get_active_preset(envelope_editor);
  
  if(preset == NULL){
    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);
    
    return;
  }

  envelope_dialog = (AgsEnvelopeDialog *) gtk_widget_get_ancestor((GtkWidget *) envelope_editor,
								  AGS_TYPE_ENVELOPE_DIALOG);

  /* attack */
  g_value_init(&value,
	       AGS_TYPE_COMPLEX);

  error = NULL;
  ags_preset_get_parameter(preset,
			   "attack", &value,
			   &error);

  if(error != NULL){
    g_warning("%s", error->message);

    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);

    g_error_free(error);
    
    return;
  }

  val = (AgsComplex *) g_value_get_boxed(&value);  
  z = ags_complex_get(val);
  
  gtk_range_set_value((GtkRange *) envelope_editor->attack_x,
		      creal(z));
  gtk_range_set_value((GtkRange *) envelope_editor->attack_y,
		      cimag(z));

  /* decay */
  g_value_reset(&value);

  error = NULL;
  ags_preset_get_parameter(preset,
			   "decay", &value,
			   &error);

  if(error != NULL){
    g_warning("%s", error->message);

    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);

    g_error_free(error);

    return;
  }

  val = (AgsComplex *) g_value_get_boxed(&value);  
  z = ags_complex_get(val);
  
  gtk_range_set_value((GtkRange *) envelope_editor->decay_x,
		      creal(z));
  gtk_range_set_value((GtkRange *) envelope_editor->decay_y,
		      cimag(z));

  /* sustain */
  g_value_reset(&value);

  error = NULL;
  ags_preset_get_parameter(preset,
			   "sustain", &value,
			   &error);

  if(error != NULL){
    g_warning("%s", error->message);

    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);

    g_error_free(error);

    return;
  }

  val = (AgsComplex *) g_value_get_boxed(&value);  
  z = ags_complex_get(val);
  
  gtk_range_set_value((GtkRange *) envelope_editor->sustain_x,
		      creal(z));
  gtk_range_set_value((GtkRange *) envelope_editor->sustain_y,
		      cimag(z));

  /* release */
  g_value_reset(&value);

  error = NULL;
  ags_preset_get_parameter(preset,
			   "release", &value,
			   &error);

  if(error != NULL){
    g_warning("%s", error->message);

    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);

    g_error_free(error);
   
    return;
  }

  val = (AgsComplex *) g_value_get_boxed(&value);  
  z = ags_complex_get(val);
  
  gtk_range_set_value((GtkRange *) envelope_editor->release_x,
		      creal(z));
  gtk_range_set_value((GtkRange *) envelope_editor->release_y,
		      cimag(z));

  /* ratio */
  g_value_reset(&value);

  error = NULL;
  ags_preset_get_parameter(preset,
			   "ratio", &value,
			   &error);

  if(error != NULL){
    g_warning("%s", error->message);

    envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);

    g_error_free(error);

    return;
  }

  val = (AgsComplex *) g_value_get_boxed(&value);  
  z = ags_complex_get(val);
  
  gtk_range_set_value((GtkRange *) envelope_editor->ratio,
		      cimag(z));

  /* unset no update */
  envelope_editor->flags &= (~AGS_ENVELOPE_EDITOR_NO_UPDATE);
}

/**
 * ags_envelope_editor_plot:
 * @envelope_editor: the #AgsEnvelopeEditor
 * 
 * Plot envelope.
 * 
 * Since: 3.0.0
 */
void
ags_envelope_editor_plot(AgsEnvelopeEditor *envelope_editor)
{
  AgsCartesian *cartesian;
  
  AgsPlot *plot;

  gdouble default_width, default_height;
  gdouble attack_x, attack_y;
  gdouble decay_x, decay_y;
  gdouble sustain_x, sustain_y;
  gdouble release_x, release_y;
  gdouble ratio;
  gdouble offset;
  
  if(!AGS_IS_ENVELOPE_EDITOR(envelope_editor)){
    return;
  }
  
  cartesian = envelope_editor->cartesian;

  plot = cartesian->plot->data;
  
  default_width = cartesian->x_step_width * cartesian->x_scale_step_width;
  default_height = cartesian->y_step_height * cartesian->y_scale_step_height;

  attack_x = gtk_range_get_value((GtkRange *) envelope_editor->attack_x);
  attack_y = gtk_range_get_value((GtkRange *) envelope_editor->attack_y);

  decay_x = gtk_range_get_value((GtkRange *) envelope_editor->decay_x);
  decay_y = gtk_range_get_value((GtkRange *) envelope_editor->decay_y);

  sustain_x = gtk_range_get_value((GtkRange *) envelope_editor->sustain_x);
  sustain_y = gtk_range_get_value((GtkRange *) envelope_editor->sustain_y);

  release_x = gtk_range_get_value((GtkRange *) envelope_editor->release_x);
  release_y = gtk_range_get_value((GtkRange *) envelope_editor->release_y);

  ratio = gtk_range_get_value((GtkRange *) envelope_editor->ratio);

  /* reset plot points */
  plot->point[0][0] = 0.0;
  plot->point[0][1] = default_height * ratio;

  plot->point[1][0] = default_width * attack_x;
  plot->point[1][1] = default_height * (attack_y + ratio);
    
  offset = default_width * attack_x;
  
  plot->point[2][0] = offset + default_width * decay_x;
  plot->point[2][1] = default_height * (decay_y + ratio);
  
  offset += default_width * decay_x;
  
  plot->point[3][0] = offset + default_width * sustain_x;
  plot->point[3][1] = default_height * (sustain_y + ratio);
  
  offset += default_width * sustain_x;

  plot->point[4][0] = offset + default_width * release_x;
  plot->point[4][1] = default_height * (release_y + ratio);
  
  /* redraw */
  gtk_widget_queue_draw((GtkWidget *) cartesian);
}

/**
 * ags_envelope_editor_new:
 *
 * Create a new instance of #AgsEnvelopeEditor
 *
 * Returns: the new #AgsEnvelopeEditor
 *
 * Since: 3.0.0
 */
AgsEnvelopeEditor*
ags_envelope_editor_new()
{
  AgsEnvelopeEditor *envelope_editor;

  envelope_editor = (AgsEnvelopeEditor *) g_object_new(AGS_TYPE_ENVELOPE_EDITOR,
						       NULL);

  return(envelope_editor);
}
