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

#ifndef __AGS_PRESET_H__
#define __AGS_PRESET_H__

#include <glib.h>
#include <glib-object.h>

#define AGS_TYPE_PRESET                (ags_preset_get_type())
#define AGS_PRESET(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_PRESET, AgsPreset))
#define AGS_PRESET_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_PRESET, AgsPresetClass))
#define AGS_IS_PRESET(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_PRESET))
#define AGS_IS_PRESET_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_PRESET))
#define AGS_PRESET_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_PRESET, AgsPresetClass))

typedef struct _AgsPreset AgsPreset;
typedef struct _AgsPresetClass AgsPresetClass;

typedef enum{
  AGS_PRESET_CONNECTED      = 1,
}AgsPresetFlags;

struct _AgsPreset
{
  GObject gobject;

  guint flags;

  gchar *preset_name;

  guint audio_channel_start;
  guint audio_channel_end;

  guint pad_start;
  guint pad_end;

  guint x_start;
  guint x_end;
  
  GParameter *parameter;
  guint n_params;
};

struct _AgsPresetClass
{
  GObject gobject;
};

AgsPreset* ags_preset_new();

#endif /*__AGS_PRESET_H__*/
