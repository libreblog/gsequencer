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

#ifndef __AGS_FX_LV2_RECYCLING_H__
#define __AGS_FX_LV2_RECYCLING_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>

#include <ags/audio/ags_recycling.h>
#include <ags/audio/ags_recall_recycling.h>

G_BEGIN_DECLS

#define AGS_TYPE_FX_LV2_RECYCLING                (ags_fx_lv2_recycling_get_type())
#define AGS_FX_LV2_RECYCLING(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_FX_LV2_RECYCLING, AgsFxLv2Recycling))
#define AGS_FX_LV2_RECYCLING_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_FX_LV2_RECYCLING, AgsFxLv2Recycling))
#define AGS_IS_FX_LV2_RECYCLING(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_FX_LV2_RECYCLING))
#define AGS_IS_FX_LV2_RECYCLING_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_FX_LV2_RECYCLING))
#define AGS_FX_LV2_RECYCLING_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_FX_LV2_RECYCLING, AgsFxLv2RecyclingClass))

typedef struct _AgsFxLv2Recycling AgsFxLv2Recycling;
typedef struct _AgsFxLv2RecyclingClass AgsFxLv2RecyclingClass;

struct _AgsFxLv2Recycling
{
  AgsRecallRecycling recall_recycling;
};

struct _AgsFxLv2RecyclingClass
{
  AgsRecallRecyclingClass recall_recycling;
};

GType ags_fx_lv2_recycling_get_type();

/*  */
AgsFxLv2Recycling* ags_fx_lv2_recycling_new(AgsRecycling *recycling);

G_END_DECLS

#endif /*__AGS_FX_LV2_RECYCLING_H__*/
