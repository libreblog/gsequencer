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

#include <ags/file/ags_file_thread.h>

#include <libxml/parser.h>
#include <libxml/xlink.h>
#include <libxml/xpath.h>

#include <ags/util/ags_id_generator.h>

#include <ags/file/ags_file_stock.h>
#include <ags/file/ags_file_id_ref.h>
#include <ags/file/ags_file_lookup.h>
#include <ags/file/ags_file_launch.h>

#include <ags/thread/ags_task_thread.h>
#include <ags/thread/ags_returnable_thread.h>
#include <ags/thread/ags_devout_thread.h>
#include <ags/thread/ags_audio_thread.h>
#include <ags/thread/ags_channel_thread.h>

void ags_file_read_thread_start(AgsFileLaunch *file_launch, AgsThread *thread);

void ags_file_read_thread_resolve_devout(AgsFileLookup *file_lookup,
					 AgsThread *thread);
void ags_file_write_thread_resolve_devout(AgsFileLookup *file_lookup,
					  AgsThread *thread);

void ags_file_read_thread_pool_start(AgsFileLaunch *file_launch, AgsThreadPool *thread_pool);

void
ags_file_read_thread(AgsFile *file, xmlNode *node, AgsThread **thread)
{
  AgsFileLookup *file_lookup;
  AgsFileLaunch *file_launch;
  AgsThread *gobject;
  xmlNode *child;
  xmlChar *type_name;
  static gboolean thread_type_is_registered = FALSE;

  if(*thread != NULL &&
     AGS_IS_RETURNABLE_THREAD(*thread)){
    return;
  }

  if(*thread == NULL){
    GType type;

    if(!thread_type_is_registered){
      ags_main_register_thread_type();

      thread_type_is_registered = TRUE;
    }

    type_name = xmlGetProp(node,
			   AGS_FILE_TYPE_PROP);

    type = g_type_from_name(type_name);

    if(g_type_is_a(type,
		   AGS_TYPE_RETURNABLE_THREAD) ||
       g_type_is_a(type,
		   AGS_TYPE_AUDIO_THREAD) ||
       g_type_is_a(type,
		   AGS_TYPE_CHANNEL_THREAD)){
      return;
    }

    gobject = g_object_new(type,
			   NULL);

    *thread = gobject;
  }else{
    gobject = *thread;
  }

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", xmlGetProp(node, AGS_FILE_ID_PROP)),
				   "reference\0", gobject,
				   NULL));

  g_atomic_int_set(&(gobject->flags),
		   ((~(AGS_THREAD_WAIT_0 |
		       AGS_THREAD_WAIT_1 |
		       AGS_THREAD_WAIT_2 |
		       AGS_THREAD_RUNNING)) & (guint) g_ascii_strtoull(xmlGetProp(node,
										  AGS_FILE_FLAGS_PROP),
								       NULL,
								       16)));

  /* start */
  if((AGS_THREAD_RUNNING & (g_atomic_int_get(&(gobject->flags)))) != 0){
    //FIXME:JK: workaround file setting AGS_THREAD_RUNNING is just ignored
    if(AGS_IS_AUDIO_LOOP(gobject)){
      //      file_launch = (AgsFileLaunch *) g_object_new(AGS_TYPE_FILE_LAUNCH,
      //					   NULL);
      //      ags_file_add_launch(file, (GObject *) file_launch);
      //      g_signal_connect(G_OBJECT(file_launch), "start\0",
      //	       G_CALLBACK(ags_file_read_thread_start), gobject);
    }
  }

  /* devout */
  file_lookup = (AgsFileLookup *) g_object_new(AGS_TYPE_FILE_LOOKUP,
					       "file\0", file,
					       "node\0", node,
					       "reference\0", gobject,
					       NULL);
  ags_file_add_lookup(file, (GObject *) file_lookup);
  g_signal_connect(G_OBJECT(file_lookup), "resolve\0",
		   G_CALLBACK(ags_file_read_thread_resolve_devout), gobject);

  /* read children */
  child = node->children;

  while(child != NULL){
    if(child->type == XML_ELEMENT_NODE){
      if(!xmlStrncmp(child->name,
		     "ags-thread-list\0",
		     16)){
	if(AGS_IS_AUDIO_LOOP(gobject)){
	  AgsThread *async_queue;
	  AgsThread *devout_thread;
	  AgsThread *gui_thread;
	  AgsThread *timestamp_thread;
	  AgsThread *export_thread;
	  
	  xmlXPathContext *xpath_context;
	  xmlXPathObject *xpath_object;
    
	  /* task thread */
	  async_queue = NULL;
	  
	  xpath_context = xmlXPathNewContext(file->doc);
	  xpath_context->node = child;
	  //	  xmlXPathSetContextNode(child,
	  //			 xpath_context);
	  xpath_object = xmlXPathCompiledEval(xmlXPathCompile("./ags-thread[@type='AgsTaskThread']\0"),
					      xpath_context);
	  //	  xmlXPathNodeEval(child,
	  //		   "./ags-thread[@type='AgsTaskThread']\0",
	  //		   xpath_context);

	  ags_file_read_thread(file,
			       xpath_object->nodesetval->nodeTab[0],
			       &(async_queue));
	  AGS_AUDIO_LOOP(gobject)->async_queue = async_queue;
	  ags_thread_add_child_extended(gobject,
					async_queue,
					TRUE, TRUE);

	  /* devout thread */
	  devout_thread = NULL;
	  
	  xpath_context = xmlXPathNewContext(file->doc);
	  xpath_context->node = child;
	  xpath_object = xmlXPathCompiledEval(xmlXPathCompile("./ags-thread[@type='AgsDevoutThread']\0"),
					      xpath_context);

	  ags_file_read_thread(file,
			       xpath_object->nodesetval->nodeTab[0],
			       &(devout_thread));
	  ags_thread_add_child_extended(gobject,
					devout_thread,
					TRUE, TRUE);

	  /* timestamp thread */
	  timestamp_thread = NULL;
	  
	  xpath_context = xmlXPathNewContext(file->doc);
	  xpath_context->node = child;
	  xpath_object = xmlXPathCompiledEval(xmlXPathCompile("./ags-thread[@type='AgsDevoutThread']/ags-thread-list/ags-thread[@type='AgsTimestampThread']\0"),
					      xpath_context);

	  ags_file_read_thread(file,
			       xpath_object->nodesetval->nodeTab[0],
			       &(timestamp_thread));
	  ags_thread_add_child_extended(devout_thread,
					timestamp_thread,
					TRUE, TRUE);

	  /* gui thread */
	  gui_thread = NULL;

	  xpath_context = xmlXPathNewContext(file->doc);
	  xpath_context->node = child;
	  xpath_object = xmlXPathCompiledEval(xmlXPathCompile("./ags-thread[@type='AgsGuiThread']\0"),
					      xpath_context);

	  ags_file_read_thread(file,
			       xpath_object->nodesetval->nodeTab[0],
			       &(gui_thread));
	  ags_thread_add_child_extended(gobject,
					gui_thread,
					TRUE, TRUE);

	  /* export thread */
	  export_thread = NULL;

	  xpath_context = xmlXPathNewContext(file->doc);
	  xpath_context->node = child;
	  xpath_object = xmlXPathCompiledEval(xmlXPathCompile("./ags-thread[@type='AgsExportThread']\0"),
					      xpath_context);

	  ags_file_read_thread(file,
			       xpath_object->nodesetval->nodeTab[0],
			       &(export_thread));
	  ags_thread_add_child_extended(gobject,
					export_thread,
					TRUE, TRUE);
	}else{
	  GList *list;

	  list = NULL;

	  //FIXME:JK: buggy
	  //	  ags_file_read_thread_list(file,
	  //			    child,
	  //			    &list);

	  //  while(list != NULL){
	    //  ags_thread_add_child(gobject,
	    //			 list->data);

	    // list = list->next;
	  //  }
	}
      }else if(!xmlStrncmp(child->name,
			   "ags-audio-loop\0",
			   15)){
	ags_file_read_audio_loop(file,
				 child,
				 AGS_AUDIO_LOOP(gobject));
      }
    }

    child = child->next;
  }
}

