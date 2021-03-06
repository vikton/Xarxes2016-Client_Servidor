#define _POSIX_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define STATE_DISCON 0
#define STATE_REGIST 1
#define STATE_ALIVE 2
#define LIST "list"
#define QUIT "quit"
#define BUFSIZEUDP 78
#define BUFSIZETCP 178
#define NUMCOMPUTERS 100
#define MAP_ANONYMOUS 0x20
#define WAITTIME 4
/*REGISTER CODE*/
#define REGISTER_REQ 0x00
#define REGISTER_ACK 0x01
#define REGISTER_NACK 0x02
#define REGISTER_REJ 0x03

/*ALIVE CODE*/
#define ALIVE_INF 0x10
#define ALIVE_ACK 0x11
#define ALIVE_NACK 0x12
#define ALIVE_REJ 0x13

/*SEND CODE*/
#define SEND_FILE 0x20
#define SEND_ACK 0x21
#define SEND_NACK 0x22
#define SEND_REJ 0x23
#define SEND_DATA 0x24
#define SEND_END 0x25

/*GET CONF*/
#define GET_FILE 0x30
#define GET_ACK 0x31
#define GET_NACK 0x32
#define GET_REJ 0x33
#define GET_DATA 0x34
#define GET_END 0x35

#define ERROR 0x09

struct SERVINFO{
  char nomServ[7];
  char MAC[13];
  char UDP[5];
  char TCP[5];
};

struct PCREG{
  char nomEquip[7];
  char adresaMac[13];
  char IP[9];
  int estat;
  char numAleat[7];
  long timing;
};

struct PDU_UDP{
  unsigned char tipusPacket;
  char nomEquip[7];
  char adresaMac[13];
  char numAleat[7];
  char Dades[50];
};

struct PDU_TCP{
  unsigned char tipusPacket;
  char nomEquip[7];
  char adresaMac[13];
  char numAleat[7];
  char Dades[150];
};

struct PCREG *registres[NUMCOMPUTERS];
int pidWork, pidAlives, pidMsg;
struct SERVINFO serverInfo;
int debug;
char servFile[15] = "server.cfg\0";
char autorized[15] = "equips.dat\0";

ssize_t getline(char **lineptr, size_t *n, FILE *stream);
void work(int udpSocket, struct SERVINFO serverInfo, int lines);
void makeUDPPDU(unsigned char sign, struct SERVINFO serverInfo, struct PDU_UDP *sendPDU, char random[], char error[]);
void makeErrorPDU(unsigned char sign, char nom[], char mac[], struct PDU_UDP *sendPDU, char random[], char error[]);
void readServInfo(struct SERVINFO *serverInfo);
int readPermitedComputers();
int createUDPSocket();
int randomGenerator();
void modifyPCRegisters(int i, struct sockaddr_in udpRecvAdress);
void initializeRandom();
void request(int udpSocket, int lines, struct PDU_UDP recvPDU,  struct sockaddr_in udpRecvAdress, struct SERVINFO serverInfo);
void aliveResponse(int udpSocket, int lines, struct PDU_UDP recvPDU,  struct sockaddr_in udpRecvAdress, struct SERVINFO serverInfo);
void aliveStateChecker(int lines);
void handler(int sig);
int createTCPSocket();
void sendFile(int newTCPSock, struct PDU_TCP pdu_tcp, struct sockaddr_in tcpRecvAdress, struct SERVINFO serverInfo, char sndrcvFile[], char alea[]);
void getFile(int newTCPSock, struct PDU_TCP pdu_tcp, struct sockaddr_in tcpRecvAdress, struct SERVINFO serverInfo, char sndrcvFile[], char alea[]);
void makeTCPPDU(unsigned char sign,struct SERVINFO serverInfo, struct PDU_TCP *send_tcp, char random[], char dades[]);

