/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2017 Joël Krähemann
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

#ifndef __AGS_ENVELOPE_DIALOG_H__
#define __AGS_ENVELOPE_DIALOG_H__

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include <ags/X/ags_machine.h>

#include <ags/X/editor/ags_envelope_editor.h>
#include <ags/X/editor/ags_pattern_envelope.h>
#include <ags/X/editor/ags_envelope_info.h>

#define AGS_TYPE_ENVELOPE_DIALOG                (ags_envelope_dialog_get_type())
#define AGS_ENVELOPE_DIALOG(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_ENVELOPE_DIALOG, AgsEnvelopeDialog))
#define AGS_ENVELOPE_DIALOG_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_ENVELOPE_DIALOG, AgsEnvelopeDialogClass))
#define AGS_IS_ENVELOPE_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_ENVELOPE_DIALOG))
#define AGS_IS_ENVELOPE_DIALOG_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_ENVELOPE_DIALOG))
#define AGS_ENVELOPE_DIALOG_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_ENVELOPE_DIALOG, AgsEnvelopeDialogClass))

#define AGS_ENVELOPE_DIALOG_DEFAULT_VERSION "0.8.1\0"
#define AGS_ENVELOPE_DIALOG_DEFAULT_BUILD_ID "CEST 29-04-2017 09:55\0"

typedef struct _AgsEnvelopeDialog AgsEnvelopeDialog;
typedef struct _AgsEnvelopeDialogClass AgsEnvelopeDialogClass;

typedef enum{
  AGS_ENVELOPE_DIALOG_CONNECTED     = 1,
  AGS_ENVELOPE_DIALOG_PATTERN_TAB   = 1 <<  1,
}AgsEnvelopeDialogFlags;

struct _AgsEnvelopeDialog
{
  GtkDialog dialog;

  guint flags;
  
  gchar *version;
  gchar *build_id;
  
  AgsMachine *machine;

  GtkNotebook *notebook;

  GtkScrolledWindow *envelope_editor_scrolled_window;
  AgsEnvelopeEditor *envelope_editor;

  GtkScrolledWindow *pattern_envelope_scrolled_window;
  AgsPatternEnvelope *pattern_envelope;

  GtkScrolledWindow *envelope_info_scrolled_window;
  AgsEnvelopeInfo *envelope_info;
  
  GtkButton *apply;
  GtkButton *ok;
  GtkButton *cancel;
};

struct _AgsEnvelopeDialogClass
{
  GtkDialogClass dialog;
};

GType ags_envelope_dialog_get_type(void);

void ags_envelope_dialog_load_preset(AgsEnvelopeDialog *envelope_dialog);

void ags_envelope_dialog_add_pattern_tab(AgsEnvelopeDialog *envelope_dialog);

AgsEnvelopeDialog* ags_envelope_dialog_new(AgsMachine *machine);

#endif /*__AGS_ENVELOPE_DIALOG_H__*/
