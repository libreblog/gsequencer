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

#ifndef __AGS_ANALYSE_CHANNEL_RUN_H__
#define __AGS_ANALYSE_CHANNEL_RUN_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/libags.h>

#include <ags/audio/ags_channel.h>
#include <ags/audio/ags_recall_channel_run.h>

G_BEGIN_DECLS

#define AGS_TYPE_ANALYSE_CHANNEL_RUN            (ags_analyse_channel_run_get_type())
#define AGS_ANALYSE_CHANNEL_RUN(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, AGS_TYPE_ANALYSE_CHANNEL_RUN, AgsAnalyseChannelRun))
#define AGS_ANALYSE_CHANNEL_RUN_CLASS(class)    (G_TYPE_CHECK_INSTANCE_CAST(class, AGS_TYPE_ANALYSE_CHANNEL_RUN, AgsAnalyseChannelRunClass))
#define AGS_IS_ANALYSE_CHANNEL_RUN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_ANALYSE_CHANNEL_RUN))
#define AGS_IS_ANALYSE_CHANNEL_RUN_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE((class), AGS_TYPE_ANALYSE_CHANNEL_RUN))
#define AGS_ANALYSE_CHANNEL_RUN_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS((obj), AGS_TYPE_ANALYSE_CHANNEL_RUN, AgsAnalyseChannelRunClass))

typedef struct _AgsAnalyseChannelRun AgsAnalyseChannelRun;
typedef struct _AgsAnalyseChannelRunClass AgsAnalyseChannelRunClass;

struct _AgsAnalyseChannelRun
{
  AgsRecallChannelRun recall_channel_run;
};

struct _AgsAnalyseChannelRunClass
{
  AgsRecallChannelRunClass recall_channel_run;
};

GType ags_analyse_channel_run_get_type();

AgsAnalyseChannelRun* ags_analyse_channel_run_new(AgsChannel *source);

G_END_DECLS

#endif /*__AGS_ANALYSE_CHANNEL_RUN_H__*/
