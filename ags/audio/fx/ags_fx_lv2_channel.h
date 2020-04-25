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

#ifndef __AGS_FX_LV2_CHANNEL_H__
#define __AGS_FX_LV2_CHANNEL_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>

#include <ags/audio/ags_channel.h>
#include <ags/audio/ags_recall_channel.h>

G_BEGIN_DECLS

#define AGS_TYPE_FX_LV2_CHANNEL                (ags_fx_lv2_channel_get_type())
#define AGS_FX_LV2_CHANNEL(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_FX_LV2_CHANNEL, AgsFxLv2Channel))
#define AGS_FX_LV2_CHANNEL_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_FX_LV2_CHANNEL, AgsFxLv2Channel))
#define AGS_IS_FX_LV2_CHANNEL(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_FX_LV2_CHANNEL))
#define AGS_IS_FX_LV2_CHANNEL_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_FX_LV2_CHANNEL))
#define AGS_FX_LV2_CHANNEL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_FX_LV2_CHANNEL, AgsFxLv2ChannelClass))

typedef struct _AgsFxLv2Channel AgsFxLv2Channel;
typedef struct _AgsFxLv2ChannelClass AgsFxLv2ChannelClass;

struct _AgsFxLv2Channel
{
  AgsRecallChannel recall_channel;
};

struct _AgsFxLv2ChannelClass
{
  AgsRecallChannelClass recall_channel;
};

GType ags_fx_lv2_channel_get_type();

/*  */
AgsFxLv2Channel* ags_fx_lv2_channel_new(AgsChannel *channel);

G_END_DECLS

#endif /*__AGS_FX_LV2_CHANNEL_H__*/
