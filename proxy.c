#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <regex.h>


#define PACKAGE_LENGTH 4096 // Attention si vous augmentez l taille du uffer vous ne recevrez pas les paquest du navigateur
#define HTTP_PORT 80
#define HTTPS_PORT 443


char *strChange(char *orig, char *rep, char *with) { //fonction qui remplace une chaine de caractère par une autres dans un char*
    char *result; 
    char *ins;    
    char *tmp;    
    int len_rep;  
    int len_with; 
    int len_front; 
    int count;

    if (!orig)
      return NULL;
    if (!rep)
      rep = "";
    len_rep = strlen(rep);
    if (!with)
      with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
      ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
      return NULL;

    while (count--) {
      ins = strstr(orig, rep);
      len_front = ins - orig;
      tmp = strncpy(tmp, orig, len_front) + len_front;
      tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
      }
      strcpy(tmp, orig);
      return result;
    }

void ThrowError(char *err){
	perror(err);
	exit(1);
}


int init(int sock, struct sockaddr_in addr_server, int port){ // Initialisation du proxy socket
  printf("\n\n- - - - - - - - - - - - - - - - - - - - - - -");
  printf("\n- - - - - - - - Proxy MyAdBlock - - - - - - - -");
  printf("\n- - - - - - - - - - - BY - - - - - - - - - - - ");
  printf("\n- - - - - - - - -  TOM BARAT - - - - - - - - -");
  printf("\n- - - - - - - - - - - AND - - - - - - - - - - -");
  printf("\n- - - - - - - - VINCENT NAHMIAS - - - - - - - - ");
  printf("\n- - - - - Because F*** ADVERTISING - - - - - - -");
  printf("\n- - - - - - - - - - - - - - - - - - - - - - - -\n\n");

  addr_server.sin_family = AF_INET; // AF_INET: domaine IP
  addr_server.sin_addr.s_addr = INADDR_ANY; //  INADDR_ANY: notre proxy traite toutes les adresses
  addr_server.sin_port = htons(port);
  

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <0) {  //création de la socket
    ThrowError("Error : Can't create the proxy socket\n");
  }


  //lien entre le socket et l'adresse
  if (bind(sock, (struct sockaddr *) &addr_server, sizeof(addr_server)) < 0) {
    ThrowError("Error : Can't bind proxy socket to server adress\n");
  }

  // réglage des options du proxy socket
  if(listen(sock,SOMAXCONN) <0){
    ThrowError("Error : Can't listen on proxy socket\n");
  }
  return sock;
}

// vérification du nombre d'arguments
void checkArg (int nb){
	if(nb!=2){
		ThrowError("Error : Wrong number of arguments. The program only expects port number as argument\n");
	}
}


void sendToBrowser(int socket, char *buffer, int sock_client, int n){

  n=send(socket,buffer,strlen(buffer),0);  //envoie du buffer au serveur.
  if(n<0){
    ThrowError("Error : can't send buffer to server\n");
  }else{
    do
    {
      bzero((char*)buffer,sizeof(buffer));
      n=recv(socket,buffer,sizeof(buffer),0); //réception du buffer depuis le serveur
      if(!(n<=0))
      send(sock_client,buffer,n,0);  //envoi du buffer au navigateur
     }while(n>0);
  }
}

//fonction de blocage par adresses
int AddrBlock(char *addr){
  FILE* file = NULL;
  regex_t regtest;

  file = fopen("easyList.txt", "r");
  if(file==NULL){
    ThrowError("Error : Can't open rules file");
  }
	char line[200]="";
	char *buf=NULL;
	while(fgets(line,200,file)!=NULL){ // parcours de toutes les lignes du fichier
		if(line[0]!='#' && line[0]!='|' && line[0]!='@'){ //Si c'est une règle qui concerne les url
			 if(strstr(addr,line)!=NULL){ //Si l'adresse contient l'élément présent sur la ligne
         		printf("\nligne qu'on test addr : %s",line);
         		printf("\n on est sensé avoir blocage");
			 	return(1);
			 }
		}else if(line[0]=='|' && line[1]!='|'){ //blocage par adresse exacte
			if(strcmp(line,addr)==0){
				return(1);
			}
		}
		

	}
	return(0);

} 



int HostBlock(char* host){
  FILE* file = NULL;

  file = fopen("easyList.txt", "r");
  if(file==NULL){
    ThrowError("Error : Can't open rules file");
}
  char line[200]="";
  char *buff=NULL;
  while(fgets(line,200,file)!=NULL){// parcours de toutes les lignes du fichier
    if (line[0]=='|' && line[1]=='|'){//Si c'est une règle qui concerne les nom d'hôtes

      buff=strtok(line, "||");
      buff=strtok(buff,"^");

      if (strstr(host,buff)!=NULL){
        printf("\nbuffer : %s\n",buff);

        return(1);
      }
    }
  }
  return(0); 
    
}

