import time, cons
import struct, socket, signal
import os, optparse, sys

__version__ = '0.0.1'


def main():
    actState("DISCONNECTED")
    register()

#FASE DE REGISTRE
#PROCES D'ENREGISTRAMENT
def register():
    global rndnum
    reply = ""

    createSock()
    debugMode("Creat Socket UDP")
    treatDataFile()
    debugMode("Lectura del fitxer de dades del client")
    regPDU = definePDU(cons.PDU_FORM,cons.REGISTER_REQ, cons.DEF_RND, '')
    debugMode("Inici del proces de registre")
    reply = registerloop(regPDU)
    debugMode("Proces de registre finalitzat")
    rndnum = reply[3]
    replyProcess(reply)

#TRACTAMENT DADES LLEGIDES
def treatDataFile():
    global eqp, mac, ip, port

    #lectura del fitxer i recollida de dades necessaries.
    eqp, mac, ip, port = readFile(options.client)
    eqp = eqp.split()[1]
    mac = mac.split()[1]
    ip = ip.split()[1]
    port = port.split()[1]

#CREACIO DEL SOCSKET
def createSock():
    global socudp

    #creacio i assignacio del socket a un port
    socudp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    socudp.bind(("", 0))

#BUCLE PER ENREGISTRAMENT
def registerloop(regPDU):
    num_packs = 1
    seq_num = 1
    t = 2

    #intentar registrar-se mentre no s'arribi al nombre maxim d'intents
    while seq_num <= cons.MAX_SEQ:
        try:
            debugMode("Intent de registre " + str(seq_num))
            debugMode("Packet numero " + str(num_packs))
            return regTry(regPDU, t)

        except socket.timeout:
        #si no es realitza el registre modifiquem el temps de enviament del
        #seguent intent de registre en funcio del nombre de paquets enviats
            if num_packs == 1:
                t = 2

            if num_packs >= 3 and t != cons.MAX_TIME:
                t = t + 2

            elif num_packs == cons.MAX_PACK:
                t = t + 5
                num_packs = 1
                seq_num = seq_num + 1

            num_packs = num_packs + 1

    return ""

#TRACTAMENT DE RESPOSTA DE REGISTRE
def replyProcess(reply):
    #tractem la resposta rebuda per part del servidor en funcio del tipus de paquet
    #que haguem rebut
    if reply == "":
        print time.strftime("%X") + " No s'ha pogut contactar amb el servidor"
        closeConnection()

    elif reply[0] == cons.REGISTER_ACK:
        actState("CONNECTED")
        #en cas de que el registre sigui correcte ens disposem a distribuir el
        #treball de mantenir la comunicacio i la recepcio de comandes
        distributeWork(reply)

    elif reply[0] == cons.REGISTER_NACK:
        print time.strftime("%X") + " Denegacio de Registre"
        closeConnection()

    elif reply[0] == cons.REGISTER_REJ:
        print time.strftime("%X") +" "+ reply[4].rstrip('\n')
        closeConnection()

    elif reply[0] == cons.ERROR:
        print time.strftime("%X") + " Error de Protocol"
        closeConnection()

#ENVIAMENT PDU PER REGISTRE
def regTry(regPDU, t):
    global socudp, state, options, ip, port, recPort
    #s'intenta fer un registre
    recPort = socudp.getsockname()[1]

    actState("WAIT_REG")
    #enviem la nostra PDU i establim un temps per esperar la resposta
    socudp.sendto(regPDU, (ip, int(port)))
    socudp.settimeout(t)
    #retornem la resposta si l'hem rebut, en cas contrari es llanca una
    #exepcio que sera agafada per la funcio "registerloop"
    return struct.unpack(cons.PDU_FORM,socudp.recvfrom(recPort)[0])

