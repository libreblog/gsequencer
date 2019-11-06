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

#ifndef __AGS_TACTABLE_H__
#define __AGS_TACTABLE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define AGS_TYPE_TACTABLE                    (ags_tactable_get_type())
#define AGS_TACTABLE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_TACTABLE, AgsTactable))
#define AGS_TACTABLE_INTERFACE(vtable)       (G_TYPE_CHECK_CLASS_CAST((vtable), AGS_TYPE_TACTABLE, AgsTactableInterface))
#define AGS_IS_TACTABLE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE((obj), AGS_TYPE_TACTABLE))
#define AGS_IS_TACTABLE_INTERFACE(vtable)    (G_TYPE_CHECK_CLASS_TYPE((vtable), AGS_TYPE_TACTABLE))
#define AGS_TACTABLE_GET_INTERFACE(obj)      (G_TYPE_INSTANCE_GET_INTERFACE((obj), AGS_TYPE_TACTABLE, AgsTactableInterface))

typedef struct _AgsTactable AgsTactable;
typedef struct _AgsTactableInterface AgsTactableInterface;

struct _AgsTactableInterface
{
  GTypeInterface ginterface;

  guint64 (*get_sequencer_duration)(AgsTactable *tactable);
  guint64 (*get_notation_duration)(AgsTactable *tactable);
  guint64 (*get_wave_duration)(AgsTactable *tactable);
  guint64 (*get_midi_duration)(AgsTactable *tactable);

  gdouble (*get_bpm)(AgsTactable *tactable);
  gdouble (*get_tact)(AgsTactable *tactable);
    
  void (*change_sequencer_duration)(AgsTactable *tactable, guint64 duration);
  void (*change_notation_duration)(AgsTactable *tactable, guint64 duration);
  void (*change_wave_duration)(AgsTactable *tactable, guint64 duration);
  void (*change_midi_duration)(AgsTactable *tactable, guint64 duration);

  void (*change_bpm)(AgsTactable *tactable, gdouble new_bpm, gdouble old_bpm);
  void (*change_tact)(AgsTactable *tactable, gdouble new_tact, gdouble old_tact);
};

GType ags_tactable_get_type();

guint64 ags_tactable_get_sequencer_duration(AgsTactable *tactable);
guint64 ags_tactable_get_notation_duration(AgsTactable *tactable);
guint64 ags_tactable_get_wave_duration(AgsTactable *tactable);
guint64 ags_tactable_get_midi_duration(AgsTactable *tactable);

gdouble ags_tactable_get_bpm(AgsTactable *tactable);
gdouble ags_tactable_get_tact(AgsTactable *tactable);

void ags_tactable_change_sequencer_duration(AgsTactable *tactable, guint64 duration);
void ags_tactable_change_notation_duration(AgsTactable *tactable, guint64 duration);
void ags_tactable_change_wave_duration(AgsTactable *tactable, guint64 duration);
void ags_tactable_change_midi_duration(AgsTactable *tactable, guint64 duration);

void ags_tactable_change_bpm(AgsTactable *tactable, gdouble new_bpm, gdouble old_bpm);
void ags_tactable_change_tact(AgsTactable *tactable, gdouble new_tact, gdouble old_tact);

G_END_DECLS

#endif /*__AGS_TACTABLE_H__*/
