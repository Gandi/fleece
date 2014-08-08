#ifndef JSONNCSA_H__
#define JSONNCSA_H__

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
#define UNDEF_VAL "undefined"

int jsonncsa(json_t * jsonevent, char *ncsaline, int line_size);

#endif