void 
ags_file_read_thread_resolve_devout(AgsFileLookup *file_lookup,
				    AgsThread *thread)
{
  AgsFileIdRef *id_ref;
  gchar *xpath;

  xpath = (gchar *) xmlGetProp(file_lookup->node,
			       "devout\0");

  id_ref = (AgsFileIdRef *) ags_file_find_id_ref_by_xpath(file_lookup->file, xpath);

  if(id_ref != NULL){
    g_object_set(G_OBJECT(thread),
		 "devout\0", id_ref->ref,
		 NULL);
  }
}

void
ags_file_read_thread_start(AgsFileLaunch *file_launch, AgsThread *thread)
{
  ags_thread_start(thread);

  /* wait thread */
  pthread_mutex_lock(thread->start_mutex);

  g_atomic_int_set(&(thread->start_wait),
		   TRUE);
	
  if(g_atomic_int_get(&(thread->start_wait)) == TRUE &&
     g_atomic_int_get(&(thread->start_done)) == FALSE){
    while(g_atomic_int_get(&(thread->start_wait)) == TRUE &&
	  g_atomic_int_get(&(thread->start_done)) == FALSE){
      pthread_cond_wait(thread->start_cond,
			thread->start_mutex);
    }
  }
	
  pthread_mutex_unlock(thread->start_mutex);
}

