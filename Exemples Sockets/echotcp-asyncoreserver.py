#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

"""
SYNOPSIS

    echotcp-asyncoreserver.py [-h,--help] [--version] [-p,--port <port>, default=1234]

DESCRIPTION

    Creates a ECHO/TCP server, resending all received data back to sender.
    Uses asyncore module
    Default port 1234


EXAMPLES

    echotcp-asyncoreserver.py  --port 4321

AUTHOR

    Carles Mateu <carlesm@carlesm.com>

LICENSE

    This script is published under the Gnu Public License GPL3+

VERSION

    0.0.1 
"""

import sys, os, traceback, optparse
import time, datetime
import socket, asyncore
 

__program__ = "echotcp-asyncoreserver"
__version__ = '0.0.1'
__author__ = 'Carles Mateu <carlesm@carlesm.com>'
__copyright__ = 'Copyright (c) 2012  Carles Mateu '
__license__ = 'GPL3+'
__vcs_id__ = '$Id: echotcp-asyncoreserver.py 563 2012-05-06 20:12:02Z carlesm $'


class ServerSocket(asyncore.dispatcher):
    def __init__(self, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET,socket.SOCK_STREAM)
        self.bind(('',port))
        self.listen(5)

    def handle_accept(self):
        newsocket,address=self.accept()
        print >>sys.stderr, "Connect from: ",address
        AttendServerSocket(newsocket)

class AttendServerSocket(asyncore.dispatcher_with_send):
    def handle_read(self):
        recvdata=self.recv(8192)
        if recvdata:
            self.send(recvdata)
        else:
            self.close()
    def handle_close(self):
        print >>sys.stderr,"Disconnect from:", self.getpeername()


def main():
    global options, args
    ServerSocket(options.port)
    asyncore.loop()

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




