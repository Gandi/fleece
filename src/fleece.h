#define BUFFERSIZE 16384
#define FLEECE_VERSION "0.2"

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

typedef enum {
    opt_help = 'h',
    opt_version = 'v',
    opt_field,
    opt_host,
    opt_quiet,
    opt_port,
    opt_window_size,
    opt_rsyslog,
    opt_rsyslog_port,
    opt_syslog
} optlist_t;

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

static struct option_doc options[] = {
    { "help", no_argument, opt_help, "show this help" },
    { "version", no_argument, opt_version, "show the version of fleece" },
    { "field", required_argument, opt_field,
      "Add a custom key-value mapping to every line emitted" },
    { "host", required_argument, opt_host,
      "The hostname to send udp messages to" },
    { "port", required_argument, opt_port,
      "The port to connect on the udp server" },
    { "quiet", no_argument, opt_quiet,
      "Disable outputs" },
    { "window_size", optional_argument, opt_window_size,
      "The window size" },
    { "syslog-host", optional_argument, opt_rsyslog,
      "The remote syslog that will receive ncsa" },
    { "syslog-port", optional_argument, opt_rsyslog_port,
      "The port used to send to the ncsa syslog" },
    { "local-syslog", no_argument, opt_syslog,
      "Log into the local syslog" },
    { NULL, 0, 0, NULL },
};

extern char *strdup(const char *s);
extern char *strsep(char **stringp, const char *delim);
extern void usage(const char *prog);

void usage(const char *prog) {
    printf("Usage: %s [options]]\n", prog);

    for (int i = 0; options[i].name != NULL; i++) {
        printf("  --%s%s %.*s %s\n", options[i].name,
            options[i].has_arg ? " VALUE" : "",
            (int)(20 - strlen(options[i].name) - (options[i].has_arg ? 6 : 0)),
            "                                   ",
            options[i].documentation);
    }
} /* usage */

struct option *build_getopt_options() {
    /* variables */
    int i;
    struct option *getopt_options = NULL;

    /* convert the 'option_doc' array into a 'struct option' array
     * for use with getopot_long_only */
    for (i = 0; options[i].name != NULL; i++) {
        getopt_options = realloc(getopt_options, (i+1) * sizeof(struct option));
        getopt_options[i].name = options[i].name;
        getopt_options[i].has_arg = options[i].has_arg;
        getopt_options[i].flag = NULL;
        getopt_options[i].val = options[i].val;
    }
    /* Add one last item for the list terminator NULL */
    getopt_options = realloc(getopt_options, (i+1) * sizeof(struct option));
    getopt_options[i].name = NULL;

    return getopt_options;
} /* build_getopt_options */


void *configure_fleece_from_cli(fleece_options_t *flconf, struct option **getopt_options) {
    int c,i;
    char *tmp;

    while (i = -1,
    c = getopt_long_only(flconf->argc, flconf->argv, "+hv", *getopt_options, &i), c != -1) {
      switch (c) {
        case opt_version:
          printf("Fleece version %s\n",FLEECE_VERSION);
          return 0;
        case opt_help:
          usage(flconf->argv[0]);
          return 0;
        case opt_host:
          flconf->host = strdup(optarg);
          break;
        case opt_port:
          flconf->port = (unsigned short)atoi(optarg);
          break;
        case opt_window_size:
          flconf->window_size = (size_t)atoi(optarg);
          break;
        case opt_quiet:
          flconf->quietmode = true;
          break;
        case opt_field:
          tmp = strchr(optarg, '=');
          if (tmp == NULL) {
            printf("Invalid --field : expected 'foo=bar' form " \
                   "didn't find '=' in '%s'", optarg);
            usage(flconf->argv[0]);
            exit(1);
          }
          flconf->extra_fields_len += 1;
          flconf->extra_fields = realloc(flconf->extra_fields,
                                 flconf->extra_fields_len * sizeof(struct kv));
          *tmp = '\0'; /* turn '=' into null terminator */
          tmp++; /* skip to first char of value */
          flconf->extra_fields[flconf->extra_fields_len - 1].key = strdup(optarg);
          flconf->extra_fields[flconf->extra_fields_len - 1].key_len = strlen(optarg);
          flconf->extra_fields[flconf->extra_fields_len - 1].value = strdup(tmp);
          flconf->extra_fields[flconf->extra_fields_len - 1].value_len = strlen(tmp);
          break;
        case opt_rsyslog:
          flconf->rsyslog = strdup(optarg);
          break;
        case opt_rsyslog_port:
          flconf->rsyslog_port = (unsigned short)atoi(optarg);
          break;
        case opt_syslog:
          flconf->syslog = true;
          break;
        default:
          printf("Not handled\n");
          usage(flconf->argv[0]);
          return NULL;
      }
    }
    free(*getopt_options);

    if (flconf->host == NULL) {
      printf("Missing --host flag\n");
      usage(flconf->argv[0]);
      return NULL;
    }

    if (flconf->port == 0) {
      printf("Missing --port flag\n");
      usage(flconf->argv[0]);
      return NULL;
    }

    if (flconf->rsyslog != NULL ) {
        if (flconf->rsyslog_port == 0 ) {
            printf("Missing --syslog-port flag\n");
            usage(flconf->argv[0]);
            return NULL;
        }
    }

    flconf->argc -= optind;
    flconf->argv += optind;

    return flconf;
}
