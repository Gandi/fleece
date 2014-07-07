/* poc for json to ncsa like
 *
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

#define STRMAXSZ 2048
#define INTMAXSZ 16

//char *colorder[] = {"host", "login", "user", "time", "request", "result", "size", NULL};
// tous les champs ne sont pas à prendre, car la version json est plus détaillé que ncsa-like
// il faut juste s'assurer de prendre le même ordre
// aussi, comme on doit pouvoir gérer les logs d'erreurs, là il faudra que je gère
// plusieurs champs à prendre, en fonction de 'environment', 'role', ou 'tags'
// définition d'un champ 'noop' qui vaut '-', en ce moment c'est "role" que je prends pour user
// bon pour la requête il me manque le type si c'est http/1.0,1.1 voir 0.9

typedef enum type_encaps {
    ENCAPS_BEGIN,
    ENCAPS_END,
    ENCAPS_IT,
    COMBINED,
    NOTHING,
};

#define COMBINED_JOIN ':'
#define COMBINED_DEFAULT ' '
#define ENCAPS_CHAR '"'
#define UNDEF_VAL "-"

struct entity {
    char *name;
    int attribut;
};
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

int trimjson(char *jsoninline) {
  int idx, szwrite;
  char *ptr, *value;
  json_t *jsonevent, *jsondata;
  json_error_t jsonerror;
  char ncsaline[STRMAXSZ], intbuffer[INTMAXSZ];

  jsonevent = json_loads(jsoninline, 0, &jsonerror);
  if (!jsonevent) {
    fprintf(stderr, "aouch no json\n");
    return 1;
  }

  if (!json_is_object(jsonevent)) {
    fprintf(stderr, "not tha type: %08X\n", json_typeof(jsonevent));
    free(jsonevent);
    return 1;
  }

  idx = 0;
  szwrite = 0;
  ptr = ncsaline;
  do {
    jsondata = json_object_get(jsonevent, entities[idx].name);
    if ( jsondata != NULL ) {
        value = json_string_value(jsondata);
        if ( value != NULL ) {
            szwrite = printformatted(ptr, STRMAXSZ - 1 - (ptr - ncsaline), entities[idx].attribut, value);
            free(value);
        } else {
			snprintf(intbuffer, INTMAXSZ - 1, "%d", json_integer_value(jsondata));
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

int inQueue(int fd) {
  int fds;
  fd_set read;
  struct timeval tv={DELAYS, DELAY};

  FD_ZERO(&read);
  FD_SET(fd, &read);
  fds=fd+1;
  select(fds, &read, NULL, NULL, &tv);
  if (FD_ISSET(fd, &read)) {
    return 1;
  }
  return 0;
}

int main(int ac, char **av) {
  int errorcond;
  FILE *influx;
  char buffer[STRMAXSZ];

  // set explicit noblock on stdin
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
