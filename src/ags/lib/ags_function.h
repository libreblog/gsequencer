/* AGS - Advanced GTK Sequencer
 * Copyright (C) 2014 Joël Krähemann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __AGS_FUNCTION_H__
#define __AGS_FUNCTION_H__

#include <glib.h>
#include <glib-object.h>

#include <ags/lib/ags_complex.h>

#define AGS_TYPE_FUNCTION                (ags_function_get_type())
#define AGS_FUNCTION(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_FUNCTION, AgsFunction))
#define AGS_FUNCTION_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_FUNCTION, AgsFunctionClass))
#define AGS_IS_FUNCTION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_FUNCTION))
#define AGS_IS_FUNCTION_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_FUNCTION))
#define AGS_FUNCTION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS (obj, AGS_TYPE_FUNCTION, AgsFunctionClass))

#define AGS_SYMBOLIC_PI "𝜋\0"
#define AGS_SYMBOLIC_INFINIT "∞\0"
#define AGS_COMPLEX_UNIT "𝑖\0"

typedef struct _AgsFunction AgsFunction;
typedef struct _AgsFunctionClass AgsFunctionClass;

typedef enum{
  AGS_FUNCTION_LINEAR               = 1,
  AGS_FUNCTION_EXPONENTIAL          = 1 <<  1,
  AGS_FUNCTION_LOGARITHMIC          = 1 <<  2,
  AGS_FUNCTION_IS_UNIQUE            = 1 <<  3,
  AGS_FUNCTION_SOLVE_PIVOT_TABLE    = 1 <<  4,
  AGS_FUNCTION_SOLVE_MAXIMUM_COLON  = 1 <<  5,
  AGS_FUNCTION_SOLVE_GAUSS          = 1 <<  6,
}AgsFunctionFlags;

struct _AgsFunction
{
  GObject gobject;

  guint flags;
  
  gboolean is_pushing;
  gchar **equation;
  guint equation_count;
  
  gchar *source_function;

  gchar *normalized_function;

  gchar **symbol;
  guint symbol_count;

  guint n_rows;
  guint n_cols;
  AgsComplex*** pivot_table;
  gchar ****function_vector_table;
  
  guint solver_level;
  AgsComplex **solver_vector;
  gchar ***solver_function_vector;
};

struct _AgsFunctionClass
{
  GObjectClass gobject;

  void (*literal_solve)(AgsFunction *function);
};

GType ags_function_get_type(void);

gchar** ags_function_find_literals(AgsFunction *function,
				   guint *n_symbols);
void ags_function_literal_solve(AgsFunction *function);

gboolean ags_function_push_equation(AgsFunction *function,
				    gchar *equation);
void ags_function_pop_equation(AgsFunction *function,
			       GError **error);

gchar* ags_function_get_expanded(AgsFunction *function,
				 gchar **symbols,
				 guint n_symbols);
gchar* ags_funciton_get_normalized(AgsFunction *function);

AgsComplex* ags_function_compute_term(gchar *term,
				      gchar *substitute_symbol, AgsComplex *subsitute_value);

AgsComplex** ags_function_symbolic_translate_value(AgsFunction *function,
						   gchar *symbol,
						   AgsComplex *value);

gboolean ags_function_substitute_values(AgsFunction *function,
					gchar *symbol, ...);
AgsComplex* ags_function_translate_value(AgsFunction *function,
					 AgsComplex *value);

AgsFunction* ags_function_new();

#endif /*__AGS_FUNCTION_H__*/
