#include "ags_machine_editor_callbacks.h"

#include "../audio/ags_audio.h"
#include "../audio/ags_channel.h"
#include "../audio/ags_output.h"
#include "../audio/ags_input.h"

#include "ags_machine.h"
#include "ags_pad_editor.h"
#include "ags_line_editor.h"
#include "ags_link_editor.h"

int
ags_machine_editor_destroy_callback(GtkObject *object, AgsMachineEditor *machine_editor)
{
  ags_machine_editor_destroy(object);
  return(0);
}

int
ags_machine_editor_show_callback(GtkWidget *widget, AgsMachineEditor *machine_editor)
{
  ags_machine_editor_show(widget);
  return(0);
}

int
ags_machine_editor_switch_page_callback(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, AgsMachineEditor *machine_editor)
{
  if(page_num == 0 || page_num == 1){
    gtk_widget_show((GtkWidget *) machine_editor->add);
    gtk_widget_show((GtkWidget *) machine_editor->remove);
  }else{
    gtk_widget_hide((GtkWidget *) machine_editor->add);
    gtk_widget_hide((GtkWidget *) machine_editor->remove);
  }
}

int
ags_machine_editor_add_callback(GtkWidget *button, AgsMachineEditor *machine_editor)
{
  if(gtk_notebook_get_current_page(machine_editor->notebook) == 0)
    g_signal_emit_by_name((GObject *) machine_editor, "add_output\0", NULL);
  else
    g_signal_emit_by_name((GObject *) machine_editor, "add_input\0", NULL);

  return(0);
}

int
ags_machine_editor_remove_callback(GtkWidget *button, AgsMachineEditor *machine_editor)
{
  if(gtk_notebook_get_current_page(machine_editor->notebook) == 0)
    g_signal_emit_by_name((GObject *) machine_editor, "remove_output\0", NULL);
  else
    g_signal_emit_by_name((GObject *) machine_editor, "remove_input\0", NULL);

  return(0);
}

int
ags_machine_editor_apply_callback(GtkWidget *widget, AgsMachineEditor *machine_editor)
{
  ags_machine_property_editor_apply(machine_editor->output_property_editor);
  ags_machine_property_editor_apply(machine_editor->input_property_editor);

  ags_machine_resize_editor_apply(machine_editor->machine_resize_editor, TRUE);
  ags_machine_link_editor_apply(machine_editor->machine_link_editor, TRUE);
  ags_machine_line_member_editor_apply(machine_editor->machine_line_member_editor, TRUE);

  return(0);
}

int
ags_machine_editor_ok_callback(GtkWidget *widget, AgsMachineEditor *machine_editor)
{
  //  ags_machine_property_editor_apply(machine_editor->output_property_editor);
  //  ags_machine_property_editor_apply(machine_editor->input_property_editor);

  //  ags_machine_resize_editor_apply(machine_editor->machine_resize_editor, FALSE);
  ags_machine_link_editor_apply(machine_editor->machine_link_editor, FALSE);
  //  ags_machine_line_member_editor_apply(machine_editor->machine_line_member_editor, FALSE);

  gtk_widget_destroy((GtkWidget *) machine_editor);

  return(0);
}

int
ags_machine_editor_cancel_callback(GtkWidget *widget, AgsMachineEditor *machine_editor)
{
  gtk_widget_destroy((GtkWidget *) machine_editor);

  return(0);
}
