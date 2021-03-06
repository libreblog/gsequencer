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

#include "config.h"

#include <glib.h>
#include <glib-object.h>

#include <gdk/gdk.h>
#include <pango/pangocairo.h>

#include <gtk/gtk.h>

#ifdef AGS_WITH_QUARTZ
#include <gtkosxapplication.h>
#endif

#include <ags/libags.h>

#ifdef AGS_WITH_LIBINSTPATCH
#include <libinstpatch/libinstpatch.h>
#endif

#include <libxml/parser.h>
#include <libxml/xlink.h>
#include <libxml/xpath.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlsave.h>

#define _GNU_SOURCE
#include <locale.h>

#include <stdlib.h>

#ifndef AGS_W32API
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <ags/X/ags_xorg_application_context.h>

#include "gsequencer_main.h"

#include <ags/i18n.h>

void* ags_setup_thread(void *ptr);
void ags_setup(int argc, char **argv);

extern AgsApplicationContext *ags_application_context;

void*
ags_setup_thread(void *ptr)
{
  AgsXorgApplicationContext *xorg_application_context;
  
  xorg_application_context = (AgsXorgApplicationContext *) ptr;

  while(g_atomic_int_get(&(xorg_application_context->gui_ready)) == 0){
    usleep(500000);
  }

  ags_application_context_setup(AGS_APPLICATION_CONTEXT(xorg_application_context));
  
  g_thread_exit(NULL);

  return(NULL);
}

void
ags_setup(int argc, char **argv)
{
  AgsApplicationContext *application_context;
  AgsLog *log;

  /* application context */
  application_context = 
    ags_application_context = (AgsApplicationContext *) ags_xorg_application_context_new();
  g_object_ref(application_context);
  
  application_context->argc = argc;
  application_context->argv = argv;

  log = ags_log_get_instance();

  ags_log_add_message(log,
		      "Welcome to Advanced Gtk+ Sequencer");
  
  /* application context */
  g_thread_new("Advanced Gtk+ Sequencer - setup",
	       ags_setup_thread,
	       application_context);
  
  ags_application_context_prepare(application_context);
}

int
main(int argc, char **argv)
{  
  GtkCssProvider *css_provider;
  
  AgsConfig *config;
  AgsPriority *priority;
  
  gchar *filename;
#if defined AGS_W32API
  gchar *app_dir;
  gchar *path;
#endif

  gboolean builtin_theme_disabled;
  guint i;

#ifdef AGS_WITH_RT
  struct sched_param param;
  struct rlimit rl;
#endif

#ifndef AGS_W32API
  struct passwd *pw;

  uid_t uid;
#endif
  
  gchar *wdir;
  gchar *config_filename;
  gchar *priority_filename;
  gchar *css_filename;
  gchar *str;
  
  gboolean has_file;
  int result;

#ifdef AGS_WITH_RT
  const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64 Mb
#endif
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  
  builtin_theme_disabled = FALSE;

  config = NULL;
  
  priority = ags_priority_get_instance();  
  ags_priority_load_defaults(priority);

//  mtrace();
  
#if defined (AGS_W32API)
  app_dir = NULL;

  if(strlen(argv[0]) > strlen("\\gsequencer.exe")){
    app_dir = g_strndup(argv[0],
			strlen(argv[0]) - strlen("\\gsequencer.exe"));
  }

  putenv(g_strdup_printf("XDG_DATA_DIRS=%s\\share", app_dir));
  putenv(g_strdup_printf("XDG_CONFIG_HOME=%s\\etc", app_dir));

  putenv(g_strdup_printf("GDK_PIXBUF_MODULE_FILE=%s\\lib\\gdk-pixbuf-2.0\\2.10.0\\loaders.cache", app_dir));

  putenv(g_strdup_printf("GTK_EXE_PREFIX=%s", app_dir));
  putenv(g_strdup_printf("GTK_DATA_PREFIX=%s\\share", app_dir));
  putenv(g_strdup_printf("GTK_PATH=%s", app_dir));
  putenv(g_strdup_printf("GTK_IM_MODULE_FILE=%s\\lib\\gtk-3.0\\3.0.0\\immodules.cache", app_dir));

  if(getenv("GTK_THEME") == NULL){
    putenv(g_strdup("GTK_THEME=BlueMenta"));
  }
#else
  uid = getuid();
  pw = getpwuid(uid);

  wdir = g_strdup_printf("%s/%s",
			 pw->pw_dir,
			 AGS_DEFAULT_DIRECTORY);
  
  priority_filename = g_strdup_printf("%s/priority.conf",
				      wdir);

  ags_priority_load_from_file(priority,
			      priority_filename);

  g_free(priority_filename);
  g_free(wdir);
#endif

  /* real-time setup */
#ifdef AGS_WITH_RT
  result = getrlimit(RLIMIT_STACK, &rl);

  /* set stack size 64M */
  if(result == 0){
    if(rl.rlim_cur < kStackSize){
      rl.rlim_cur = kStackSize;
      result = setrlimit(RLIMIT_STACK, &rl);

      if(result != 0){
	//TODO:JK
      }
    }
  }

  param.sched_priority = 15;

  str = ags_priority_get_value(priority,
			       AGS_PRIORITY_RT_THREAD,
			       AGS_PRIORITY_KEY_GUI_MAIN_LOOP);

  if(str != NULL){
    param.sched_priority = (int) g_ascii_strtoull(str,
						  NULL,
						  10);
  }

  if(str == NULL ||
     ((!g_ascii_strncasecmp(str,
			    "0",
			    2)) != TRUE)){
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
      perror("sched_setscheduler failed");
    }
  }

  g_free(str);
