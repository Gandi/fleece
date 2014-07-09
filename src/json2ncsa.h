/* for json to ncsa like part of fleece

   Copyright (C) 2014  Kilian Hart <eldre@gandi.net> <kilian@afturgurluk.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <jansson.h>

extern int printformatted(char *out, size_t maxlen, int attribut, const char *value);
extern int trimjson(json_t *, char *ncsaline);

#ifdef STANDALONE
/* inQueue() */
#define DELAY 0
#define DELAYS 1
#endif

/* max json input string */
#ifndef LINE_MAXSZ
#define LINE_MAXSZ 2048
#endif
/* max integer (or long long) to string, expected from json input */
#define INTMAXSZ 32

/* type of output we want against fields */
typedef enum {
    ENCAPS_BEGIN,
    ENCAPS_END,
    ENCAPS_IT,
    COMBINED,
    NOTHING,
} type_encaps;

/* cosmetics we want for formatted output of fields */
#define COMBINED_JOIN ':'
#define COMBINED_DEFAULT ' '
#define ENCAPS_CHAR '"'

/* in case we don't get a required value in the json, we output this default string */
#define UNDEF_VAL "-"

/* fields struct rules */
struct entity {
    char *name;
    int attribut;
};
/* as some fields have to be reconstructed, and encapsulated between ",
 * i.e. ENCAPS_BEGIN rule will match until an ENCAPS_END, and will use the defined ENCAPS_CHAR
 * but on contrary, ENCAPS_IT will just encapsulate just one field.
 * Also, COMBINED is to be used to reflect the Apache log vhost_combined, it is the separator that
 * is replaced with the previously defined COMBINED_JOIN so.
 */
struct entity entities[] = {
    { "vhost", COMBINED },
    { "clientip", NOTHING },
    { "role", NOTHING },
    { "@timestamp", ENCAPS_IT },
    { "method", ENCAPS_BEGIN },
    { "message", NOTHING },
    { "httpversion", ENCAPS_END },
    { "status", NOTHING },
    { "bytes", NOTHING },
    { "referer", ENCAPS_IT },
    { "useragent", ENCAPS_IT },
    { "duration", NOTHING },
    { NULL, 0 }
};


