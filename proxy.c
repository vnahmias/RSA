/*
 * proxyHTTP.c
 *
 *  Created on: 9 avr. 2016
 *      Author: Denzel And Gauthey
 */

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


#define PACKAGE_LENGTH 4096 // be careful, if you increase the buffer's size, you won't receive packets from the browser
#define HTTP_PORT 80
#define HTTPS_PORT 443


char *strChange(char *orig, char *rep, char *with) { // function to replace a char * to an other in the buffer (Source : http://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c)
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

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
        orig += len_front + len_rep; // move to next "end of rep"
      }
      strcpy(tmp, orig);
      return result;
    }

void ThrowError(char *err){
	perror(err);
	exit(1);
}


int init(int sock, struct sockaddr_in addr_server, int port){ // function to "prepare" the sockProxy 
  printf("\n\n- - - - - - - - - - - - - - - - - - - - - - -");
  printf("\n- - - - - - - - Proxy MyAdBlock - - - - - - - -");
  printf("\n- - - - - - - - - - - BY - - - - - - - - - - - ");
  printf("\n- - - - - - - - -  TOM BARAT - - - - - - - - -");
  printf("\n- - - - - - - - - - - AND - - - - - - - - - - -");
  printf("\n- - - - - - - - VINCENT NAHMIAS - - - - - - - - ");
  printf("\n- - - - - Because F*** ADVERTISING - - - - - - -");
  printf("\n- - - - - - - - - - - - - - - - - - - - - - - -\n\n");

  addr_server.sin_family = AF_INET; // AF_INET: IP domain
  addr_server.sin_addr.s_addr = INADDR_ANY; //  INADDR_ANY: our proxy accept to deal with any address
  addr_server.sin_port = htons(port); // link addr_server port to the port decided at the exec of the proxy
  
  //we set our socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <0) {  // create the socket
    ThrowError("Error : Can't create the proxy socket\n");
  }


  // we link this socket to addresses defined
  if (bind(sock, (struct sockaddr *) &addr_server, sizeof(addr_server)) < 0) {  // function bind
    ThrowError("Error : Can't bind proxy socket to server adress\n");
  }

  // we set options of this proxy socket
  if(listen(sock,SOMAXCONN) <0){  // function listen
    ThrowError("Error : Can't listen on proxy socket\n");
  }
  return sock;
}

void checkArg (int nb){
	if(nb!=2){
		ThrowError("Error : Wrong number of arguments. The program only expects port number as argument\n");
	}
}

void sendToBrowser(int socket, char *buffer, int sock_client, int n){

  n=send(socket,buffer,strlen(buffer),0);  //send buffer to server
  if(n<0){
    ThrowError("Error : can't send buffer to server\n");
  }else{
    do
    {
      bzero((char*)buffer,sizeof(buffer));
      n=recv(socket,buffer,sizeof(buffer),0); //rcv the buffer from server
      if(!(n<=0))
      send(sock_client,buffer,n,0);  //send buffer to the browser
     }while(n>0);
  }
}

int AddrBlock(char *addr, FILE* file){
  /*printf("\n la fonction est appelée\n");
  printf("\n");
  printf("\n l'adresse est : %s\n",addr);
  printf("\n");*/
	//fonction qui assure le blocage par adresse
	char line[200]="";
	while(fgets(line,200,file)!=NULL){ // parcours de toutes les lignes du fichier
		if(line[0]!='#' && line[0]!='|' && line[0]!='@'){ //Si c'est une règle qui concerne les url
      
			if(strstr(addr,line)!=NULL){ //Si l'adresse contiens l'élément présent sur la ligne
        printf("\nligne qu'on test : %s",line);
        printf("\n on est sensé avoir blocage");
				return(1);
			}
		}
	}
	return(0);

}

int HostBlock(char* host, FILE* file){
  printf("l'hote est :%s\n",host);
  char line[200]="";
  char *buff=NULL;
  while(fgets(line,200,file)!=NULL){// parcours de toutes les lignes du fichier
    if (line[0]=='|' && line[1]=='|'){
      buff=strtok((char*)line, "||");
      buff=strtok(NULL,"\n");
      printf("1");
      printf("buffer : %s\n",buff);
      printf("2");

      if (strstr(host,buff)!=NULL){
        return(1);
      }
    }
  }
  return(0); 
    
}

