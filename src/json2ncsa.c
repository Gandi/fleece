/* for json to ncsa alike part of fleece

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

#include <stdbool.h>
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

#include <jansson.h>

#include "fleece.h"
#include "json2ncsa.h"

/* header fields of json */
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
int jsonncsa(json_t *jsonevent, char *ncsaline, int line_size) {
    int idx, szwrite = 0;
    long long tmp;
    char *ptr;
    const char *value;
    json_t *jsondata;
    char intbuffer[INTMAXSZ];

    ptr = ncsaline;
    /* if we receive a rejected json, it will not have to be formatted, but directly sent */
    jsondata = json_object_get(jsonevent, REJECTED_JSON);
    if ( jsondata != NULL ) {
        value = json_string_value(jsondata);
        if ( value != NULL ) {
            szwrite = snprintf(ptr, line_size, "%s", value);
            return szwrite < 0;
        }
        return -1;
    }
    /* iterate through the fields list contained in entities struct, in order */
    idx = 0;
    do {
        jsondata = json_object_get(jsonevent, entities[idx].name);
        if ( jsondata != NULL ) {
            json_incref(jsondata);
            value = json_string_value(jsondata);
            if ( value != NULL ) {
                szwrite = printformatted(ptr, line_size - (ptr - ncsaline), \
                  entities[idx].attribut, value);
            } else {
                /* it is possible that it is instead, an integer from json, so we get it */
                tmp = json_integer_value(jsondata);
                /* json_integer_value will return 0 in case it fails, but then, what should i do?
                   0 value could be a good value in some case in our logs, so there is no fail */
                snprintf(intbuffer, INTMAXSZ, "%lld", tmp);
                szwrite = printformatted(ptr, line_size - (ptr - ncsaline), \
                  entities[idx].attribut, intbuffer);
            }
            json_decref(jsondata);
        } else {
            szwrite = printformatted(ptr, line_size - (ptr - ncsaline), \
              entities[idx].attribut, UNDEF_VAL);
        }
        if ( szwrite < 0 ) {
            /* something want really bad, snprintf erro */
            fprintf(stderr, "something went wrong with snprintf\n");
            break;
        }
        /* there we have a possible b0f, should ensure we really got that written, and not all
          we wanna (cf. 'Return value' of snprintf manpage), verify that */
        if ( ptr >= (ncsaline + line_size) ) {
            fprintf(stderr, "received a too long json\n");
            break;
        }
        ptr += szwrite;
    } while (entities[++idx].name != NULL);

    return 0;
}

#ifdef STANDALONE

/* in case the json is rejected, the input will be prefixed by this format */
#define BAD_JSON "wrong json:%s"

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
`
/* quickly handle one json and return a json_t */
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

/* loop on stdin, just for testing, it is fleece cores' job :) */
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
                    fprintf(stdout, BAD_JSON, buffer);
                    continue;
                }
                jsonncsa(jsonevent, ncsaline);
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
