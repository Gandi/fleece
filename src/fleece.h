#define BUFFERSIZE 16384
#define FLEECE_VERSION "0.1"

typedef enum {
    opt_help = 'h',
    opt_version = 'v',
    opt_field,
    opt_host,
    opt_quiet,
    opt_port,
    opt_window_size,
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
    char *host;
    char ip[100];
    unsigned short port;
    size_t window_size;
    struct kv *extra_fields;
    size_t extra_fields_len;
    bool quietmode;
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

