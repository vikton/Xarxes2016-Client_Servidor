#pylint: disable=C, W0601, W0602, W0613
"""
import time
import struct, socket, signal
import os, optparse, sys
import cons


__version__ = '0.0.1'


def main():
    actState("DISCONNECTED")
    register()


#Fase de Registre
def register():
    global ip, port, options, rndnum

    reply = ""

    createSock()
    regPDU = definePDU(cons.PDU_FORM,cons.REGISTER_REQ, cons.DEF_RND, '')
    ip, port = readRecInf(options.client)
    reply = registerloop(regPDU)
    rndnum = reply[3]
    replyProcess(reply)

#Fase manteniment Comunicacio
def keepCommunication(reply):
    pid = os.fork()

    if pid == 0:
        KeyboardCommand(reply)

    else:
        AliveTreatment(reply,pid)


#Tractament Alives
def AliveTreatment(reply, pid):
    global socudp, ip, port, rndnum

    resp = 0
    comPDU = definePDU(cons.PDU_FORM, cons.ALIVE_INF, rndnum, "")
    socudp.sendto(comPDU, (ip, int(port)))
    resp = resp + 1
    timer = time.time()
    first = False
    socudp.setblocking(0)

    while True:
        if resp < 3:
            try:
                signal.signal(signal.SIGTERM,handler)
                msg = struct.unpack(cons.PDU_FORM, socudp.recvfrom(socudp.getsockname()[1])[0])
                resp, timer = sendAlive(resp, timer, comPDU)

                if msg[0] == cons.ALIVE_ACK:

                    if cmp(reply[1:4],msg[1:4]) == 0:
                        resp  = resp - 1
                        if first == False:
                            actState("ALIVE")
                            first = True
                elif msg[0] == cons.ALIVE_REJ:
                    os.kill(pid,signal.SIGKILL)
                    timer = time.time()
                    closeConnection()
                    while True:
                        if time.time() - timer >= 9:
                            register()

            except socket.error:
                resp, timer = sendAlive(resp, timer, comPDU)

        else:
            os.kill(pid,signal.SIGKILL)
            closeConnection()
            timer = time.time()
            while True:
                if time.time() - timer  >= 10:
                    register()


#SIGNAL handler
def handler(signum,frame):
    closeConnection()
    sys.exit()

#Enviament Alives
def sendAlive(resp, timer, comPDU):
    if time.time() - timer >= cons.SND_TM:
        socudp.sendto(comPDU, (ip, int(port)))
        resp = resp + 1
        timer = time.time()

        return resp, timer

    return resp, timer


#Recepcio de Comandes
def KeyboardCommand(reply):
    try:
        while True:

            com = raw_input("")

            if com == cons.QUIT:
                os.kill(os.getppid(),signal.SIGTERM)
                socudp.close()
                sys.exit()

            elif com == cons.SEND:
                openTCPCon(reply)
                sendConf()
                closeTCPCON()

            elif com == cons.GET:
                openTCPCon(reply)
                getConf()
                closeTCPCON()

            else:
                print time.strftime('%X') + " Commanda Incorrecta"

    except:
        #socudp.close()
        os.kill(os.getppid(),signal.SIGTERM)
        sys.exit()



#Obrir Conexio TCP
def openTCPCon(reply):
    global soctcp, ip

    portTCP = int(reply[4][:4])
    soctcp = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    soctcp.connect((ip,portTCP))
"""

#Send conf file
def sendConf():
    global soctcp, ip, rndnum

    fileLines = readFile(options.file)
    size = os.stat(options.file).st_size
    data = options.file + "," + str(size)
    soctcp.settimeout(cons.TCP_WAIT)

    print "Solicitud d'enviament d'arxiu de configuracio al servidor (" + options.file + ")"
    pduFile = definePDU(cons.TCP_FORM, cons.SEND_FILE, rndnum, data)
    soctcp.send(pduFile)

    try:
        msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))

        if msg[0] == cons.SEND_ACK:
            for l in fileLines:
                pduFile = definePDU(cons.TCP_FORM, cons.SEND_DATA, rndnum, l)
                soctcp.send(pduFile)


        pduFile = definePDU(cons.TCP_FORM, cons.SEND_END, rndnum, "")
        soctcp.send(pduFile)

        print "Finalitzat enviament d'arxiu de configuracio al servidor (" + options.file + ")"
    except:
        print "No hi ha resposta per part del servidor: send-conf"
        closeTCPCON()