int main(int argc, char *argv[]){

  struct sockaddr_in addr_client;
  struct addrinfo *res;
  struct addrinfo *p;
  struct sockaddr_in addr_server; 
  int sockProxy=0;
  int sockClient=0; // création des deux sockets correspondants au navigateur et au proxy coté serveur
  int state=0; 
  int port;
  char ipstring[INET_ADDRSTRLEN]; //ipstring = addresse IP
  char* tmpr=NULL;

  char ipver; 
  pid_t pid; // n°pid pour le fork
  void *addr=NULL;
  
 
  memset((char *) &addr_server, 0, sizeof(addr_server));
  memset((char *) &addr_client, 0, sizeof(addr_client));

  port = atoi(argv[1]); 
  checkArg(argc);
  sockProxy=init(sockProxy,addr_server, port); // initialisation du proxy Socket
  
  int clieServLen=sizeof(addr_client);
  
  while(1){

    sockClient = accept(sockProxy,(struct sockaddr *) &addr_client, (socklen_t *)&clieServLen);
    if( sockClient <0){
      ThrowError("Error : Can't accept client socket on proxy socket\n");
    }
    
    pid=fork(); // utilisation du fork pour serveur multiclient
    if (pid==0){
      struct sockaddr_in *IP;
      struct addrinfo proxy_addr;

      int n=0;
      int newsockfd=0; //création des deux autres sockets correspondant à celle du serveur et à celle du proxy côté navigateur
      int port =80;
      int sockfd=0;
      int path_length;

      char send_buffer[PACKAGE_LENGTH];
      char web_url[200];
      char *client_buffer=NULL;
      char* host=NULL;

      char* tmp=NULL;
      char protocol_name[10];
      char url_parser[300];
      char *tmp_buffer=NULL; 
      char url[500]; //url de la requête
      char *path=NULL;  //chemin de chaque élément de la requête
      



      memset(&IP, 0, sizeof(IP));
      memset((char *) send_buffer, 0, sizeof(send_buffer));
      

      if ( (n= recv(sockClient,send_buffer,sizeof(send_buffer),0 ))<0 )  {
        ThrowError("Error : Can't receive buffer from client\n");
      }

    	sscanf(send_buffer,"%s %s %s",url_parser,web_url,protocol_name); //Parsing de la requête GET
    	strcpy(url,web_url); 
    
    	if(((strncmp(protocol_name,"HTTP/1.1",8)==0)||(strncmp(protocol_name,"HTTP/1.0",8)==0))&&((strncmp(web_url,"http://",7)==0))) //traitement de la requête GET
    	{
		      port=80;
		      strcpy(url_parser,web_url);
		      tmp=strtok(web_url,"//");
		      
		      tmp=strtok(NULL,"/");
		      strcpy(web_url,tmp);

		      strcat(url_parser,"::");
		      tmp=strtok(NULL,"::");

		      if(tmp!=NULL) { // on doit rajouter un "/" car le parse de la requête le supprime.
		        path_length = strlen(tmp) + 2;
		      	path = (char*)malloc(path_length * sizeof(char));
		      	*path = '/';
		      	strcat(path, tmp);
      }
      memset(&proxy_addr, 0, sizeof(proxy_addr));
      proxy_addr.ai_family=AF_INET;
      proxy_addr.ai_socktype = SOCK_STREAM;

      
      if ((state = getaddrinfo(web_url,NULL, &proxy_addr, &res)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(state));
        return 2;
      }

      p=res;
      while(p!=NULL){
      // Identification de l'adresse courante
        IP = (struct sockaddr_in *)p->ai_addr; //IP addr
        IP->sin_family=AF_INET; //IP family
        if(port==80){
          IP-> sin_port = htons(HTTP_PORT); //port HTTP
        }
        addr = &(IP->sin_addr);
        inet_ntop(p->ai_family, addr, ipstring, INET_ADDRSTRLEN); // transforme l'adresse IP en char
        
      
      
      p = p->ai_next;
    	}

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <0) { 
          ThrowError("Error : Can't create proxy socket for IP\n");
        }
        if((newsockfd=connect(sockfd,(struct sockaddr*)IP,sizeof(struct sockaddr)))<0){
          ThrowError("Error : Can't connect client socket to server socket\n");
        }

        if((tmp!=NULL) || (port ==80)){  //Si cc'est une page HTTP

          if(path==NULL){
            path="/";
          }

          tmp_buffer = strChange(send_buffer,url, path);

          client_buffer = strChange(tmp_buffer, "keep-alive", "close"); // remplace keep alive par close dans le buffer
          int buff_len = strlen(client_buffer);
          tmpr = (char*)malloc(buff_len * sizeof(char));
          strcpy(tmpr,client_buffer);

          host=strtok(tmpr,"User-Agent: ");
          host= strtok(NULL,"\n");
          host= strtok(NULL,"\n");
          host= strtok(host," :");
          host = strtok(NULL," :");
          if(AddrBlock(url)==0 && HostBlock(host)==0){
            sendToBrowser(sockfd,client_buffer,sockClient,n);
          }else{
            printf("\n\n----------------------------------------------------------------------------------\n\n");
          }

          
        }
	    }
	    close(sockClient);
	    close(sockfd);  //fermeture des sockets
	    close(sockProxy);
	    _exit(0);
	} else {

    close(sockClient);

  }
}

return 0;
}
