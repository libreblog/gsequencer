/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2015 Joël Krähemann
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

#include <ags/object/ags_stackable.h>

#include <stdio.h>

void ags_stackable_base_init(AgsStackableInterface *interface);

GType
ags_stackable_get_type()
{
  static GType ags_type_stackable = 0;

  if(!ags_type_stackable){
    static const GTypeInfo ags_stackable_info = {
      sizeof(AgsStackableInterface),
      (GBaseInitFunc) ags_stackable_base_init,
      NULL, /* base_finalize */
    };

    ags_type_stackable = g_type_register_static(G_TYPE_INTERFACE,
						"AgsStackable\0", &ags_stackable_info,
						0);
  }

  return(ags_type_stackable);
}

void
ags_stackable_base_init(AgsStackableInterface *interface)
{
  /* empty */
}

void
ags_stackable_push(AgsStackable *stackable)
{
  AgsStackableInterface *stackable_interface;

  g_return_if_fail(AGS_IS_STACKABLE(stackable));
  stackable_interface = AGS_STACKABLE_GET_INTERFACE(stackable);
  g_return_if_fail(stackable_interface->push);
  stackable_interface->push(stackable);
}

void
ags_stackable_pop(AgsStackable *stackable)
{
  AgsStackableInterface *stackable_interface;

  g_return_if_fail(AGS_IS_STACKABLE(stackable));
  stackable_interface = AGS_STACKABLE_GET_INTERFACE(stackable);
  g_return_if_fail(stackable_interface->pop);
  stackable_interface->pop(stackable);
}
