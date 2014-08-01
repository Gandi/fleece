#ifndef FLEECE_H__
# define FLEECE_H__

#define BUFFERSIZE 16384

/* a hostname could be greater than the 63 of original dns ? counter possible bof */
#define HOSTNAME_MAXSZ 100
/* that is received line from apache */
#define LINE_MAXSZ 1024
/* it is ipv4 now(so ~16), but we could got ipv6 and in case.. */
#define IP_MAXSZ 100
/* syslog priority level */
#define syslog_facility LOG_USER
#define syslog_priority (LOG_USER|LOG_INFO)
/* in case the json was rejected, content will be inside this name, presently it is 'message' */
#define REJECTED_JSON "nojson"

struct option_doc {
    const char *name;
    int         has_arg;
    int         val;
    const char *documentation;
};

struct kv {
    char *key;
    size_t key_len;
    char *value;
    size_t value_len;
}; /* struct kv */

struct fleece_options {
    int argc;
    char **argv;
    char *host;
    char ip[IP_MAXSZ];
    unsigned short port;
    size_t window_size;
    struct kv *extra_fields;
    size_t extra_fields_len;
    bool quietmode;
    char *rsyslog;
    unsigned short rsyslog_port;
    bool syslog;
};

typedef struct fleece_options fleece_options_t;

#endif
