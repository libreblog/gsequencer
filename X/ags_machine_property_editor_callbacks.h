#ifndef __AGS_MACHINE_PROPERTY_EDITOR_CALLBACKS_H__
#define __AGS_MACHINE_PROPERTY_EDITOR_CALLBACKS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "ags_machine_property_editor.h"
#include "ags_machine_editor.h"

int ags_machine_property_editor_parent_set_callback(GtkWidget *widget, GtkObject *old_parent, AgsMachinePropertyEditor *machine_property_editor);
int ags_machine_property_editor_show_callback(GtkWidget *widget, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_set_machine_callback(AgsMachineEditor *machine_editor, AgsMachine *machine, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_add_input_callback(AgsMachineEditor *machine_editor, AgsMachinePropertyEditor *machine_property_editor);
void ags_machine_property_editor_add_output_callback(AgsMachineEditor *machine_editor, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_remove_input_callback(AgsMachineEditor *machine_editor, AgsMachinePropertyEditor *machine_property_editor);
void ags_machine_property_editor_remove_output_callback(AgsMachineEditor *machine_editor, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_resize_audio_channels_callback(AgsMachineResizeEditor *machine_resize_editor, guint audio_channels, guint audio_channels_old, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_resize_input_pads_callback(AgsMachineResizeEditor *machine_resize_editor, guint pads, guint pads_old, AgsMachinePropertyEditor *machine_property_editor);
void ags_machine_property_editor_resize_output_pads_callback(AgsMachineResizeEditor *machine_resize_editor, guint pads, guint pads_old, AgsMachinePropertyEditor *machine_property_editor);

void ags_machine_property_editor_reset_input_callback(AgsMachineLinkEditor *machine_link_editor, AgsMachinePropertyEditor *machine_property_editor);
void ags_machine_property_editor_reset_output_callback(AgsMachineLinkEditor *machine_link_editor, AgsMachinePropertyEditor *machine_property_editor);

#endif /*__AGS_MACHINE_PROPERTY_EDITOR_CALLBACKS_H__*/
