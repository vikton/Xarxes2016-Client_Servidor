#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

"""
SYNOPSIS

    echotcp-server.py [-h,--help] [--version] [-p,--port <port>, default=1234]

DESCRIPTION

    Creates a ECHO/TCP server, resending all received data back to sender.
    Default port 1234


EXAMPLES

    echotcp-server.py  --port 4321

AUTHOR

    Carles Mateu <carlesm@carlesm.com>

LICENSE

    This script is published under the Gnu Public License GPL3+

VERSION

    0.0.1 
"""

import sys, os, traceback, optparse
import time, datetime
import socket
 

__program__ = "echotcp-server"
__version__ = '0.0.1'
__author__ = 'Carles Mateu <carlesm@carlesm.com>'
__copyright__ = 'Copyright (c) 2012  Carles Mateu '
__license__ = 'GPL3+'
__vcs_id__ = '$Id: echotcp-server.py 550 2012-05-05 11:48:08Z carlesm $'



def setup():
    global sock, options
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("",options.port))
    sock.listen(5)

def mainloop():
    global sock, options
    try:
        while True:
            newsocket,adreca = sock.accept()
            print >>sys.stderr, "Connexio de ",adreca
            while True:
                dades = newsocket.recv(8192)
                if not dades:
                    break
                newsocket.sendall(dades)
            newsocket.close()
            print >> sys.stderr, "Desconnexió de ",adreca
    finally:
        sock.close()

def main():
    global options, args
    setup()
    mainloop()

if __name__ == '__main__':
    try:
        start_time = time.time()
        parser = optparse.OptionParser(formatter=optparse.TitledHelpFormatter(), usage=globals()["__doc__"],version=__version__)
        parser.add_option ('-v', '--verbose', action='store_true', default=False, help='verbose output')
        parser.add_option ('-p', '--port', action='store', default=1234, help='Listening port, default 1234')
        (options, args) = parser.parse_args()
        if len(args) > 0: parser.error ('bad args, use --help for help')

        if options.verbose: print time.asctime()

        main()

        now_time = time.time()
        if options.verbose: print time.asctime()
        if options.verbose: print 'TOTAL TIME:', (now_time - start_time), "(seconds)"
        if options.verbose: print '          :', datetime.timedelta(seconds=(now_time - start_time)) 
        sys.exit(0)
    except KeyboardInterrupt, e: # Ctrl-C
        raise e
    except SystemExit, e: # sys.exit()
        raise e
    except Exception, e:
        print 'ERROR, UNEXPECTED EXCEPTION'
        print str(e)
        traceback.print_exc()
        os._exit(1)




