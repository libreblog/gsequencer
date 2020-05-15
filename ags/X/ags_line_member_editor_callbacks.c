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

#include <ags/X/ags_line_member_editor_callbacks.h>

#include <ags/X/ags_ui_provider.h>
#include <ags/X/ags_window.h>
#include <ags/X/ags_machine.h>
#include <ags/X/ags_pad.h>
#include <ags/X/ags_line.h>
#include <ags/X/ags_effect_bridge.h>
#include <ags/X/ags_effect_pad.h>
#include <ags/X/ags_effect_line.h>
#include <ags/X/ags_line_member.h>
#include <ags/X/ags_machine_editor.h>
#include <ags/X/ags_line_editor.h>
#include <ags/X/ags_lv2_browser.h>
#include <ags/X/ags_ladspa_browser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <ladspa.h>

void ags_line_member_editor_plugin_browser_response_create_entry(AgsLineMemberEditor *line_member_editor,
								 gchar *filename, gchar *effect);
  
void
ags_line_member_editor_add_callback(GtkWidget *button,
				    AgsLineMemberEditor *line_member_editor)
{
  if(line_member_editor->plugin_browser == NULL){
    line_member_editor->plugin_browser = ags_plugin_browser_new((GtkWidget *) line_member_editor);

    ags_connectable_connect(AGS_CONNECTABLE(line_member_editor->plugin_browser));

    g_signal_connect(G_OBJECT(line_member_editor->plugin_browser), "response",
		     G_CALLBACK(ags_line_member_editor_plugin_browser_response_callback), line_member_editor);
  }
  
  gtk_widget_show_all((GtkWidget *) line_member_editor->plugin_browser);
}

void
ags_line_member_editor_plugin_browser_response_create_entry(AgsLineMemberEditor *line_member_editor,
							    gchar *filename, gchar *effect)
{
  GtkHBox *hbox;
  GtkCheckButton *check_button;
  GtkLabel *label;

  gchar *str;
  
  /* create entry */
  hbox = (GtkHBox *) gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(line_member_editor->line_member),
		     GTK_WIDGET(hbox),
		     FALSE, FALSE,
		     0);
      
  check_button = (GtkCheckButton *) gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox),
		     GTK_WIDGET(check_button),
		     FALSE, FALSE,
		     0);

  str = g_strdup_printf("%s - %s",
			filename,
			effect);
    
  label = (GtkLabel *) gtk_label_new(str);
  gtk_box_pack_start(GTK_BOX(hbox),
		     GTK_WIDGET(label),
		     FALSE, FALSE,
		     0);
  gtk_widget_show_all((GtkWidget *) hbox);

  g_free(str);
}

void
ags_line_member_editor_plugin_browser_response_callback(GtkDialog *dialog,
							gint response,
							AgsLineMemberEditor *line_member_editor)
{
  AgsWindow *window;
  AgsMachine *machine;
  AgsMachineEditor *machine_editor;
  AgsLineEditor *line_editor;
  
  AgsApplicationContext *application_context;
  
  GList *pad, *pad_start;
  GList *list, *list_start;

  gchar *filename, *effect;

  gboolean has_bridge;
  gboolean is_output;  
  
  switch(response){
  case GTK_RESPONSE_ACCEPT:
    {
      machine_editor = (AgsMachineEditor *) gtk_widget_get_ancestor((GtkWidget *) line_member_editor,
								    AGS_TYPE_MACHINE_EDITOR);
      line_editor = (AgsLineEditor *) gtk_widget_get_ancestor((GtkWidget *) line_member_editor,
							      AGS_TYPE_LINE_EDITOR);

      machine = machine_editor->machine;

      window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) machine);

      application_context = ags_application_context_get_instance();

      if(AGS_IS_OUTPUT(line_editor->channel)){
	is_output = TRUE;
      }else{
	is_output = FALSE;
      }

      if(machine->bridge != NULL){
	has_bridge = TRUE;
      }else{
	has_bridge = FALSE;
      }
      
      if(!has_bridge){	
	AgsLine *line;
	
	/* find pad and line */
	line = NULL;
	
	if(is_output){
	  pad_start = 
	    pad = gtk_container_get_children((GtkContainer *) machine_editor->machine->output);
	}else{
	  pad_start = 
	    pad = gtk_container_get_children((GtkContainer *) machine_editor->machine->input);
	}

	pad = g_list_nth(pad,
			 line_editor->channel->pad);

	if(pad != NULL){
	  list_start =
	    list = gtk_container_get_children((GtkContainer *) AGS_PAD(pad->data)->expander_set);

	  while(list != NULL){
	    if(AGS_LINE(list->data)->channel == line_editor->channel){
	      break;
	    }

	    list = list->next;
	  }

	  if(list != NULL){
	    line = AGS_LINE(list->data);
	    g_list_free(list_start);
	  }
	}

	g_list_free(pad_start);

	/* retrieve plugin */
	filename = ags_plugin_browser_get_plugin_filename(line_member_editor->plugin_browser);
	effect = ags_plugin_browser_get_plugin_effect(line_member_editor->plugin_browser);

	if(line != NULL){
	  //TODO:JK: implement me
//	  ags_line_add_plugin();
	}
	
	g_free(filename);
	g_free(effect);
      }else{
	AgsEffectBridge *effect_bridge;
	AgsEffectLine *effect_line;
	
	effect_bridge = (AgsEffectBridge *) machine->bridge;
	effect_line = NULL;
	
	/* find effect pad and effect line */
	if(is_output){
	  pad_start = 
	    pad = gtk_container_get_children((GtkContainer *) effect_bridge->output);
	}else{
	  pad_start = 
	    pad = gtk_container_get_children((GtkContainer *) effect_bridge->input);
	}

	pad = g_list_nth(pad,
			 line_editor->channel->pad);

	if(pad != NULL){
	  list_start =
	    list = gtk_container_get_children((GtkContainer *) AGS_EFFECT_PAD(pad->data)->table);

	  while(list != NULL){
	    if(AGS_EFFECT_LINE(list->data)->channel == line_editor->channel){
	      break;
	    }

	    list = list->next;
	  }

	  if(list != NULL){
	    effect_line = AGS_EFFECT_LINE(list->data);
	    g_list_free(list_start);
	  }
	}

	g_list_free(pad_start);

	/* retrieve plugin */
	filename = ags_plugin_browser_get_plugin_filename(line_member_editor->plugin_browser);
	effect = ags_plugin_browser_get_plugin_effect(line_member_editor->plugin_browser);

	if(effect_line != NULL){
	  //TODO:JK: implement me
//	  ags_effect_line_add_plugin();
	}
	
	g_free(filename);
	g_free(effect);
      }
    }
    break;      
  }
}