int main(int argc, char *argv[]){
  FILE* file = NULL;

  file = fopen("easyList.txt", "r");
  if(file==NULL){
  	ThrowError("Error : Can't open rules file");
  }
  struct sockaddr_in addr_client;
  struct addrinfo *res;
  struct addrinfo *p;
  struct sockaddr_in addr_server; 
  int sockProxy=0;
  int sockClient=0; // create our 2 first socket correspond to browser and serveur side of our proxy
  int state=0; 
  int port;
  char ipstring[INET_ADDRSTRLEN]; //ipstring = address IP
  char* pute=NULL;

  char ipver; 
  pid_t pid; // n°pid for the fork
  void *addr=NULL;
  
 
   // parameter of the port to indicates in the launch

  memset((char *) &addr_server, 0, sizeof(addr_server));
  memset((char *) &addr_client, 0, sizeof(addr_client));

  port = atoi(argv[1]); //convert the port argument into int
  checkArg(argc); // check the nb of arguments at exec
  sockProxy=init(sockProxy,addr_server, port); // initialize proxy Socket
  
  int clieServLen=sizeof(addr_client);
  
  while(1){

    sockClient = accept(sockProxy,(struct sockaddr *) &addr_client, (socklen_t *)&clieServLen);  // function accept
    if( sockClient <0){
      ThrowError("Error : Can't accept client socket on proxy socket\n");
    }
    
    pid=fork(); // fork use for the multiclient
    if (pid==0){
      struct sockaddr_in *IP;
      struct addrinfo proxy_addr;

      int n=0;
      int newsockfd=0; //create our two other socket which is server and client sight of the proxy sockets
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
      char url[500]; //char that will contains the total url of the request
      char *path=NULL;  //path of each url of a request
      



      memset(&IP, 0, sizeof(IP));
      memset((char *) send_buffer, 0, sizeof(send_buffer));
      

      if ( (n= recv(sockClient,send_buffer,sizeof(send_buffer),0 ))<0 )  { //function recv that receives the buffer of the client by our proxy
        ThrowError("Error : Can't receive buffer from client\n");
      }

    	sscanf(send_buffer,"%s %s %s",url_parser,web_url,protocol_name); //Parsing the request GET 
    	strcpy(url,web_url); 
    
    	if(((strncmp(protocol_name,"HTTP/1.1",8)==0)||(strncmp(protocol_name,"HTTP/1.0",8)==0))&&((strncmp(web_url,"http://",7)==0))) //Treats the request GET and POST in IP or ipv6
    	{
		      port=80;
		      strcpy(url_parser,web_url);
		      tmp=strtok(web_url,"//");
		      
		      tmp=strtok(NULL,"/");
		      strcpy(web_url,tmp);

		      strcat(url_parser,"::");
		      tmp=strtok(NULL,"::");

		      if(tmp!=NULL) { // We need to add a '/' before our path because the parse of the request delete it
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
        inet_ntop(p->ai_family, addr, ipstring, INET_ADDRSTRLEN); // transform the ip adress into char
        
      
      
      p = p->ai_next;
    	}

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <0) { //create our socket for IP
          ThrowError("Error : Can't create proxy socket for IP\n");
        }
        if((newsockfd=connect(sockfd,(struct sockaddr*)IP,sizeof(struct sockaddr)))<0){ //  connect between client sight and server socket
          ThrowError("Error : Can't connect client socket to server socket\n");
        }

        if((tmp!=NULL) || (port ==80)){  //If we are in HTTP

          if(path==NULL){  // If the path is null the buffer still anyway want to have a '/'
            path="/";
          }
        	//printf("path : %s\n",path);
        	//printf("url : %s\n",url);

          tmp_buffer = strChange(send_buffer,url, path); // replace the url send by the path

          client_buffer = strChange(tmp_buffer, "keep-alive", "close"); // replace keep alive by close
          int buff_len = strlen(client_buffer);
          pute = (char*)malloc(buff_len * sizeof(char));
          strcpy(pute,client_buffer);

          host=strtok(pute,"User-Agent: ");
          host= strtok(NULL,"\n");
          host= strtok(NULL,"\n");
          host= strtok(host," :");
          host = strtok(NULL," :");
          //strcat(client_buffer, "Connection: close");  // prepare our buffer to be send
          //printf("%s\n",client_buffer );
          //printf("url : %s",url);
          if(AddrBlock(url,file)==0 && HostBlock(host,file)==0){
            sendToBrowser(sockfd,client_buffer,sockClient,n);
          }else{
            printf("\n\n----------------------------------------------------------------------------------\n\n");
          }

          
        }
	    }
	    close(sockClient);
	    close(sockfd);  //close our sockets
	    close(sockProxy);
	    _exit(0);
	} else {

    close(sockClient);

  }
}

return 0;
}