#DIVISIO DE TREABALL PER RECEPCIO DE TECLAT I TRACTAMENT ALIVES
def distributeWork(reply):
    global pid
    debugMode("Registre Correcte")
    debugMode("Creacio de proces fill per a rebre instruccions per teclat")
    pid = os.fork()
    #distribuim el treball a realitzar
    #la rebuda de comandes sera realitzada per el "fill" mentre que el manteniment
    #de la conexio el realitzara el "pare"
    if pid == 0:
        debugMode("Proces fill preparat per a rebre instruccions per teclat")
        KeyboardCommand(reply)

    else:
        debugMode("Proces pare preparat per iniciar el manteniment de comunicacions")
        AliveTreatment(reply)

#FASE DE MANTENIMENT DE COMUNICACIO
#TRACTAMENT ALIVES
def AliveTreatment(reply):
    global socudp, ip, port, rndnum, recPort, pid
    #preparacio dels parametres necessaris per al proces de enviament d'ALIVES
    resp = 0
    comPDU = definePDU(cons.PDU_FORM, cons.ALIVE_INF, rndnum, "")
    socudp.sendto(comPDU, (ip, int(port)))
    debugMode("Enviat primer Paquet amb ALIVE")
    resp = resp + 1
    timer = time.time()
    first = False
    debugMode("Iniciant temportizador per a rebre respota d'ALIVES")
    #recv no bloquejant
    socudp.setblocking(0)
    #enviament de alives
    while True:
        #si el servidor contesta als nostres enviaments continuem enviant
        if resp < 3:
            try:
                #deteccio de finalitzacio
                signal.signal(signal.SIGTERM,handler)
                #recepcio del paquet del servidor
                msg = struct.unpack(cons.PDU_FORM, socudp.recvfrom(recPort)[0])
                debugMode("Enviat Paquet amb ALIVE")
                resp, timer = sendAlive(resp, timer, comPDU)
                #comprovacio del paquet rebut
                if msg[0] == cons.ALIVE_ACK and cmp(reply[1:4],msg[1:4]) == 0:
                    resp  = resp - 1
                    #en cas de resposta al primer ALIVE informem del canvi d'estat
                    if first == False:
                        debugMode("Primer ALIVE rebut passem a estat ALIVE")
                        actState("ALIVE")
                        first = True
                #si el paquet es un rebug finalitzem proces
                elif msg[0] == cons.ALIVE_REJ:
                    debugMode("Rebut rebuig de paquet")
                    debugMode("Paquet rebut: " + "Tipus paquet: " + str(msg[0]) + " Nom: " + str(msg[1]) + " MAC: " + str(msg[2]) + " Aleatori " + str(msg[3]) + " Dades: " + str(msg[4]))
                    os.kill(pid,signal.SIGKILL)
                    closeConnection()
                    timedRegister()

                signal.signal(signal.SIGTERM,handler)
            except socket.error:
                debugMode("Rebut error de paquet")
                debugMode("Paquet rebut: " + "Tipus paquet: " + str(msg[0]) + " Nom: " + str(msg[1]) + " MAC: " + str(msg[2]) + " Aleatori " + str(msg[3]) + " Dades: " + str(msg[4]))
                signal.signal(signal.SIGTERM,handler)
                debugMode("Enviat Paquet amb ALIVE")
                resp, timer = sendAlive(resp, timer, comPDU)
        #si el servidor no contesta tanquem proces
        else:
            debugMode("Impossible mantenir comunicacio amb el servidor")
            os.kill(pid,signal.SIGKILL)
            closeConnection()
            timedRegister()

#INTENT DE REGISTRE TEMPORITZAT
def timedRegister():
    timer = time.time()
    while True:
        if time.time() - timer  >= 10:
            register()

#TRACTAMENT DEL SENYAL
def handler(signum,frame):
    global pid
    #tractament del senyal kill
    os.kill(pid,signal.SIGKILL)
    closeConnection()
    sys.exit()

#ENVIAMENT ALIVES
def sendAlive(resp, timer, comPDU):
    #enviment d'alives segons temporitzador
    if time.time() - timer >= cons.SND_TM:
        socudp.sendto(comPDU, (ip, int(port)))
        #en cas de enviament increment de numero de enviades per portar control
        #i actualitzacio del temporitzador
        resp = resp + 1
        timer = time.time()

    return resp, timer