void
ags_line_member_editor_remove_callback(GtkWidget *button,
				       AgsLineMemberEditor *line_member_editor)
{
  AgsMachine *machine;
  AgsMachineEditor *machine_editor;
  AgsLineEditor *line_editor;

  GList *start_line_member, *line_member;
  GList *list, *list_start, *pad, *pad_start;
  GList *children;

  guint nth;
  gboolean has_bridge;
  gboolean is_output;
  
  if(button == NULL ||
     line_member_editor == NULL){
    return;
  }

  machine_editor = (AgsMachineEditor *) gtk_widget_get_ancestor((GtkWidget *) line_member_editor,
								AGS_TYPE_MACHINE_EDITOR);
  line_editor = (AgsLineEditor *) gtk_widget_get_ancestor((GtkWidget *) line_member_editor,
							  AGS_TYPE_LINE_EDITOR);

  line_member =
    start_line_member = gtk_container_get_children(GTK_CONTAINER(line_member_editor->line_member));

  machine = machine_editor->machine;

  if(AGS_IS_OUTPUT(line_editor->channel)){
    is_output = TRUE;
  }else{
    is_output = FALSE;
  }

  if(machine->bridge != NULL){
    has_bridge = TRUE;
  }else{
    has_bridge = FALSE;
  }

  if(!has_bridge){	
    AgsLine *line;
    
    /* retrieve line and pad */
    line = NULL;

    if(AGS_IS_OUTPUT(line_editor->channel)){
      pad_start = 
	pad = gtk_container_get_children((GtkContainer *) machine->output);
    }else{
      pad_start = 
	pad = gtk_container_get_children((GtkContainer *) machine->input);
    }

    pad = g_list_nth(pad,
		     line_editor->channel->pad);

    if(pad != NULL){
      list_start =
	list = gtk_container_get_children((GtkContainer *) AGS_PAD(pad->data)->expander_set);

      while(list != NULL){
	if(AGS_LINE(list->data)->channel == line_editor->channel){
	  break;
	}

	list = list->next;
      }

      if(list != NULL){
	line = AGS_LINE(list->data);
	g_list_free(list_start);
      }
    }

    g_list_free(pad_start);

    /* iterate line member */
    if(line != NULL){
      //TODO:JK: implement me
//	  ags_line_remove_plugin();      
    }
  }else{
    AgsEffectBridge *effect_bridge;
    AgsEffectLine *effect_line;
	
    effect_bridge = AGS_EFFECT_BRIDGE(machine->bridge);

    effect_line = NULL;
    
    /* retrieve effect line and effect pad */
    if(is_output){
      pad_start = 
	pad = gtk_container_get_children((GtkContainer *) effect_bridge->output);
    }else{
      pad_start = 
	pad = gtk_container_get_children((GtkContainer *) effect_bridge->input);
    }

    pad = g_list_nth(pad,
		     line_editor->channel->pad);

    if(pad != NULL){
      list_start =
	list = gtk_container_get_children((GtkContainer *) AGS_EFFECT_PAD(pad->data)->table);

      while(list != NULL){
	if(AGS_EFFECT_LINE(list->data)->channel == line_editor->channel){
	  break;
	}

	list = list->next;
      }

      if(list != NULL){
	effect_line = AGS_EFFECT_LINE(list->data);
	g_list_free(list_start);
      }
    }

    g_list_free(pad_start);

    /* iterate line member */
    if(effect_line != NULL){
      //TODO:JK: implement me
//	  ags_effect_line_remove_plugin();
    }
  }

  g_list_free(start_line_member);
}