int main(int argc, char *argv[]){
  int udpSocket;
  int lines = 0, i;
  char input[5];
  char estat[12];

  /*Reserva de espai de memória compartida per a la taula de clients*/
  for(i=0;i<NUMCOMPUTERS;i++){
 	  registres[i] = (struct PCREG *)mmap(0, sizeof(*registres[NUMCOMPUTERS]), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  }

  /*Parse de linea de comandes*/
  if(argc > 1){
    for(i=1;i < argc; i++){
      /*Parserd mode debug*/
      if(strcmp(argv[i],"-d") == 0){
        debug = 1;
        printf("Mode debug activat\n");
      }
      if(strcmp(argv[i],"-c") == 0){
        /*Parser fitxer arrancada de servidor*/
        if(i+1 < argc){
          sprintf(servFile,"%s", argv[i+1]);
          printf("Realitzat canvi en el fitxer de arrancada de servidor\n");
        }else{
          printf("Detectat -c pero cap nom darrera per defecte el fitxer de arrancada és server.cfg\n");
        }
      }
      if(strcmp(argv[i],"-u") == 0){
        /*Parser fitxer equips autoritzats*/
        if(i+1 < argc){
          sprintf(autorized,"%s", argv[i+1]);
          printf("Realitzat canvi en el fitxer d'equips autoritzats\n");
        }else{
          printf("Detectat -u però cap nom darrera per defecte el fitxer d'equips autoritzats és equips.dat\n");
        }
      }
    }
  }

  /*Lectura del fitxer d'informació del servidor*/
  readServInfo(&serverInfo);
  /*Lectura del fitxer de equips permesos*/
  lines = readPermitedComputers();

  pidWork = fork();
  if(pidWork == 0){
    if(debug == 1){
      printf("[DEBUG] -> Preparat procés per a rebre peticions TCP/UDP\n");
    }
    udpSocket = createUDPSocket();

    printf("[DEBUG] -> Preparat per a rebre regitres:\n");
    work(udpSocket,serverInfo,lines);

    exit(0);
  } else{
    if(debug == 1){
      printf("[DEBUG] -> Preparat procés per a rebre comandes\n");
    }
    while(1){
      scanf("%s",input);

      if(strcmp(input,QUIT) == 0){
        /*Fi del programa*/
        kill(pidWork,SIGTERM);
        return 0;
      }
      else if(strcmp(input,LIST) == 0){
        /*Mostra dels estats dels equips autoritzats*/
        for(i=0;i<lines;i++){
          if(registres[i] -> estat == 0){
            sprintf(estat,"%s","DISCONNECTED");
          }else if(registres[i] -> estat == 1){
            sprintf(estat,"%s","REGISTERED");
          }else if(registres[i] -> estat == 2){
            sprintf(estat,"%s","ALIVE");
          }
          printf("Nom Equip: %s\tAdreca Mac: %s\tNumero Aleatori:%s\tIP:%s\tEstat:%s\n",registres[i] -> nomEquip, registres[i] -> adresaMac, registres[i] -> numAleat, registres[i] -> IP, estat);
        }
      }
    }
  }
  return 0;
}

/*Treball a realitzar*/
void work(int udpSocket, struct SERVINFO serverInfo, int lines){
  int pidAlives, pidMsg, i;
  long timed;
  char sndrcvFile[10]= "";
  char badPack[] = "Paquet Erroni";
  char noAuto[] = "Equip no autoritzat";
  /*TCP*/
  struct PDU_TCP recv_tcp, send_tcp;
  int tcpSocket, newTCPSock, recvtcplen;
  struct sockaddr_in tcpRecvAdress;
  socklen_t addrlentcp = sizeof(tcpRecvAdress);
  /*TCP*/
  /*UDP*/
  struct PDU_UDP recvPDU;
  int recvlen;
  struct sockaddr_in udpRecvAdress;
  socklen_t addrlen = sizeof(udpRecvAdress);
  /*UDP*/
  fcntl(udpSocket, F_SETFL, O_NONBLOCK);
  initializeRandom();
  tcpSocket = createTCPSocket();
  fcntl(tcpSocket, F_SETFL, O_NONBLOCK);

  pidAlives = fork();
  if(pidAlives == 0){
    aliveStateChecker(lines);

  }else{
    pidMsg = fork();

    if(pidMsg == 0){
      int pid;

      while(1){
        recvlen = recvfrom(udpSocket, &recvPDU, BUFSIZEUDP, 0, (struct sockaddr *)&udpRecvAdress, &addrlen);
        listen(tcpSocket,lines);

        /*SECCIÓ UDP*/
        if(recvlen > 0){
          /*REGISTRE*/
          /*distribució dels paquets segons el tipus*/
          if(recvPDU.tipusPacket == REGISTER_REQ){
            pid = fork();

            /*Procés per gestionar registres*/
            if(pid == 0){
              printf("Rebut registre de %s\n",recvPDU.nomEquip);
              if(debug == 1){
                printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
              }
              request(udpSocket, lines, recvPDU,  udpRecvAdress, serverInfo);
              exit(0);
            }
          }
          /*Procés per gestionar ALIVES */
          else if(recvPDU.tipusPacket == ALIVE_INF){
            pid = fork();
            if(pid == 0){
              aliveResponse(udpSocket, lines, recvPDU, udpRecvAdress, serverInfo);
            }
          }

          /*SECCIÓ TCP*/
          /*Accepta nova conexió*/
          if((newTCPSock = accept(tcpSocket, (struct sockaddr *) &tcpRecvAdress, &addrlentcp)) > 0){
            /*Fill encarregat de processar la petició*/
            pid = fork();
            if(pid == 0){
              /*socket no bloquejant*/
              fcntl(newTCPSock, F_SETFL, O_NONBLOCK);
              timed = time(NULL);
              while(1){
                recvtcplen = recvfrom(newTCPSock, &recv_tcp, BUFSIZETCP, 0, (struct sockaddr *)&tcpRecvAdress, &addrlentcp);
                /*Si rebem un packet el processem*/
                if(recvtcplen > 0){

                  /*Comprovem que el paquet ens arriba d'un usuari autoritzat i en estat ALIVE*/
                  for(i=0; i<lines ;i++){
                    if(strcmp(registres[i] -> nomEquip,recv_tcp.nomEquip) == 0 && strcmp(registres[i] -> numAleat,recv_tcp.numAleat) == 0
                      && strcmp(registres[i] -> adresaMac,recv_tcp.adresaMac) == 0 && registres[i] -> estat == 2){

                        strcat(sndrcvFile,recv_tcp.nomEquip);
                        strcat(sndrcvFile,".cfg\0");

                      /*Mirem si hem de enviar o rebre un fixter*/
                      if(recv_tcp.tipusPacket == GET_FILE){
                        /*Fem la PDU per a TCP*/
                        makeTCPPDU(GET_ACK, serverInfo, &send_tcp, recvPDU.numAleat, sndrcvFile);
                        /*Enviem la PDU accpetant la petició*/
                        sendto(newTCPSock, &send_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlentcp);
                        /*Enviem el fitxer*/
                        getFile(newTCPSock, recv_tcp, tcpRecvAdress, serverInfo, sndrcvFile, recvPDU.numAleat);
                        /*Tanquem socket*/
                        close(newTCPSock);
                      }
                      else if(recv_tcp.tipusPacket == SEND_FILE){
                        /*Fem la PDU per a TCP*/
                        makeTCPPDU(SEND_ACK, serverInfo, &send_tcp, recvPDU.numAleat, sndrcvFile);
                        /*Enviem la PDU accpetant la petició*/
                        sendto(newTCPSock, &send_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlentcp);
                        /*Rebem el fitxer*/
                        sendFile(newTCPSock, recv_tcp, tcpRecvAdress, serverInfo, sndrcvFile, recvPDU.numAleat);
                      }
                      else{
                        printf("Paquet Erroni\n");
                        /*Fem la PDU per a TCP*/
                        makeTCPPDU(SEND_NACK, serverInfo, &send_tcp, recvPDU.numAleat, badPack);
                        /*Enviem la PDU amb l'error*/
                        sendto(newTCPSock, &send_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlentcp);
                        close(newTCPSock);
                      }
                      exit(0);
                    }
                  }
                  if(lines == i){
                    /*Si em comprobat tots els equips */
                    printf("Equip no autoritzat\n");
                    /*Fem la PDU per a TCP*/
                    makeTCPPDU(SEND_NACK, serverInfo, &send_tcp, recvPDU.numAleat, noAuto);
                    /*Enviem la PDU amb l'error*/
                    sendto(newTCPSock, &send_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlentcp);
                    if(debug == 1){
                      printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recv_tcp.nomEquip, recv_tcp.adresaMac,recv_tcp.numAleat,recv_tcp.Dades);
                      printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_REJ NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",send_tcp.nomEquip, send_tcp.adresaMac,send_tcp.numAleat,send_tcp.Dades);
                    }
                    close(newTCPSock);
                    exit(0);
                  }
                }
                else  if(time(NULL)-timed > WAITTIME){
                  /*Si em comprobat tots els equips */
                  printf("No hi ha comunicació amb el client\n");
                  /*Fem la PDU per a TCP*/
                  makeTCPPDU(SEND_NACK, serverInfo, &send_tcp, recvPDU.numAleat, noAuto);
                  /*Enviem la PDU amb l'error*/
                  sendto(newTCPSock, &send_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlentcp);
                  close(newTCPSock);
                  exit(0);
                }
              }
            }
          }
        }
      }
    }
    while(1){
      signal(SIGTERM,handler);
    }
  }
}