#FASE DE RECECPCIO DE COMANDES
def KeyboardCommand(reply):
    #recepcio de comandes fins que s'introdueixi la comanda quit
    debugMode("Iniciant espera de comandes")
    while True:
        try:
            #lectura de consola
            com = raw_input("")

            #tancament de tots els processos en cas de quit
            if com == cons.QUIT:
                while True:
                    debugMode("Rebut quit, finalitzant client")
                    os.kill(os.getppid(),signal.SIGTERM)
                    #enviament de configuracio en cas de send
            elif com == cons.SEND:
                debugMode("Rebut send-conf")
                openTCPCon(reply)
                sendConf(reply)
                closeTCPCON()
            #peticio de recepcio en cas de get
            elif com == cons.GET:
                debugMode("Rebut get-conf")
                openTCPCon(reply)
                getConf(reply)
                closeTCPCON()
            #en cas de comanda incorrecta avis
            else:
                print time.strftime('%X') + " Commanda Incorrecta"

        except KeyboardInterrupt:
            while True:
                os.kill(os.getppid(),signal.SIGTERM)
        except socket.error:
            pass
        except:
            #tancament en cas de fallada de recepcio de comandes
            while True:
                os.kill(os.getppid(),signal.SIGTERM)

#ENVIAR CONFIGURACIO AL SERVIDOR
def sendConf(reply):
    global soctcp, ip, rndnum

    #lectura del fitxer a enviar al servidor
    debugMode("Llegit fitxer a enviar")
    fileLines = readFile(options.file)
    #calcul del tamany
    size = os.stat(options.file).st_size
    data = options.file + "," + str(size)
    #establiment de temps per a rebre resposta del servidor
    debugMode("Establert temportizador per a rebre resposta del servidor")
    soctcp.settimeout(cons.TCP_WAIT)
    #solicitud de enviament de arxiu de configuracio
    print time.strftime('%X') + " Solicitud d'enviament d'arxiu de configuracio al servidor (" + options.file + ")"
    sendPDUTCP(cons.SEND_FILE,data)

    try:
        #recepcio de resposta per part del servidor
        msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))
        debugMode("Resposta rebuda iniciant comprovacio de camps")
        #comprovacio de camps correctes
        if msg[0] == cons.SEND_ACK and (cmp(reply[1:4],msg[1:4]) == 0):
            #enviament de linies de l'arxiu de configuracio
            debugMode("Camps Correctes enviament de fitxer")
            for l in fileLines:
                sendPDUTCP(cons.SEND_DATA,l)
            #linia final
            sendPDUTCP(cons.SEND_END,"")

            print time.strftime('%X') + " Finalitzat enviament d'arxiu de configuracio al servidor (" + options.file + ")"
        else:
            print time.strftime('%X') + " Abortat enviament d'arxiu de configuracio al servidor (" + options.file + ")"
            print time.strftime('%X') + " Camps de la PDU Incorrectes"


    except socket.error:
        #tancament de socket en cas de no rebre resposta
        print time.strftime('%X') + " No hi ha resposta per part del servidor: send-conf"
        closeTCPCON()

