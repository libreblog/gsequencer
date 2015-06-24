/* This file is part of GSequencer.
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

#ifndef __AGS_NOTEBOOK_CALLBACKS_H__
#define __AGS_NOTEBOOK_CALLBACKS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <ags/X/editor/ags_notebook.h>

#include <ags/X/ags_machine.h>
#include <ags/X/ags_editor.h>

void ags_notebook_machine_changed_callback(AgsEditor *editor, AgsMachine *machine,
					   AgsNotebook *notebook);

#endif /*__AGS_NOTEBOOK_CALLBACKS_H__*/