/*Peticions de registre*/
void request(int udpSocket, int lines, struct PDU_UDP recvPDU,  struct sockaddr_in udpRecvAdress, struct SERVINFO serverInfo){
  int i;
  char random[7] = "",error[50] = "";
  struct PDU_UDP sendPDU;
  socklen_t addrlen = sizeof(udpRecvAdress);
  /*Comprovació de existencia del equip dins de'ls equips registrats*/
  for(i = 0; i < lines; i++){
    if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0){

      if(registres[i] -> estat == STATE_DISCON && (strcmp("000000",recvPDU.numAleat) == 0 || strcmp(registres[i] -> numAleat,recvPDU.numAleat) == 0)){

          /*Enviament de numero aleatori depenen de si en te un asignat o no*/
          if(strcmp(registres[i] -> numAleat,"000000") == 0){
            modifyPCRegisters(i, udpRecvAdress);
          }

          makeUDPPDU(REGISTER_ACK, serverInfo, &sendPDU, registres[i] -> numAleat, serverInfo.TCP);
          sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);

          registres[i] -> timing = time(NULL);
          registres[i] -> estat = STATE_REGIST;

          printf("L'equip %s passa a l'estat REGISTRAT\n",registres[i] -> nomEquip);
          break;

          /*En cas de que l'equip estigui conectat enviament de REGISTER REJ*/
      }else if(registres[i] -> estat == STATE_REGIST || registres[i] -> estat == STATE_ALIVE){

        sprintf(error,"%s","Equip conectat");
        makeUDPPDU(REGISTER_NACK, serverInfo, &sendPDU, random, error);
        sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      }
    }
  }
  /*Si despres de comprovar els equips no es troba el que ha enviat la PDU es torna un error*/
  if(i == lines){

    strcpy(error, "Equip no autoritzat en el sistema\0");
    makeUDPPDU(REGISTER_REJ, serverInfo, &sendPDU, random, error);
    sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
  }
}

