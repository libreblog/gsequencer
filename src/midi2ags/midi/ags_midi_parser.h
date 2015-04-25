/* AGS - Advanced GTK Sequencer
 * Copyright (C) 2015 Joël Krähemann
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

#ifndef __AGS_MIDI_PARSER_H__
#define __AGS_MIDI_PARSER_H__

#include <glib.h>
#include <glib-object.h>

#define AGS_TYPE_MIDI_PARSER                (ags_midi_parser_get_type ())
#define AGS_MIDI_PARSER(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), AGS_TYPE_MIDI_PARSER, AgsMidiParser))
#define AGS_MIDI_PARSER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST((class), AGS_TYPE_MIDI_PARSER, AgsMidiParserClass))
#define AGS_IS_MIDI_PARSER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_MIDI_PARSER))
#define AGS_IS_MIDI_PARSER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_MIDI_PARSER))
#define AGS_MIDI_PARSER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_MIDI_PARSER, AgsMidiParserClass))

#define AGS_MIDI_PARSER_MTHD "MThd\0"
#define AGS_MIDI_PARSER_MTCK "MTck\0"

typedef struct _AgsMidiParser AgsMidiParser;
typedef struct _AgsMidiParserClass AgsMidiParserClass;

typedef enum{
  AGS_MIDI_CHUNK_HEADER   = 1,
  AGS_MIDI_CHUNK_TRACK    = 1 << 1,
  AGS_MIDI_CHUNK_UNKNOWN  = 1 << 2,
}AgsMidiChunkFlags;

struct _AgsMidiParser
{
  GObject gobject;

  int fd;
  guint nth_chunk;
};

struct _AgsMidiParserClass
{
  GObjectClass gobject;
};

GType ags_midi_parser_get_type(void);

char* ags_midi_parser_read_chunk(AgsMidiParser *midi_parser,
				 guint *message_type, guint *message_length,
				 GError **error);
void ags_midi_parser_write_chunk(AgsMidiParser *midi_parser,
				 char *chunk, size_t length);
void ags_midi_parser_seek(AgsMidiParser *midi_parser, guint n_chunks, gint whence);
void ags_midi_parser_flush(AgsMidiParser *midi_parser);

xmlNode* ags_midi_parser_parse_header(AgsMidiParser *parser,
				      char *chunk_data,
				      guint chunk_type,
				      guint chunk_length);
xmlNode* ags_midi_parser_parse_track(AgsMidiParser *parser,
				     char *chunk_data,
				     guint chunk_type,
				     guint chunk_length);

AgsMidiParser* ags_midi_parser_new(int fd);

#endif /*__AGS_MIDI_PARSER_H__*/
