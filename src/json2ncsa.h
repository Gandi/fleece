#ifndef LINE_MAXSZ
/* max json input string */
#define LINE_MAXSZ 2048
#endif
/* max integer (or long long) to string, expected from json input */
#define INTMAXSZ 32

#ifdef STANDALONE
/* inQueue() */
#define DELAY 0
#define DELAYS 1
#endif

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

/* in case we don't get a required value in the json, we output this default string, use "-" */
#define UNDEF_VAL "uNdeFiNeD"

/* in case the json is rejected, the input will be prefixed by this format */
#define BAD_JSON "wrong json:%s"
#ifdef STANDALONE
/* or with the following param name of fleece */
#define REJECTED_JSON "nojson"
#endif

#include <jansson.h>

extern int printformatted(char *out, size_t maxlen, int attribut, const char *value);
extern int jsonncsa(json_t *jsonevent, char *ncsaline);