/*Modificar els valors de IP i numero aleatori del equip registrat*/
void modifyPCRegisters(int i, struct sockaddr_in udpRecvAdress){
  char random[7];

  sprintf(random,"%d",randomGenerator());
  registres[i] -> estat = 1;
  sprintf(registres[i] -> numAleat,"%6s", random);
  sprintf(registres[i] -> IP,"%9s", inet_ntoa(udpRecvAdress.sin_addr));
}

/*generador del nombre aleatori*/
int randomGenerator(){
  int random;
  srand(time(NULL));
  random = rand()%900000+100000;

  return random;
}

/*Generador de la PDU de protocol UDP*/
void makeUDPPDU(unsigned char sign,struct SERVINFO serverInfo, struct PDU_UDP *sendPDU, char random[], char dades[]){
  sendPDU -> tipusPacket = sign;
  strcpy(sendPDU -> nomEquip,serverInfo.nomServ);
  strcpy(sendPDU -> adresaMac,serverInfo.MAC);
  strcpy(sendPDU -> numAleat,random);
  strcpy(sendPDU -> Dades, dades);
}

/*Llegeix i agafa les dades del fitxer de informacio del servidor*/
void readServInfo(struct SERVINFO *serverInfo){
  FILE *f;
  char ignore[5];
  f = fopen(servFile, "r");
  if(f == NULL){
    printf("El fitxer no existeix");
    exit(0);
  }else{
    /*Llegir la informació del servidor*/
    while(!feof(f)){
      fscanf(f, "%s %6s",ignore, serverInfo -> nomServ);
      fscanf(f, "%s %12s",ignore, serverInfo -> MAC);
      fscanf(f, "%s %5s",ignore, serverInfo -> UDP);
      fscanf(f, "%s %5s",ignore, serverInfo -> TCP);
    }

    fclose(f);
  }
}

