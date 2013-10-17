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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <getopt.h>

#include <string.h>
#include <strings.h>

#include <jansson.h>

#include "str.h"
#include "fleece.h"
#include "hostnameip.h"
#include "net.h"

int main(int argc, char**argv)
{
    /* declarations and initialisations */
    int sockfd, i, c, retval;
    size_t j = 0;

    char hostname[200];
    char *tmp;

    struct sockaddr_in servaddr;
    char sendline[1024];

    json_t *jsonevent;
    json_error_t jsonerror;
    char *jsoneventstring;

    struct option *getopt_options = NULL;

    /* initialize fleece configuration struct */
    fleece_options_t flconf  = {
        NULL,
        "",
        (unsigned short) 0,
        (size_t)1024,
        NULL,
        (size_t)0,
        false
    };

    /* File descriptors for udp and stdin */
    fd_set fds;
    struct timeval tv;

    /* convert the 'option_doc' array into a 'struct option' array
     * for use with getopt_long_only */
    getopt_options = build_getopt_options();

    while (i = -1,
    c = getopt_long_only(argc, argv, "+hv", getopt_options, &i), c != -1) {
      switch (c) {
        case opt_version:
          printf("Fleece version %s\n",FLEECE_VERSION);
          return 0;
        case opt_help:
          usage(argv[0]);
          return 0;
        case opt_host:
          flconf.host = strdup(optarg);
          break;
        case opt_port:
          flconf.port = (unsigned short)atoi(optarg);
          break;
        case opt_window_size:
          flconf.window_size = (size_t)atoi(optarg);
          break;
        case opt_quiet:
          flconf.quietmode = true;
          break;
        case opt_field:
          tmp = strchr(optarg, '=');
          if (tmp == NULL) {
            printf("Invalid --field : expected 'foo=bar' form " \
                   "didn't find '=' in '%s'", optarg);
            usage(argv[0]);
            exit(1);
          }
          flconf.extra_fields_len += 1;
          flconf.extra_fields = realloc(flconf.extra_fields,
                                 flconf.extra_fields_len * sizeof(struct kv));
          *tmp = '\0'; /* turn '=' into null terminator */
          tmp++; /* skip to first char of value */
          flconf.extra_fields[flconf.extra_fields_len - 1].key = strdup(optarg);
          flconf.extra_fields[flconf.extra_fields_len - 1].key_len = strlen(optarg);
          flconf.extra_fields[flconf.extra_fields_len - 1].value = strdup(tmp);
          flconf.extra_fields[flconf.extra_fields_len - 1].value_len = strlen(tmp);
          break;
        default:
      printf("Not handled\n");
          usage(argv[0]);
          return 1;
      }
    }
    free(getopt_options);

    if (flconf.host == NULL) {
      printf("Missing --host flag\n");
      usage(argv[0]);
      return 1;
    }

    if (flconf.port == 0) {
      printf("Missing --port flag\n");
      usage(argv[0]);
      return 1;
    }

    argc -= optind;
    argv += optind;

    /*
     * prepare info to send stuff
     */

    gethostname(hostname, sizeof(hostname));

    sockfd = socket(AF_INET,SOCK_DGRAM, 0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    hostname_to_ip(flconf.host,flconf.ip);
    servaddr.sin_addr.s_addr=inet_addr(flconf.ip);
    servaddr.sin_port=htons(flconf.port);

    /* prepare select */
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if(!flconf.quietmode)
    {
        printf("%s has ip %s\n", flconf.host, flconf.ip);
        printf("fleece: sending jsonified stdin to %s:%i\n", flconf.host, flconf.port);
    }

    while (fgets(sendline, flconf.window_size, stdin) != NULL)
    {
        if (strlen(sendline) == 0)
        {
             /* no data let's not burn cpu for nothing */
             tv.tv_sec = 1;
             tv.tv_usec = 0;
             retval = select(1, &fds, NULL, NULL, &tv);
             if ( retval == -1 )
             {
                 exit(0);
             }
        } else {
             /* hey there's data let's process it */
             jsonevent = json_loads(sendline, 0, &jsonerror);
             if (jsonevent== NULL)
             {
              /* json not parsed ok then push jsonified version of msg */
                 jsonevent = json_object();
                 json_object_set(jsonevent, "message", json_string(sendline));
             }
             /* json parsed ok */
             for ( j = 0; j < flconf.extra_fields_len; j++)
             {
                 json_object_set_new(jsonevent, flconf.extra_fields[j].key, \
                      json_string(flconf.extra_fields[j].value));
             }

             /* add mandatory fields */
             json_object_set_new(jsonevent, "file", json_string("-"));
             json_object_set_new(jsonevent, "host", json_string(hostname));

             /* copy modified json string to sendline */
             jsoneventstring = json_dumps(jsonevent,JSON_COMPACT);
             strcpy(sendline, jsoneventstring);

             sendto(sockfd, sendline, strlen(sendline), 0, \
                 (struct sockaddr *)&servaddr, sizeof(servaddr));

             /* free memory */
             json_decref(jsonevent);
             free(jsoneventstring);
             }
    } /* loop forever, reading from a file */
}
