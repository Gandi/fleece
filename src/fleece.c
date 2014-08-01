/* This is Fleece a simple stdin to jsonified UDP stream software

   Copyright (C) 2013  Aurelien Rougemont <beorn@gandi.net> <beorn@binaries.fr>

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

/*
   fleece is heavily inspired of lumberjack
     https://github.com/jordansissel/lumberjack/tree/old/lumberjack-c
   By design lumberjacks can (and is) deadlocking apache. Because of this
   i have had to write something equivalent but with udp instead of 0mq
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <getopt.h>

#include <string.h>
#include <strings.h>

#include <jansson.h>

#include <syslog.h>

#include "fleece.h"
#include "hostnameip.h"
#include "json2ncsa.h"

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


static struct option_doc options[] = {
    {"help", no_argument, opt_help, "show this help"},
    {"version", no_argument, opt_version, "show the version of fleece"},
    {"field", required_argument, opt_field,
     "Add a custom key-value mapping to every line emitted"},
    {"host", required_argument, opt_host,
     "The hostname to send udp messages to"},
    {"port", required_argument, opt_port,
     "The port to connect on the udp server"},
    {"quiet", no_argument, opt_quiet,
     "Disable outputs"},
    {"window_size", required_argument, opt_window_size,
     "The window size"},
    {"syslog-host", required_argument, opt_rsyslog,
     "The remote syslog that will receive ncsa"},
    {"syslog-port", required_argument, opt_rsyslog_port,
     "The port used to send to the ncsa syslog"},
    {"local-syslog", no_argument, opt_syslog,
     "Log into the local syslog"},
    {NULL, 0, 0, NULL},
};

void usage(const char *prog)
{
    printf("Usage: %s [options]]\n", prog);

    for (int i = 0; options[i].name != NULL; i++) {
	printf("  --%s%s %.*s %s\n", options[i].name,
	       options[i].has_arg ? " VALUE" : "",
	       (int) (20 - strlen(options[i].name) -
		      (options[i].has_arg ? 6 : 0)),
	       "                                   ",
	       options[i].documentation);
    }
}				/* usage */

struct option *build_getopt_options()
{
    /* variables */
    int i;
    struct option *getopt_options = NULL;

    /* convert the 'option_doc' array into a 'struct option' array
     * for use with getopot_long_only */
    for (i = 0; options[i].name != NULL; i++) {
	getopt_options =
	    realloc(getopt_options, (i + 1) * sizeof(struct option));
	getopt_options[i].name = options[i].name;
	getopt_options[i].has_arg = options[i].has_arg;
	getopt_options[i].flag = NULL;
	getopt_options[i].val = options[i].val;
    }
    /* Add one last item for the list terminator NULL */
    getopt_options =
	realloc(getopt_options, (i + 1) * sizeof(struct option));
    getopt_options[i].name = NULL;

    return getopt_options;
}				/* build_getopt_options */


void *configure_fleece_from_cli(fleece_options_t * flconf,
				struct option **getopt_options)
{
    int c;
    int optindex = 0;
    char *tmp;