#DEMANAR CONFIGURACIO AL SERVIDOR
def getConf(reply):
    global soctcp, ip, rndnum
    prefile = list()
    #borrar quan servidor fet
    size = os.stat(options.file).st_size
    data = options.file + "," + str(size)
    #borrar quan servidor fet
    print time.strftime('%X') + " Solicitud de recepcio d'arxiu de configuracio al servidor"
    #enviament peticio de l'arxiu de configuracio
    sendPDUTCP(cons.GET_FILE,data)
    #s'estableix el temps de espera per a rebre els packets
    soctcp.settimeout(cons.TCP_WAIT)

    try:
        #recepcio dels paquets amb la configuracio enviada per el servidor
        msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))
        debugMode("Resposta rebuda iniciant comprovacio de camps")
        #comprovacio de que les dades rebudes son correctes
        if msg[0] == cons.GET_ACK and (cmp(reply[1:4],msg[1:4]) == 0):
            debugMode("Inici de recepcio de dades")
            #tractament de les dades rebudes
            while msg[0] != cons.GET_END:
                msg = struct.unpack(cons.TCP_FORM, soctcp.recv(cons.SIZETCP))
                debugMode("Paquet rebut: " + "Tipus paquet: " + str(msg[0]) + " Nom: " + str(msg[1]) + " MAC: " + str(msg[2]) + " Aleatori " + str(msg[3]) + " Dades: " + str(msg[4]))
                if msg[0] == cons.GET_DATA:
                    prefile.append(msg[4].split('\n')[0] + '\n')

            if msg[0] == cons.GET_END:
                f = open(options.file,"w")
                for l in prefile:
                    f.write(l)
                f.close()
                print time.strftime('%X') +  " Finalitzat recepcio d'arxiu de configuracio al servidor"
            else:
                print time.strftime('%X') +  " Error recepcio d'arxiu de configuracio al servidor ultim paquet no GET_END"
        else:
            print time.strftime('%X') + " Abortat recepcio d'arxiu de configuracio al servidor (" + options.file + ")"
            print time.strftime('%X') + " Camps de la PDU Incorrectes"
    except:
        #en cas de no resposta per part del servidor es tanca la conexio
        print time.strftime('%X') +  " No hi ha resposta per part del servidor: get-conf"
        closeTCPCON()

#OBRIR CONEXIO TCP
def openTCPCon(reply):
    global soctcp, ip
    #creacio del socket TCP i conexio amb el servidor
    debugMode("Obrint Connexio TCP")
    portTCP = int(reply[4][:4 ])
    soctcp = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    soctcp.connect((ip,portTCP))

#TANCAMENT CONEXIO TCP
def closeTCPCON():
    global soctcp
    debugMode("Tancant conexio TCP")
     #tancament de la conexio TCP
    soctcp.close()

#ENVIAMENT PDU DE TCP
def sendPDUTCP(sign,data):
    global rndnum, soctcp
    #envia la PDU per el port TCP
    pduFile = definePDU(cons.TCP_FORM, sign, rndnum, data)
    soctcp.send(pduFile)

#FUNCIONS UTILITZADES EN TOTES LES FASES
#DEFINEIX LA PDU A UTILITZAR
def definePDU(form, sign, random, data):
    global eqp, mac
    #crea una PDU amb les dades rebudes per parametre
    debugMode("Dades a enviar:")
    debugMode("Tipus Paquet: " + str(sign) + " Nom: " + eqp + " MAC: " + mac + " Numero aleatori: " + str(random) + " Dades: " + data)
    return struct.pack(form, sign, eqp, mac, random, data)

#MOSTRA L'ESTAT DEL CLIENT
def actState(st):
    global state
    #mostra els canvis d'estat
    state = st
    print time.strftime('%X') + " Client passa a l'estat: " + state

#LLEGIR DEL FITXER
def readFile(fileToRead):
    #lectura del fitxer i retorn del contingut
    f = open(fileToRead, "r")
    lines = f.readlines()
    f.close()

    return lines

#TANCA CONEXIO UDP
def closeConnection():
    global socudp
    #tancament de la coexio
    actState("DISCONNECTED")
    socudp.close()

#MODE DEBUG
def debugMode(toPrint):
    if options.verbose == True:
        print time.strftime('%X') + ' ' + toPrint

if __name__ == '__main__':
    #parseig de les comandes rebudes a la hroa de la crida
    parser = optparse.OptionParser(formatter=optparse.TitledHelpFormatter(), usage=globals()["__doc__"], version=__version__)
    parser.add_option('-c', '--client', action='store', default='client.cfg', help='-c <nom_arxiu>')
    parser.add_option('-f', '--file', action='store', default='boot.cfg', help='-f <nom_arxiu>')
    parser.add_option('-d', '--debug', action='store_true', dest='verbose', default=False)
    (options, args) = parser.parse_args()
    main()