/*Lleigeix i agafa les dades del fixer de equips autoritzats*/
int readPermitedComputers(){
  FILE *f;
  int i = 0;
  f = fopen(autorized, "r");

  /*Mentres no arribem a fí de fitxer guardem les dades*/
  while(!feof(f)){
    fscanf(f,"%6s %12s", registres[i] -> nomEquip, registres[i] -> adresaMac);
    i++;
  }

  return i-1;
}

/*Creacio del socket UDP*/
int createUDPSocket(){
  int udpSocket;
  struct sockaddr_in udpSockAdress;

  /*creació de socket de protocol UDP*/
  udpSocket = socket(AF_INET,SOCK_DGRAM,0);

  if(udpSocket < 0){
    printf("Error Opening Socket");
  }

  memset((char *)&udpSockAdress, 0, sizeof(udpSockAdress));
  udpSockAdress.sin_family = AF_INET;
  udpSockAdress.sin_addr.s_addr = htonl(INADDR_ANY);
  udpSockAdress.sin_port = htons(2016);

  /*Assignació de socket a adreça*/
  if(bind(udpSocket, (struct sockaddr *)&udpSockAdress, sizeof(udpSockAdress)) < 0){
    perror("bind failed");
    return 0;
  }
  return udpSocket;
}

/*Inicialitza el numero aleatori per al equip que es conecta*/
void initializeRandom(){
  int i;
  for( i = 0; i < NUMCOMPUTERS; i++){
    strcpy(registres[i] -> numAleat, "000000");
  }
}

/*Resposta als ALIVES REBUTS*/
void aliveResponse(int udpSocket, int lines, struct PDU_UDP recvPDU,  struct sockaddr_in udpRecvAdress, struct SERVINFO serverInfo){
  int i;
  char error[50] = "";
  char alea[7] = "";
  char nom[7] = "";
  char mac[13] = "";
  struct PDU_UDP sendPDU;
  socklen_t addrlen = sizeof(udpRecvAdress);

  /*Comprovem si l'equip esta autoritzat i el seu estat es Registered o ALIVE*/
  for(i=0; i<lines ;i++){
    /*Si l'equip es correcte i esta ALIVE enviem ALIVE_ACK*/
    if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> numAleat,recvPDU.numAleat) == 0
      && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0 && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) == 0
      && (registres[i] -> estat == STATE_REGIST || registres[i] -> estat == STATE_ALIVE)){

      if(registres[i] -> estat == STATE_REGIST){
        registres[i] -> timing = time(NULL);
        registres[i] -> estat = STATE_ALIVE;

        makeUDPPDU(ALIVE_ACK, serverInfo, &sendPDU, registres[i] -> numAleat , error);
        sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
        printf("Confirmacio ALIVE  %s\n",registres[i] ->nomEquip);
        if(debug == 1){
          printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
          printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_ACK NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
        }

      }else if (registres[i] -> estat == STATE_ALIVE){
        registres[i] -> timing = time(NULL);
        makeUDPPDU(ALIVE_ACK, serverInfo, &sendPDU, registres[i] -> numAleat , error);
        sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);

        printf("Confirmacio ALIVE  %s\n",registres[i] ->nomEquip);
        if(debug == 1){
          printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
          printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_ACK NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
        }

      }
      break;

    }

    /*Si l'equip esta desconectat enviem un ALIVE_REJ informant de l'error*/
    else if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0
    && registres[i] -> estat == STATE_DISCON){
      strcpy(error,"Equip no Registrat");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");;

      makeErrorPDU(ALIVE_REJ,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      if(debug == 1){
        printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
        printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_ACK NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
      }
      exit(0);
    }

    /*Si el numero aleatori es incorrecte informem*/
    else if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0
    && registres[i] -> estat == STATE_REGIST && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) == 0 && strcmp(registres[i] -> numAleat, recvPDU.numAleat) != 0){
      strcpy(error,"Numero Aleatori Incorrecte");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");

      makeErrorPDU(ALIVE_NACK,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      if(debug == 1){
        printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
        printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_NACK NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
      }
      exit(0);
    }

    /*Si la IP es incorrecta informem*/
    else if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0
    && registres[i] -> estat == STATE_REGIST && strcmp(registres[i] -> numAleat, recvPDU.numAleat) == 0
    && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) != 0){
      strcpy(error,"IP incorrecta");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");

      makeErrorPDU(ALIVE_NACK,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      if(debug == 1){
        printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
        printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_NACK NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
      }
      exit(0);
    }

    /*Si la IP es incorrecta informem*/
    else if(strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0 && registres[i] -> estat == STATE_REGIST &&
    strcmp(registres[i] -> numAleat, recvPDU.numAleat) == 0  && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) == 0
    && strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) != 0){
      strcpy(error,"Nom Equip incorrecte");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");

      makeErrorPDU(ALIVE_REJ,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      if(debug == 1){
        printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
        printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_REJ NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
      }
      exit(0);
    }
  }
  /*Si em comprovat tots els equips i no em trobat el que ha enviat l'ALIVE enviem un ALIVE REJ i informem de l'error*/
  if(i == lines){
    strcpy(error,"Equip no autoritzat en el sistema");
    strcpy(alea,"000000");
    strcpy(mac,"000000000000");
    strcpy(nom,"");

    makeErrorPDU(ALIVE_REJrecvPDU = 0;
    registres[i] -> estat = STATE_DISCON;
    if(debug == 1){
      printf("[DEBUG] -> REBUT: NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",recvPDU.nomEquip, recvPDU.adresaMac,recvPDU.numAleat,recvPDU.Dades);
      printf("[DEBUG] -> ENVIAT: PAQUET: ALIVE_REJ NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",sendPDU.nomEquip, sendPDU.adresaMac,sendPDU.numAleat,sendPDU.Dades);
    }
  }
  exit(0);
}