def getConf():
    global soctcp, ip, rndnum

    size = os.stat(options.file).st_size
    data = options.file + "," + str(size)
    print "Solicitud de rececpcio d'arxiu de configuracio al servidor"
    pduFile = definePDU(cons.TCP_FORM, cons.GET_FILE, rndnum, data)
    f = open(options.file,"w")
    soctcp.send(pduFile)
    soctcp.settimeout(cons.TCP_WAIT)

    try:
        msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))

        if msg[0] == cons.GET_ACK:
            while msg[0] != cons.GET_END:
                msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))
                if msg[0] != cons.GET_END:
                    f.write(msg[4].split("\n")[0]+"\n")

        print "Finalitzat enviament d'arxiu de configuracio al servidor"
        f.close()
    except:
        print "No hi ha resposta per part del servidor: get-conf"
        f.close()
        closeTCPCON()
"""
#Llegir fitxer
def readFile():
    global options
    f = open(options.file, "r")
    lines = []

    for l in f:
        lines.append(l)

    return lines

#tancar Conexio
def closeTCPCON():
    global soctcp
    soctcp.close()

#Crear Socket
def createSock():
    global socudp

    socudp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    socudp.bind(("", 0))


#Creacio de la PDU
def definePDU(form, sign, random, data):
    global options

    (eqp, mac) = readSendInf(options.client)

    return struct.pack(form, sign, eqp, mac, random, data)


#Loop de sequencia de enviamentes de registre
def registerloop(regPDU):
    num_packs = 1
    seq_num = 1
    t = 2

    while seq_num <= cons.MAX_SEQ:
        try:
            return regTry(regPDU, t)

        except socket.timeout:

            if num_packs == 1:
                t = 2

            num_packs = num_packs + 1

            if num_packs >= 3 and t != cons.MAX_TIME:
                t = t + 2

            elif num_packs == cons.MAX_PACK:
                t = t + 5
                num_packs = 1
                seq_num = seq_num + 1

    return ""

#Send msgPDU
def regTry(regPDU, t):
    global socudp, state, options, ip, port
    recPort = socudp.getsockname()

    actState("WAIT_REG")
    socudp.sendto(regPDU, (ip, int(port)))
    socudp.settimeout(t)

    return struct.unpack(cons.PDU_FORM,socudp.recvfrom(recPort[1])[0])


#Llegir del fitxer nom i mac.cfg
def readSendInf(clfile):
    f = open(clfile, "r")
    lines = f.readlines()
    f.close()

    return (lines[0].split()[1], lines[1].split()[1])



def readFile(fileToRead):
    f = open(fileToRead, "r")
    lines = f.readlines()
    f.close()

    return lines


#Tractament de la resposta.
def replyProcess(reply):

    if reply == "":
        print time.strftime("%X") + " No s'ha pogut contactar amb el servidor"
        closeConnection()

    elif reply[0] == cons.REGISTER_ACK:
        actState("CONNECTED")
        keepCommunication(reply)

    elif reply[0] == cons.REGISTER_NACK:
        print time.strftime("%X") + " Denegacio de Registre"
        closeConnection()

    elif reply[0] == cons.REGISTER_REJ:
        print time.strftime("%X") + reply[4]
        closeConnection()

    elif reply[0] == cons.ERROR:
        print time.strftime("%X") + " Error de Protocol"
        closeConnection()


#Estat Actual del Client
def actState(st):
    global state

    state = st
    print time.strftime('%X') + " Client passa a l'estat: " + state


#Tancar conexio
def closeConnection():
    global socudp

    actState("DISCONNECTED")
    socudp.close()


if __name__ == '__main__':
    parser = optparse.OptionParser(formatter=optparse.TitledHelpFormatter(), usage=globals()["__doc__"], version=__version__)
    parser.add_option('-c', '--client', action='store', default='client.cfg', help='-c <nom_arxiu>')
    parser.add_option('-f', '--file', action='store', default='boot.cfg', help='-f <nom_arxiu>')
    (options, args) = parser.parse_args()
    main()
"""