#endif

  /* parse command line parameter */
  filename = NULL;
  has_file = FALSE;
  
  for(i = 0; i < argc; i++){
    if(!strncmp(argv[i], "--help", 7)){
      printf("GSequencer is an audio sequencer and notation editor\n\n");

      printf("Usage:\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\n",
	     "Report bugs to <jkraehemann@gmail.com>\n",
	     "--filename file     open file",
	     "--no-builtin-theme  disable built-in theme",
	     "--help              display this help and exit",
	     "--version           output version information and exit");
      
      exit(0);
    }else if(!strncmp(argv[i], "--version", 10)){
      printf("GSequencer %s\n\n", AGS_VERSION);
      
      printf("%s\n%s\n%s\n\n",
	     "Copyright (C) 2005-2020 Joël Krähemann",
	     "This is free software; see the source for copying conditions.  There is NO",
	     "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");
      
      printf("Written by Joël Krähemann\n");

      exit(0);
    }else if(!strncmp(argv[i], "--no-builtin-theme", 19)){
      builtin_theme_disabled = TRUE;
    }else if(!strncmp(argv[i], "--filename", 11)){
      filename = argv[i + 1];
      i++;

      if(g_file_test(filename,
		     G_FILE_TEST_EXISTS) &&
	 g_file_test(filename,
		     G_FILE_TEST_IS_REGULAR)){
	has_file = TRUE;
      }
    }
  }

  css_filename = NULL;
  
#ifdef AGS_W32API
  if(!builtin_theme_disabled){
    app_dir = NULL;
    
    if(strlen(argv[0]) > strlen("\\gsequencer.exe")){
      app_dir = g_strndup(argv[0],
			  strlen(argv[0]) - strlen("\\gsequencer.exe"));
    }
    
    if((css_filename = getenv("AGS_CSS_FILENAME")) == NULL){
      css_filename = g_strdup_printf("%s\\share\\gsequencer\\styles\\ags.css",
				    g_get_current_dir());
    
      if(!g_file_test(css_filename,
		      G_FILE_TEST_IS_REGULAR)){
	g_free(css_filename);

	if(g_path_is_absolute(app_dir)){
	  css_filename = g_strdup_printf("%s\\%s",
					app_dir,
					"\\share\\gsequencer\\styles\\ags.css");
	}else{
	  css_filename = g_strdup_printf("%s\\%s\\%s",
					g_get_current_dir(),
					app_dir,
					"\\share\\gsequencer\\styles\\ags.css");
	}
      }
    }else{
      css_filename = g_strdup(css_filename);
    }
  
    g_free(app_dir);
  }