/*PDU en cas de error*/
void makeErrorPDU(unsigned char sign, char nom[], char mac[], struct PDU_UDP *sendPDU, char random[], char error[]){
  sendPDU -> tipusPacket = sign;
  strcpy(sendPDU -> nomEquip, nom);
  strcpy(sendPDU -> adresaMac, mac);
  strcpy(sendPDU -> numAleat,random);
  strcpy(sendPDU -> Dades, error);
}

/*Comprovacio de estat d'alive*/
void aliveStateChecker(int lines){
  int i;
  int limAlive = 9;
  int firstAlive = 6;
  long now,diff;
  long timed = time(NULL);
  /*Mentres el servidor estigui funcionan comprova dels equips registrats*/
  while(1){
    if(time(NULL)-timed >= 1){
      now = time(NULL);
      for(i = 0;i<lines;i++){
        diff = now - registres[i] -> timing;

        /*Si no s'ha rebut el primer ALIVE en 6 segons(estat del client Registered) es desconecta*/
        if(registres[i] -> estat == 1 && diff > firstAlive){
          registres[i] -> estat = STATE_DISCON;
          registres[i] -> timing = 0;
          printf("%s Desconectat, Alives no rebuts\n",registres[i] -> nomEquip);
        }

        /*Si no s'han rebut un ALIVE en 9 segons(estat del client ALIVE) es desconecta*/
        else if(registres[i] -> estat == 2 && diff > limAlive){
          registres[i] -> estat = STATE_DISCON;
          registres[i] -> timing = 0;
          printf("%s Desconectat, Alives no rebuts\n",registres[i] -> nomEquip);
        }
      }

      timed = time(NULL);
    }
  }
}

/*Control de senyal*/
void handler(int sig){
  kill(pidAlives,SIGINT);
  kill(pidMsg,SIGINT);
  exit(0);
}

/*Creacio conexió TCP*/
int createTCPSocket(){
  int tcpSocket, portnum;
  struct sockaddr_in serv_addr;

  /*Creació del socket de protocol*/
  tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

  bzero((char *) &serv_addr, sizeof(serv_addr));
  portnum = 6102 ;

  /*Adreça de binding (escolta) -> totes les disponibles */
   memset(&serv_addr,0,sizeof (struct sockaddr_in));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portnum);

   /*Assignació del socket a la adreça*/
   if (bind(tcpSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }

   return tcpSocket;
}