xmlNode*
ags_file_write_thread(AgsFile *file, xmlNode *parent, AgsThread *thread)
{
  AgsFileLookup *file_lookup;
  AgsThread *current;
  xmlNode *node, *child;
  gchar *id;

  if(AGS_IS_RETURNABLE_THREAD(thread)){
    return;
  }

  id = ags_id_generator_create_uuid();

  node = xmlNewNode(NULL,
		    "ags-thread\0");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);
 
  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", thread,
				   NULL));

  xmlNewProp(node,
	     AGS_FILE_TYPE_PROP,
	     G_OBJECT_TYPE_NAME(thread));

  xmlNewProp(node,
	     AGS_FILE_FLAGS_PROP,
	     g_strdup_printf("%x\0", thread->flags));

  /* devout */
  file_lookup = (AgsFileLookup *) g_object_new(AGS_TYPE_FILE_LOOKUP,
					       "file\0", file,
					       "node\0", node,
					       "reference\0", thread,
					       NULL);
  ags_file_add_lookup(file, (GObject *) file_lookup);
  g_signal_connect(G_OBJECT(file_lookup), "resolve\0",
		   G_CALLBACK(ags_file_write_thread_resolve_devout), thread);

  xmlAddChild(parent,
	      node);

  /* child elements */
  if(AGS_IS_AUDIO_LOOP(thread)){
    ags_file_write_audio_loop(file,
			      node,
			      AGS_AUDIO_LOOP(thread));
  }

  current = thread->children;

  child = xmlNewNode(NULL,
		     "ags-thread-list\0");
  xmlAddChild(node,
	      child);

  while(current != NULL){
    ags_file_write_thread(file,
			  child,
			  current);
    current = current->next;
  }
}

void
ags_file_write_thread_resolve_devout(AgsFileLookup *file_lookup,
				     AgsThread *thread)
{
  AgsFileIdRef *id_ref;
  gchar *id;

  id_ref = (AgsFileIdRef *) ags_file_find_id_ref_by_reference(file_lookup->file, thread->devout);

  id = xmlGetProp(id_ref->node, AGS_FILE_ID_PROP);

  xmlNewProp(file_lookup->node,
	     "devout\0",
	     g_strdup_printf("xpath=//ags-devout[@id='%s']\0", id));
}

