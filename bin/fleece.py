#!/usr/bin/env python
__author__ = "Aurelien ROUGEMONT (GANDI)"
__copyright__ = "internal"
__revision__ = "$Id$"
__version__ = "0.2"

#from socket import *
import socket
import getopt
import sys
import os
import json
import select
import pprint

def usage():
    print os.path.basename(sys.argv[0]) + " parameters are :"
    print "     -h --help"
    print "         prints this usage message and exit"
    print "     -H --hostname <hostname.domain.tld>"
    print "         destination ip address for udp stream"
    print "     -P --port <12345>"
    print "         destination port for udp stream"
    print "     -F --fields key1value,key2,value,..."

def iterload(stream):
    bufferjson = ""
    dec = json.JSONDecoder()
    for line in stream:
        bufferjson = bufferjson.strip() + line.strip()
        while(True):
            try:
                r = dec.raw_decode(bufferjson)
            except Exception:
                break
            yield r[0]
            bufferjson = bufferjson[r[1]:].strip()

def main():
    try:
        opts, args = getopt.getopt( \
            sys.argv[1:], \
            "hH:P:F:", \
            ["help", "hostname=", "port=", "fields="] \
            )
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    hostname = "events.mgt.gandi.net"
    port = 1337
    fields = ""

    # getopt data filling
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(1)
        elif opt in ("-H", "--hostname"):
            hostname = arg
        elif opt in ("-P", "--port"):
            port = int(arg)
        elif opt in ("-F", "--fields"):
            fields = arg
        else:
            raise RuntimeError("unhandled option")

    address = (hostname, port)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    fields = fields.split(',')

    for o in iterload(sys.stdin):
        try:
            for entry in fields:
                key, value = entry.decode('utf-8', 'replace').split('=')
                o[key] = value
        except ValueError:
            #no value to add to the dict
            pass
        client_socket.sendto(json.dumps(o), address)

if __name__ == "__main__":
    main()
