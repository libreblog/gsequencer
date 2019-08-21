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

#include <ags/audio/ags_fourier_transform_util.h>

#include <stdlib.h>

#include <math.h>

/**
 * ags_fourier_transform_util_compute_stft_s8:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_s8(gint8 *buffer, guint channels,
					   guint buffer_length,
					   guint samplerate,
					   gdouble **retval)
{
  complex m_e;
  guint i;

  static const gdouble normalize_divisor = exp2(7.0);

  if(buffer == NULL ||
     retval == NULL){
    return;
  }

  if(retval[0] == NULL){
    retval[0] = (AgsComplex *) malloc(buffer_length * sizeof(AgsComplex));
  }

  m_e = M_E + I * 0.0;

  for(i = 0; i < buffer_length; i++){
    complex z;
    gdouble h;
    gdouble k;
    gdouble r;

    k = (gdouble) i;
    r = (gdouble) i;

    h = AGS_FOURIER_TRANSFORM_UTIL_ANALYSIS_WINDOW(i - r);
    
    z = ((gdouble) buffer[i] / normalize_divisor) * h * cpow(m_e, -1.0 * I * 2.0 * M_PI * k * r / buffer_length);

    ags_complex_set(retval[0] + i, z);
  }
}

/**
 * ags_fourier_transform_util_compute_stft_s16:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_s16(gint16 *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_compute_stft_s24:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_s24(gint32 *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_compute_stft_s32:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_s32(gint32 *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_compute_stft_s64:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_s64(gint64 *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_compute_stft_float:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_float(gfloat *buffer, guint channels,
					      guint buffer_length,
					      guint samplerate,
					      gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_compute_stft_double:
 * @buffer: the audio buffer
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_compute_stft_double(gdouble *buffer, guint channels,
					       guint buffer_length,
					       guint samplerate,
					       gdouble **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_s8:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_s8(AgsComplex *buffer, guint channels,
					   guint buffer_length,
					   guint samplerate,
					   gint8 **retval)
{
  complex m_e;
  guint i;

  static const gdouble scale = 127.0;


  if(buffer == NULL ||
     retval == NULL){
    return;
  }

  if(retval[0] == NULL){
    retval[0] = (gint8 *) malloc(buffer_length * sizeof(gint8));
  }
  
  m_e = M_E + I * 0.0;

  for(i = 0; i < buffer_length; i++){
    complex z;
    gdouble k;
    gdouble y;

    z = ags_complex_get(buffer[i]);

    k = (gdouble) i;

    y = (1.0 / buffer_length) * (z * cpow(m_e, I * 2.0 * M_PI * k * i / buffer_length));

    retval[0][i] = scale * y;
  }
}

/**
 * ags_fourier_transform_util_inverse_stft_s16:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_s16(AgsComplex *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gint16 **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_s24:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_s24(AgsComplex *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gint32 **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_s32:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_s32(AgsComplex *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gint32 **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_s64:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_s64(AgsComplex *buffer, guint channels,
					    guint buffer_length,
					    guint samplerate,
					    gint64 **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_float:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_float(AgsComplex *buffer, guint channels,
					      guint buffer_length,
					      guint samplerate,
					      gfloat **retval)
{
  //TODO:JK: implement me
}

/**
 * ags_fourier_transform_util_inverse_stft_double:
 * @buffer: the complex data
 * @channels: number of audio channels
 * @buffer_length: the buffer's length
 * @samplerate: the samplerate
 * @retval: the return location of result
 * 
 * Compute inverse fourier transform for @buffer.
 * 
 * Since: 2.3.0
 */
void
ags_fourier_transform_util_inverse_stft_double(AgsComplex *buffer, guint channels,
					       guint buffer_length,
					       guint samplerate,
					       gdouble **retval)
{
  //TODO:JK: implement me
}
