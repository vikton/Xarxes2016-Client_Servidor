
#define _POSIX_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <time.h>
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
#define SEND_DETA 0x24
#define SEND_END 0x25

/*GET CONF*/
#define GET_FILE 0x30
#define GET_ACK 0x31
#define GET_NACK 0x32
#define GET_REJ 0x33
#define GET_DETA 0x34
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

int main(int argc, char *argv[]){
  int udpSocket;
  int lines = 0, i;
  char input[5];
  char estat[12];

  for(i=0;i<NUMCOMPUTERS;i++){
 	  registres[i] = (struct PCREG *)mmap(0, sizeof(*registres[NUMCOMPUTERS]), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  }

  readServInfo(&serverInfo);
  lines = readPermitedComputers();

  pidWork = fork();
  if(pidWork == 0){

    udpSocket = createUDPSocket();

    printf("Preparat per a rebre regitres:\n");
    work(udpSocket,serverInfo,lines);

    exit(0);
  } else{
    while(1){
      scanf("%s",input);

      if(strcmp(input,QUIT) == 0){
        kill(pidWork,SIGTERM);
        return 0;
      }
      else if(strcmp(input,LIST) == 0){
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
  struct PDU_UDP recvPDU;
  struct sockaddr_in udpRecvAdress;
  int recvlen, pidAlives, pidMsg;
  socklen_t addrlen = sizeof(udpRecvAdress);
  fcntl(udpSocket, F_SETFL, O_NONBLOCK);
  initializeRandom();

  pidAlives = fork();

  if(pidAlives == 0){
    aliveStateChecker(lines);
  }else{

    pidMsg = fork();

    if(pidMsg == 0){
      int pid;
      while(1){
        recvlen = recvfrom(udpSocket, &recvPDU, BUFSIZEUDP, 0, (struct sockaddr *)&udpRecvAdress, &addrlen);
        if(recvlen > 0){
          /*REGISTRE*/
          if(recvPDU.tipusPacket == REGISTER_REQ){
            pid = fork();
            if(pid == 0){
              printf("Rebut registre de %s\n",recvPDU.nomEquip);
              request(udpSocket, lines, recvPDU,  udpRecvAdress, serverInfo);
              exit(0);
            }
          }
          else if(recvPDU.tipusPacket == ALIVE_INF){
            pid = fork();
            if(pid == 0){
              aliveResponse(udpSocket, lines, recvPDU, udpRecvAdress, serverInfo);
            }
          }
          else if(recvPDU.tipusPacket == SEND_FILE){
            pid = fork();
            if(pid == 0){

            }
          }
          else if(recvPDU.tipusPacket == GET_FILE){
            pid = fork();
            if(pid == 0){

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

/*Llegeix i agafa les dades del fitxer de informacio de la PDU*/
void readServInfo(struct SERVINFO *serverInfo){
  FILE *f;
  char ignore[5];
  f = fopen("server.cfg", "r");

  while(!feof(f)){
    fscanf(f, "%s %6s",ignore, serverInfo -> nomServ);
    fscanf(f, "%s %12s",ignore, serverInfo -> MAC);
    fscanf(f, "%s %5s",ignore, serverInfo -> UDP);
    fscanf(f, "%s %5s",ignore, serverInfo -> TCP);
  }

  fclose(f);
}

/*Lleigeix i agafa les dades del fixer de equips autoritzats*/
int readPermitedComputers(){
  FILE *f;
  int i = 0;
  f = fopen("equips.dat", "r");

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

  udpSocket = socket(AF_INET,SOCK_DGRAM,0);

  if(udpSocket < 0){
    printf("Error Opening Socket");
  }

  memset((char *)&udpSockAdress, 0, sizeof(udpSockAdress));
  udpSockAdress.sin_family = AF_INET;
  udpSockAdress.sin_addr.s_addr = htonl(INADDR_ANY);
  udpSockAdress.sin_port = htons(2016);


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
      && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0 && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) == 0){

      if(registres[i] -> estat == STATE_REGIST){
        registres[i] -> timing = time(NULL);
        registres[i] -> estat = STATE_ALIVE;


        makeUDPPDU(ALIVE_ACK, serverInfo, &sendPDU, registres[i] -> numAleat , error);
        sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
        printf("Confirmacio ALIVE  %s\n",registres[i] ->nomEquip);

      }else if (registres[i] -> estat == STATE_ALIVE){
        registres[i] -> timing = time(NULL);
        makeUDPPDU(ALIVE_ACK, serverInfo, &sendPDU, registres[i] -> numAleat , error);
        sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);

        printf("Confirmacio ALIVE  %s\n",registres[i] ->nomEquip);

      }
      break;

    }

    /*Si l'equip esta desconectat enviem un ALIVE_REJ informant de l'error*/
    else if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0
    && registres[i] -> estat == STATE_DISCON){
      strcpy(error,"Equip no Registrat");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");
      strcpy(alea,"000000");

      makeErrorPDU(ALIVE_REJ,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      exit(0);
    }

    /*Si el numero aleatori es incorrecte informem*/
    else if(strcmp(registres[i] -> nomEquip,recvPDU.nomEquip) == 0 && strcmp(registres[i] -> adresaMac,recvPDU.adresaMac) == 0
    && registres[i] -> estat == STATE_REGIST && strcmp(registres[i] -> IP, inet_ntoa(udpRecvAdress.sin_addr)) == 0 && strcmp(registres[i] -> numAleat, recvPDU.numAleat) != 0){
      strcpy(error,"Numero Aleatori Incorrecte");
      strcpy(alea,"000000");
      strcpy(mac,"000000000000");
      strcpy(nom,"");
      strcpy(alea,"000000");

      makeErrorPDU(ALIVE_NACK,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
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
      strcpy(alea,"000000");

      makeErrorPDU(ALIVE_NACK,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
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
      strcpy(alea,"000000");

      makeErrorPDU(ALIVE_REJ,nom, mac, &sendPDU, alea , error);
      sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
      exit(0);
    }
  }
  /*Si em comprovat tots els equips i no em trobat el que ha enviat l'ALIVE enviem un ALIVE REJ i informem de l'error*/
  if(i == lines){
    strcpy(error,"Equip no autoritzat en el sistema");
    strcpy(alea,"000000");
    strcpy(mac,"000000000000");
    strcpy(nom,"");

    makeErrorPDU(ALIVE_REJ,nom, mac, &sendPDU, alea , error);
    sendto(udpSocket, &sendPDU, BUFSIZEUDP,0, (struct sockaddr *)&udpRecvAdress, addrlen);
    registres[i] -> timing = 0;
    registres[i] -> estat = STATE_DISCON;
  }
  exit(0);
}

/*PDUS PER ERRORS*/
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

        if(registres[i] -> estat == 1 && diff > firstAlive){
          registres[i] -> estat = STATE_DISCON;
          registres[i] -> timing = 0;
          printf("%s Desconectat, Alives no rebuts\n",registres[i] -> nomEquip);
        }
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

  tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

  bzero((char *) &serv_addr, sizeof(serv_addr));
  portnum = 6102 ;

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portnum);

   /* Now bind the host address using bind() call.*/
   if (bind(tcpSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }

   return tcpSocket;
}
