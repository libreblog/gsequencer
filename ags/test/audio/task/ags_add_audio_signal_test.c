/* GSequencer - Advanced GTK Sequencer
 * Copyright (C) 2005-2018 Joël Krähemann
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

#include <glib.h>
#include <glib-object.h>

#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include <ags/libags.h>
#include <ags/libags-audio.h>

#include <math.h>

int ags_add_audio_signal_test_init_suite();
int ags_add_audio_signal_test_clean_suite();

void ags_add_audio_signal_test_launch_add_audio_signal_callback();

void ags_add_audio_signal_test_launch();

gboolean ags_add_audio_signal_test_launch_add_audio_signal_invoked = FALSE;

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int
ags_add_audio_signal_test_init_suite()
{ 
  return(0);
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int
ags_add_audio_signal_test_clean_suite()
{  
  return(0);
}

void
ags_add_audio_signal_test_launch_add_audio_signal_callback()
{
  ags_add_audio_signal_test_launch_add_audio_signal_invoked = TRUE;
}

void
ags_add_audio_signal_test_launch()
{
  AgsDevout *devout;
  AgsRecycling *recycling;
  AgsAudioSignal *audio_signal;
  AgsRecallID *recall_id;

  AgsAddAudioSignal *add_audio_signal;
  
  /* create soundcard */
  devout = g_object_new(AGS_TYPE_DEVOUT,
			NULL);
  g_object_ref(devout);

  /* create recycling */
  recycling = ags_recycling_new(NULL);
  g_object_ref(recycling);

  /* create recall id */
  recall_id = ags_recall_id_new();
  g_object_ref(recall_id);

  /* create audio signal */
  audio_signal = ags_audio_signal_new(devout,
				      recycling,
				      recall_id);

  /* create add audio signal */
  add_audio_signal = ags_add_audio_signal_new(recycling,
					      audio_signal,
					      devout,
					      recall_id,
					      0);

  CU_ASSERT(AGS_IS_ADD_AUDIO_SIGNAL(add_audio_signal));
  CU_ASSERT(add_audio_signal->recycling == recycling);
  CU_ASSERT(add_audio_signal->audio_signal == audio_signal);
  CU_ASSERT(add_audio_signal->soundcard == devout);
  CU_ASSERT(add_audio_signal->recall_id == recall_id);
  
  /* launch */
  g_signal_connect(recycling, "add-audio-signal",
		   G_CALLBACK(ags_add_audio_signal_test_launch_add_audio_signal_callback), NULL);
  
  ags_task_launch(add_audio_signal);

  CU_ASSERT(ags_add_audio_signal_test_launch_add_audio_signal_invoked == TRUE);
  CU_ASSERT(g_list_find(recycling->audio_signal, audio_signal) != NULL);
}

int
main(int argc, char **argv)
{
  CU_pSuite pSuite = NULL;

  putenv("LC_ALL=C");
  putenv("LANG=C");
  
  /* initialize the CUnit test registry */
  if(CUE_SUCCESS != CU_initialize_registry()){
    return CU_get_error();
  }

  /* add a suite to the registry */
  pSuite = CU_add_suite("AgsAddAudioSignalTest", ags_add_audio_signal_test_init_suite, ags_add_audio_signal_test_clean_suite);
  
  if(pSuite == NULL){
    CU_cleanup_registry();
    
    return CU_get_error();
  }

  /* add the tests to the suite */
  if((CU_add_test(pSuite, "test of AgsAddAudioSignal launch", ags_add_audio_signal_test_launch) == NULL)){
    CU_cleanup_registry();
    
    return CU_get_error();
  }
  
  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  
  CU_cleanup_registry();
  
  return(CU_get_error());
}