void
ags_file_read_thread_list(AgsFile *file, xmlNode *node, GList **thread)
{
  AgsThread *current;
  GList *list;
  xmlNode *child;
  xmlChar *id;

  id = xmlGetProp(node, AGS_FILE_ID_PROP);

  child = node->children;
  list = NULL;

  while(child != NULL){
    if(child->type == XML_ELEMENT_NODE){
      if(!xmlStrncmp(child->name,
		     "ags-thread\0",
		     11)){
	current = NULL;
	ags_file_read_thread(file, child, &current);
	list = g_list_prepend(list, current);
      }
    }

    child = child->next;
  }

  //  list = g_list_reverse(list);
  *thread = list;

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", list,
				   NULL));
}

xmlNode*
ags_file_write_thread_list(AgsFile *file, xmlNode *parent, GList *thread)
{
  AgsThread *current;
  xmlNode *node;
  GList *list;
  gchar *id;

  id = ags_id_generator_create_uuid();

  node = xmlNewNode(NULL,
		    "ags-thread-list\0");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", list,
				   NULL));

  xmlAddChild(parent,
	      node);

  //TODO:JK: generate id and add id ref

  list = thread;

  while(list != NULL){
    ags_file_write_thread(file, node, AGS_THREAD(list->data));

    list = list->next;
  }

  return(node);
}

void
ags_file_read_thread_pool(AgsFile *file, xmlNode *node, AgsThreadPool **thread_pool)
{
  AgsThreadPool *gobject;
  AgsFileLookup *file_lookup;
  xmlNode *child;
  xmlChar *prop, *content;

  if(*thread_pool == NULL){
    gobject = g_object_new(AGS_TYPE_THREAD_POOL,
			   NULL);
    *thread_pool = gobject;
  }else{
    gobject = *thread_pool;
  }

  //TODO:JK: implement me
  //  g_object_set(G_OBJECT(gobject),
  //	       "ags-main\0", file->ags_main,
  //	       NULL);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", xmlGetProp(node, AGS_FILE_ID_PROP)),
				   "reference\0", gobject,
				   NULL));

  gobject->flags = (guint) g_ascii_strtoull(xmlGetProp(node, AGS_FILE_FLAGS_PROP),
					    NULL,
					    16);
}

xmlNode*
ags_file_write_thread_pool(AgsFile *file, xmlNode *parent, AgsThreadPool *thread_pool)
{
  AgsFileLookup *file_lookup;
  xmlNode *node, *child;
  gchar *id;
  guint i;

  id = ags_id_generator_create_uuid();
  
  node = xmlNewNode(NULL,
		    "ags-thread_pool\0");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", thread_pool,
				   NULL));

  xmlNewProp(node,
	     AGS_FILE_FLAGS_PROP,
	     g_strdup_printf("%x\0", thread_pool->flags));
}

void
ags_file_read_thread_pool_start(AgsFileLaunch *file_launch, AgsThreadPool *thread_pool)
{
  ags_thread_pool_start(thread_pool);
}

void
ags_file_read_audio_loop(AgsFile *file, xmlNode *node, AgsAudioLoop *audio_loop)
{
  xmlNode *child;

  audio_loop = AGS_AUDIO_LOOP(audio_loop);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", xmlGetProp(node, AGS_FILE_ID_PROP)),
				   "reference\0", audio_loop,
				   NULL));
}

xmlNode*
ags_file_write_audio_loop(AgsFile *file, xmlNode *parent, AgsAudioLoop *audio_loop)
{
  xmlNode *node;
  gchar *id;

  id = ags_id_generator_create_uuid();

  node = xmlNewNode(NULL,
		    "ags-audio-loop\0");
  xmlNewProp(node,
	     AGS_FILE_ID_PROP,
	     id);

  ags_file_add_id_ref(file,
		      g_object_new(AGS_TYPE_FILE_ID_REF,
				   "main\0", file->ags_main,
				   "file\0", file,
				   "node\0", node,
				   "xpath\0", g_strdup_printf("xpath=//*[@id='%s']\0", id),
				   "reference\0", audio_loop,
				   NULL));

  xmlAddChild(parent,
	      node);
}
