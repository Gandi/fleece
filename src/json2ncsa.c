/* for json to ncsa like part of fleece, to be incorporated

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

#include <stdio.h>

// just for exit..
#include <stdlib.h>

// for perror and errno
#include <errno.h>

// for select things
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// libjansson !
#include <jansson.h>

// for ioctl stdin ensure no blocking
#include <sys/ioctl.h>

// inQueue()
#define DELAY 0
#define DELAYS 1

// max json input string
#define STRMAXSZ 2048
// max integer to string, from json input
#define INTMAXSZ 16

// type of output we want against fields
typedef enum type_encaps {
    ENCAPS_BEGIN,
    ENCAPS_END,
    ENCAPS_IT,
    COMBINED,
    NOTHING,
};

// cosmetics we want for formatted output of fields
#define COMBINED_JOIN ':'
#define COMBINED_DEFAULT ' '
#define ENCAPS_CHAR '"'

// in case we don't get a required value in the json, we output this default string
#define UNDEF_VAL "-"

// fields struct rules
struct entity {
    char *name;
    int attribut;
};
/* as some field have to be reconstructed, and encapsulated between ",
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
    { NULL, NULL }
};

// return printed value, according to above rules
int printformatted(char *out, size_t maxlen, int attribut, const char *value) {
    int writtenchars;

    switch (attribut) {
        case COMBINED:
            writtenchars = snprintf(out, maxlen, "%s%c", value, COMBINED_JOIN);
            break;
        case ENCAPS_BEGIN:
            writtenchars = snprintf(out, maxlen, "%c%s%c", ENCAPS_CHAR, value, COMBINED_DEFAULT);
            break;
        case ENCAPS_END:
            writtenchars = snprintf(out, maxlen, "%s%c%c", value, ENCAPS_CHAR, COMBINED_DEFAULT);
            break;
        case ENCAPS_IT:
            writtenchars = snprintf(out, maxlen, "%c%s%c%c", ENCAPS_CHAR, value, ENCAPS_CHAR, COMBINED_DEFAULT);
            break;
        case NOTHING:
            writtenchars = snprintf(out, maxlen, "%s%c", value, COMBINED_DEFAULT);
            break;
        default:
            writtenchars = 0;
            fprintf(stderr, "unsupported type for %s\n", value);
            break;
    }
    return writtenchars;
}

// take one json and output an ncsa like line
int trimjson(char *jsoninline) {
    int idx, szwrite, tmp;
    char *ptr, *value;
    json_t *jsonevent, *jsondata;
    json_error_t jsonerror;
    char ncsaline[STRMAXSZ], intbuffer[INTMAXSZ];

	// load the supposed json line
    jsonevent = json_loads(jsoninline, 0, &jsonerror);
    if (!jsonevent) {
        fprintf(stderr, "aouch no json\n");
        return 1;
    }

	// ensure we got a json object, not other type
    if (!json_is_object(jsonevent)) {
        fprintf(stderr, "not tha type: %08X\n", json_typeof(jsonevent));
        free(jsonevent);
        return 1;
    }

	// iterate through the fields list contained in entities struct, in order
    idx = 0;
    szwrite = 0;
    ptr = ncsaline;
    do {
        jsondata = json_object_get(jsonevent, entities[idx].name);
        if ( jsondata != NULL ) {
			// try to get a string value from json
            value = json_string_value(jsondata);
            if ( value != NULL ) {
                szwrite = printformatted(ptr, STRMAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, value);
                free(value);
            } else {
				// it is possible that it is instead an integer from json, so we get it
				tmp = json_integer_value(jsondata);
				// json_integer_value will return 0 in case it fails, but then, what should i do?
				// 0 value could be a good value in some case in our logs
                snprintf(intbuffer, INTMAXSZ - 1, "%d", tmp);
                szwrite = printformatted(ptr, STRMAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, intbuffer);
            }
            ptr += szwrite;
            free(jsondata);
        } else {
            szwrite = printformatted(ptr, STRMAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, UNDEF_VAL);
            ptr += szwrite;
        }
    } while (entities[++idx].name != NULL);
    free(jsonevent);

    *ptr++ = '\n';
    *ptr = 0;
    fprintf(stdout, "%s", ncsaline);

    return 0;
}

// test if data is in queue waiting to be read
int inQueue(int fd) {
    int fds;
    fd_set read;
    struct timeval tv={DELAYS, DELAY};

    FD_ZERO(&read);
    FD_SET(fd, &read);
    fds = fd+1;
    select(fds, &read, NULL, NULL, &tv);
    if (FD_ISSET(fd, &read)) {
        return 1;
    }
    return 0;
}

// loop on stdin, that will be removed, it is fleece core part job
int main(int ac, char **av) {
    int errorcond;
    FILE *influx;
    char buffer[STRMAXSZ];

    while ( 1 ) {
        if (inQueue(fileno(stdin))) {
            errorcond=fgets(buffer, STRMAXSZ, stdin);
            if ( errorcond != NULL ) {
                if (trimjson(buffer)) {
                    fprintf(stdout, "bad json: %s\n", buffer);
                }
            } else {
                perror("fgets got:");
                break;
            }
        }
    }

  return 0;
}