    while ((c =
	    getopt_long_only(flconf->argc, flconf->argv, "+hv",
			     *getopt_options, &optindex)) != -1) {
	switch (c) {
	case opt_version:
	    printf("Fleece version %s\n", FLEECE_VERSION);
	    return 0;
	case opt_help:
	    usage(flconf->argv[0]);
	    return 0;
	case opt_host:
	    flconf->host = strdup(optarg);
	    break;
	case opt_port:
	    flconf->port = (unsigned short) atoi(optarg);
	    break;
	case opt_window_size:
	    flconf->window_size = (size_t) atoi(optarg);
	    break;
	case opt_quiet:
	    flconf->quietmode = true;
	    break;
	case opt_field:
	    tmp = strchr(optarg, '=');
	    if (tmp == NULL) {
		printf("Invalid --field : expected 'foo=bar' form "
		       "didn't find '=' in '%s'", optarg);
		usage(flconf->argv[0]);
		exit(1);
	    }
	    flconf->extra_fields_len += 1;
	    flconf->extra_fields = realloc(flconf->extra_fields,
					   flconf->extra_fields_len *
					   sizeof(struct kv));
	    *tmp = '\0';	/* turn '=' into null terminator */
	    tmp++;		/* skip to first char of value */
	    flconf->extra_fields[flconf->extra_fields_len - 1].key =
		strdup(optarg);
	    flconf->extra_fields[flconf->extra_fields_len - 1].key_len =
		strlen(optarg);
	    flconf->extra_fields[flconf->extra_fields_len - 1].value =
		strdup(tmp);
	    flconf->extra_fields[flconf->extra_fields_len - 1].value_len =
		strlen(tmp);
	    break;
	case opt_rsyslog:
	    flconf->rsyslog = strdup(optarg);
	    break;
	case opt_rsyslog_port:
	    flconf->rsyslog_port = (unsigned short) atoi(optarg);
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

    if (flconf->host == NULL && flconf->rsyslog == NULL) {
	printf("Missing --host or --syslog flag\n");
	usage(flconf->argv[0]);
	return NULL;
    }

    if (flconf->host && flconf->port == 0) {
	printf("Missing --port flag\n");
	usage(flconf->argv[0]);
	return NULL;
    }

    if (flconf->rsyslog && flconf->rsyslog_port == 0) {
	printf("Missing --syslog-port flag\n");
	usage(flconf->argv[0]);
	return NULL;
    }

    flconf->argc -= optind;
    flconf->argv += optind;

    return flconf;
}


int main(int argc, char **argv)
{
    /* declarations and initialisations */
    int sockfd = 0, sockfdsyslog = 0, retval = 0;
    size_t j = 0;

    char myhostname[HOSTNAME_MAXSZ];

    struct sockaddr_in servaddr, servaddrsyslog;
    char sendline[LINE_MAXSZ];
    char ncsaline[LINE_MAXSZ];

    json_t *jsonevent;
    /* propose to make jsonerror global, we could then retreive the error of a sub function */
    json_error_t jsonerror;
    char *jsoneventstring;

    struct option *getopt_options = NULL;

    /* initialize fleece configuration struct */
    fleece_options_t flconf = {
	argc,
	argv,
	NULL,
	"",
	(unsigned short) 0,
	(size_t) LINE_MAXSZ,
	NULL,
	(size_t) 0,
	false,
	NULL,
	0,
	false
    };

    /* File descriptors for udp and stdin */
    fd_set fds;
    struct timeval tv;

    /* convert the 'option_doc' array into a 'struct option' array
     * for use with getopt_long_only */
    getopt_options = build_getopt_options();
    if (configure_fleece_from_cli(&flconf, &getopt_options) == NULL) {
	retval = 1;
	goto end;
    }

    /* who am i */
    gethostname(myhostname, sizeof(myhostname));

    /* send to a remote json reader */
    if (flconf.host != NULL) {
	sockfd = socket(AF_INET, SOCK_DGRAM, 17);

	if (sockfd < 0) {
	    fprintf(stderr, "failed to open sock: %s\n", strerror(errno));
	    retval = 1;
	    goto end;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	hostname_to_ip(flconf.host, flconf.ip, sizeof(flconf.ip));
	servaddr.sin_addr.s_addr = inet_addr(flconf.ip);
	servaddr.sin_port = htons(flconf.port);
    }

    /* prepare syslogging */
    if (flconf.syslog == true) {
	openlog("fleece", LOG_NDELAY, syslog_facility);
    }
    /* send to a remote syslog */
    if (flconf.rsyslog != NULL) {
	char rsyslog_ip[IP_MAXSZ];

	sockfdsyslog = socket(AF_INET, SOCK_DGRAM, 17);

	if (sockfdsyslog < 0) {
	    fprintf(stderr, "failed to open sock: %s\n", strerror(errno));
	    retval = 1;
	    goto end;
	}
	bzero(&servaddrsyslog, sizeof(servaddrsyslog));
	servaddrsyslog.sin_family = AF_INET;

	hostname_to_ip(flconf.rsyslog, rsyslog_ip, sizeof(rsyslog_ip));
	servaddrsyslog.sin_addr.s_addr = inet_addr(rsyslog_ip);
	servaddrsyslog.sin_port = htons(flconf.rsyslog_port);
    }

    /* prepare select */
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if (!flconf.quietmode) {
	if (flconf.host) {
	    printf("%s has ip %s\n", flconf.host, flconf.ip);
	    printf("fleece: sending jsonified stdin to %s:%i\n",
		   flconf.host, flconf.port);
	}
	if (flconf.rsyslog) {
	    printf("fleece: sending ncsaified stdin to %s:%i\n",
		   flconf.rsyslog, flconf.rsyslog_port);
	}
    }

    /* we could use the non blocking fgets function now (if, supported on sunos too?) */
    while (fgets(sendline, flconf.window_size, stdin) != NULL) {
	if (strlen(sendline) == 0) {
	    /* no data let's not burn cpu for nothing */
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    retval = select(1, &fds, NULL, NULL, &tv);
	    if (retval == -1 && errno != EAGAIN) {
		goto end;
	    }
	} else {
	    /* hey there's data let's process it */
	    jsonevent = json_loads(sendline, 0, &jsonerror);
	    if (jsonevent == NULL) {
		/*json not parsed ok then push jsonified version of msg */
		jsonevent = json_object();
		if (jsonevent == NULL)
		    continue;
		json_object_set_new(jsonevent, REJECTED_JSON,
				    json_string(sendline));
	    }

	    /* json parsed ok */
	    for (j = 0; j < flconf.extra_fields_len; j++) {
		json_object_set_new(jsonevent, flconf.extra_fields[j].key,
				    json_string(flconf.
						extra_fields[j].value));
	    }

	    /* add mandatory fields */
	    json_object_set_new(jsonevent, "file", json_string("-"));
	    json_object_set_new(jsonevent, "host",
				json_string(myhostname));

	    /* copy modified json string to sendline */
	    jsoneventstring = json_dumps(jsonevent, JSON_COMPACT);
	    if (jsoneventstring) {
		if (sockfd) {
		    sendto(sockfd, jsoneventstring,
			   strlen(jsoneventstring), 0,
			   (struct sockaddr *) &servaddr,
			   sizeof(servaddr));
		}

		/* transform the json into a nearly classical ncsa */
		if (flconf.syslog == true || sockfdsyslog) {
		    retval = jsonncsa(jsonevent, ncsaline, LINE_MAXSZ);

		    if (retval) {
			if (!flconf.quietmode) {
			    fprintf(stderr,
				    "jsonncsa failed with status %d\n",
				    retval);
			}
		    } else {
			/* simple destination for log, recommend syslog-ng like then */
			if (flconf.syslog == true) {
			    syslog(syslog_priority, "%s", ncsaline);
			}

			/* or send directly to an other remote syslog */
			if (sockfdsyslog) {
			    sendto(sockfdsyslog, ncsaline,
				   strlen(ncsaline), 0,
				   (struct sockaddr *) &servaddrsyslog,
				   sizeof(servaddrsyslog));
			}
		    }
		}

		/* free memory */
		free(jsoneventstring);
	    }
	    json_object_clear(jsonevent);
	    json_decref(jsonevent);
	}
    }				/* loop forever, reading from a file */

  end:
    if (flconf.syslog == true) {
	closelog();
    }

/* XXX if we are not debugging, maybe let's not care about freeing resources before exit */
#ifdef DEBUG
    if (sockfdsyslog > 0) {
	close(sockfdsyslog);
    }

    if (sockfd > 0) {
	close(sockfd);
    }

    free(flconf.rsyslog);
    free(flconf.host);

    for (j = 0; j < flconf.extra_fields_len; ++j) {
	free(flconf.extra_fields[j].value);
	free(flconf.extra_fields[j].key);
    }

    free(flconf.extra_fields);
#endif

    return retval;
}
