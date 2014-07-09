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

#include <stdio.h>
/* just for exit.. */
#include <stdlib.h>
/* for perror and errno */
#include <errno.h>
/* for select things */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
/* for ioctl stdin ensure no blocking */
#include <sys/ioctl.h>

#include "json2ncsa.h"

#ifdef STANDALONE
/* quickly handle a complete json and return a json_t */
json_t *readjson(const char *jsoninline) {
    json_t *jsonevent;
    json_error_t jsonerror;

    /* load the supposed json line */
    jsonevent = json_loads(jsoninline, 0, &jsonerror);
    if (!jsonevent) {
        fprintf(stderr, "this is no json\n");
        return NULL;
    }

    /* ensure we got a json object, not other type */
    if (!json_is_object(jsonevent)) {
        fprintf(stderr, "expected different json type than: 0x%08X\n", json_typeof(jsonevent));
        free(jsonevent);
        return NULL;
    }
    return jsonevent;
}
#endif

/* return printed value, according to entities structs' rules */
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

/* take one json_t and output an ncsa like line */
int trimjson(json_t *jsonevent, char *ncsaline) {
    int idx, szwrite;
    long long tmp;
    char *ptr;
    const char *value;
    json_t *jsondata;
    char intbuffer[INTMAXSZ];

    /* iterate through the fields list contained in entities struct, in order */
    idx = 0;
    szwrite = 0;
    ptr = ncsaline;
    do {
        jsondata = json_object_get(jsonevent, entities[idx].name);
        if ( jsondata != NULL ) {
            value = json_string_value(jsondata);
            if ( value != NULL ) {
                szwrite = printformatted(ptr, LINE_MAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, value);
            } else {
                /* it is possible that it is instead an integer from json, so we get it */
                tmp = json_integer_value(jsondata);
                // json_integer_value will return 0 in case it fails, but then, what should i do?
                // 0 value could be a good value in some case in our logs
                snprintf(intbuffer, INTMAXSZ - 1, "%lld", tmp);
                szwrite = printformatted(ptr, LINE_MAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, intbuffer);
            }
            ptr += szwrite;
        } else {
            szwrite = printformatted(ptr, LINE_MAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, UNDEF_VAL);
            ptr += szwrite;
        }
    } while (entities[++idx].name != NULL);

    *ptr++ = 0;

    return 0;
}

#ifdef STANDALONE
/* test if data is in queue waiting to be read */
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

/* loop on stdin, that will be removed, it is fleece core part job */
int main(void) {
    char *errorcond;
    char buffer[LINE_MAXSZ];
    json_t *jsonevent;
    char ncsaline[LINE_MAXSZ];

    while ( 1 ) {
        if ( inQueue(fileno(stdin)) ) {
            errorcond=fgets(buffer, LINE_MAXSZ, stdin);
            if ( errorcond != NULL ) {
                jsonevent = readjson(buffer);
                if ( jsonevent == NULL ) {
                    fprintf(stdout, "bad json: %s\n", buffer);
                    continue;
                }
                trimjson(jsonevent, ncsaline);
                fprintf(stdout, "%s\n", ncsaline);
				json_decref(jsonevent);
            } else {
                perror("fgets got:");
                break;
            }
        }
    }

    return 0;
}
#endif