/*Creació de PDU per TCP*/
void makeTCPPDU(unsigned char sign,struct SERVINFO serverInfo, struct PDU_TCP *send_tcp, char random[], char dades[]){
  send_tcp -> tipusPacket = sign;
  strcpy(send_tcp -> nomEquip,serverInfo.nomServ);
  strcpy(send_tcp -> adresaMac,serverInfo.MAC);
  strcpy(send_tcp -> numAleat,random);
  strcpy(send_tcp -> Dades, dades);
}

/*Rebre fitxer de configuració*/
void sendFile(int newTCPSock, struct PDU_TCP pdu_tcp, struct sockaddr_in tcpRecvAdress, struct SERVINFO serverInfo, char sndrcvFile[], char alea[]){
  FILE *f;
  long timed;
  int recvtcplen;
  socklen_t addrlentcp = sizeof(tcpRecvAdress);
  f = fopen(sndrcvFile, "w+");
  printf("Inici de recepció del fitxer de configuració\n");
  timed = time(NULL);

  while(time(NULL)-timed < 4){
    recvtcplen = recvfrom(newTCPSock, &pdu_tcp, BUFSIZETCP, 0, (struct sockaddr *)&tcpRecvAdress, &addrlentcp);
    if(recvtcplen > 0){
      /*Mentres rebem SEND DATA escribim les dades al fitxer*/
      while(pdu_tcp.tipusPacket == SEND_DATA){
        if(recvtcplen > 0){
          timed = time(NULL);
          if(debug == 1){
            printf("[DEBUG] -> REBUT: PAQUET: SEND_DATA NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",pdu_tcp.nomEquip, pdu_tcp.adresaMac,pdu_tcp.numAleat,pdu_tcp.Dades);
          }
          fprintf(f,"%s",pdu_tcp.Dades);
        }else if(time(NULL)-timed > 4){
          /*Si no es rep una */
          printf("Error en la recepció de paquet\n");
          close(newTCPSock);
          fclose(f);
          exit(0);
        }
        recvtcplen = recvfrom(newTCPSock, &pdu_tcp, BUFSIZETCP, 0, (struct sockaddr *)&tcpRecvAdress, &addrlentcp);
      }
      if(pdu_tcp.tipusPacket == SEND_END){
        if(debug == 1){
          printf("[DEBUG] -> REBUT: PAQUET: SEND_END NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",pdu_tcp.nomEquip, pdu_tcp.adresaMac,pdu_tcp.numAleat,pdu_tcp.Dades);
        }
        printf("Finalització recepció arxiu\n");
        close(newTCPSock);
        fclose(f);
        exit(0);
      }else{
        printf("No s'ha rebut SEND_END\n");
        close(newTCPSock);
        fclose(f);
        exit(0);
      }
    }
  }
}

/*Enviar fitxer de configuració*/
void getFile(int newTCPSock, struct PDU_TCP pdu_tcp, struct sockaddr_in tcpRecvAdress, struct SERVINFO serverInfo, char sndrcvFile[], char alea[]){
  char *toSend = NULL;
  char endmsg[] = "";
  size_t len = 0;
  int nbytes;
  socklen_t addrlen = sizeof(tcpRecvAdress);
  FILE *f;
  f = fopen(sndrcvFile, "r");

  if(f == NULL){
    printf("Fitxer inexistent");
  }
  else{
    printf("Enviant fitxer de configuració\n");
    while ((nbytes = getline(&toSend, &len, f)) != -1){
      makeTCPPDU(GET_DATA, serverInfo, &pdu_tcp, alea, toSend);
      sendto(newTCPSock, &pdu_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlen);
      if(debug == 1){
        printf("[DEBUG] -> ENVIANT: PAQUET: GET_DATA NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",pdu_tcp.nomEquip, pdu_tcp.adresaMac,pdu_tcp.numAleat,pdu_tcp.Dades);
      }
    }
    makeTCPPDU(GET_END, serverInfo, &pdu_tcp, alea, endmsg);
    sendto(newTCPSock, &pdu_tcp, BUFSIZETCP,0, (struct sockaddr *)&tcpRecvAdress, addrlen);
    if(debug == 1){
      printf("[DEBUG] -> ENVIANT: PAQUET: GET_END NOM: %s  MAC: %s ALEA: %s  DADES: %s\n",pdu_tcp.nomEquip, pdu_tcp.adresaMac,pdu_tcp.numAleat,pdu_tcp.Dades);
    }
    printf("Finalitzat enviament de fitxer de configuració\n");
    fclose(f);
  }
}