#else
  uid = getuid();
  pw = getpwuid(uid);
  
  /* parse rc file */
  if(!builtin_theme_disabled){
    css_filename = g_strdup_printf("%s/%s/ags.css",
				   pw->pw_dir,
				   AGS_DEFAULT_DIRECTORY);

    if(!g_file_test(css_filename,
		    G_FILE_TEST_IS_REGULAR)){
      g_free(css_filename);

#ifdef AGS_CSS_FILENAME
      css_filename = g_strdup(AGS_CSS_FILENAME);
#else
      if((css_filename = getenv("AGS_CSS_FILENAME")) == NULL){
	css_filename = g_strdup_printf("%s%s",
				       DESTDIR,
				       "/gsequencer/styles/ags.css");
      }else{
	css_filename = g_strdup(css_filename);
      }
#endif
    }
  }
#endif
  
  /**/
  LIBXML_TEST_VERSION;
  xmlInitParser();
  
  //ao_initialize();

  //  gdk_threads_enter();
  //  g_thread_init(NULL);
  gtk_init(&argc, &argv);

#ifdef AGS_WITH_QUARTZ
  g_object_new(GTKOSX_TYPE_APPLICATION,
	       NULL);
#endif
  
#ifdef AGS_WITH_LIBINSTPATCH
  ipatch_init();
#endif
  
#if 0
  g_log_set_fatal_mask("GLib",
  		       G_LOG_LEVEL_CRITICAL);

  g_log_set_fatal_mask("libInstPatch",
  		       G_LOG_LEVEL_CRITICAL);

  g_log_set_fatal_mask("GLib-GObject",
  		       G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);

  g_log_set_fatal_mask(NULL,
  		       G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);  

  g_log_set_fatal_mask("Gtk",
  		       G_LOG_LEVEL_CRITICAL);
#endif

  g_set_application_name(i18n("Advanced Gtk+ Sequencer"));
  gtk_window_set_default_icon_name("gsequencer");
  g_setenv("PULSE_PROP_media.role", "production", TRUE);  
  
  css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css_provider,
				  css_filename,
				  NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
					    GTK_STYLE_PROVIDER(css_provider),
					    GTK_STYLE_PROVIDER_PRIORITY_USER);    
    
  g_free(css_filename);

  /* setup */
  if(!has_file){
#ifdef AGS_W32API
  app_dir = NULL;

  if(strlen(argv[0]) > strlen("\\gsequencer.exe")){
    app_dir = g_strndup(argv[0],
			strlen(argv[0]) - strlen("\\gsequencer.exe"));
  }
  
  path = g_strdup_printf("%s\\etc\\gsequencer",
			 g_get_current_dir());
    
  if(!g_file_test(path,
		  G_FILE_TEST_IS_DIR)){
    g_free(path);

    if(g_path_is_absolute(app_dir)){
      path = g_strdup_printf("%s\\%s",
			     app_dir,
			     "\\etc\\gsequencer");
    }else{
      path = g_strdup_printf("%s\\%s\\%s",
			     g_get_current_dir(),
			     app_dir,
			     "\\etc\\gsequencer");
    }
  }
    
  config_filename = g_strdup_printf("%s\\%s",
				    path,
				    AGS_DEFAULT_CONFIG);

  g_free(path);
#else
  wdir = g_strdup_printf("%s/%s",
			 pw->pw_dir,
			 AGS_DEFAULT_DIRECTORY);
    
  config_filename = g_strdup_printf("%s/%s",
				    wdir,
				    AGS_DEFAULT_CONFIG);

  g_free(wdir);
#endif

    config = ags_config_get_instance();

    ags_config_load_from_file(config,
			      config_filename);

    g_free(config_filename);
  }

  /* some GUI scaling */
  if(!builtin_theme_disabled &&
     !has_file){
//    ags_xorg_application_context_load_gui_scale(ags_application_context_get_instance());
  }

  ags_setup(argc, argv);
    
  //  muntrace();

  return(0);
}
