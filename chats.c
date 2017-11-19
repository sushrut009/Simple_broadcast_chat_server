/*
Program Name: echos.c
Author Name : Sushrut Kaul , Ishan Tyagi
Department of Electrical and Computer Engineering , Texas A&M University
*/


/*
Include the header files 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>
#include<ctype.h>
#include<stdarg.h>

struct SBCP_attribute{
	uint16_t type;
	uint16_t length;
	uint16_t client_count;
	char reason[32];
	char *attr_payload;
};
	
struct SBCP_message{
	uint8_t vrsn; // we have assigned a bit field to version.vrsn=3 in our case
	uint8_t type; //assign type=2 for JOIN,3 for forward and 4 for send
	uint16_t length;
	struct SBCP_attribute *attribute;
};

/*The following function is used to write n bytes.
This function offers an advantage over the standard
write method call. It sends 'n' bytes while the 
standard write can sometimes send less and that 
condition is not an error.
*/

void packi16(unsigned char *buf, unsigned int i){
    	*buf++ = i>>8; *buf++ = i;
}

unsigned int unpacki16(char *buf){
return (buf[0]<<8) | buf[1];
}

int32_t pack(char *buf, char *format, ...)
{
    va_list ap;
    int16_t h;
    int8_t c;
    char *s;
    int32_t size = 0, len;
    va_start(ap, format);
    for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				size += 2;
				h = (int16_t)va_arg(ap, int); // promoted
				packi16(buf, h);
				buf += 2;
				break;
			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int); // promoted
				*buf++ = (c>>0)&0xff;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = strlen(s);
				size += len + 2;
				packi16(buf, len);
				buf += 2;
				memcpy(buf, s, len);
				buf += len;
				break;
		}
	}
	va_end(ap);
	return size;
}


void unpack(char *buf, char *format, ...)
{
	va_list ap;
	int16_t *h;
	int8_t *c;
	char *s;
	int32_t len, count, maxstrlen=0;
	va_start(ap, format);
	for(; *format != '\0'; format++) {
		switch(*format) {
            case 'h': // 16-bit
				h = va_arg(ap, int16_t*);
				*h = unpacki16(buf);
				buf += 2;
				break;
			case 'c': // 8-bit
				c = va_arg(ap, int8_t*);
				*c = *buf++;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = unpacki16(buf);
				buf += 2;
				if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
				else count = len;
				memcpy(s, buf, count);
				s[count] = '\0';
				buf += len;
				break;
			default:
				if (isdigit(*format)) { // track max str len
					maxstrlen = maxstrlen * 10 + (*format-'0');
				}
		}
		if (!isdigit(*format)) maxstrlen = 0;
	}
	va_end(ap);
}



void init_sbcp_attribute(struct SBCP_attribute *struct_ptr, int type_attr ,char *ptr_to_buffer,int len){
		(struct_ptr->type)= type_attr;
		(struct_ptr->attr_payload)= ptr_to_buffer; 
		struct_ptr->length= 4+len;
}

void init_sbcp_message(struct SBCP_message *struct_msg_ptr,int version,int type_msg,struct SBCP_attribute *attr123){
		(struct_msg_ptr->vrsn)=version;
		(struct_msg_ptr->type)= type_msg;
		(struct_msg_ptr->attribute)=attr123;
		(struct_msg_ptr->length)=(attr123->length) +4;
}



void sigchld_handler(int s){
	 int saved_errno= errno;
	 while(waitpid(-1,NULL,WNOHANG )>0);
	 errno =saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET) {
	 return &(((struct sockaddr_in*)sa)->sin_addr);
	 return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}
}

void err_sys(const char* x){ 
	    perror(x); 
	     exit(1); 
}


#define MAXDATASIZE 100
char active_users_string[160];
char existing_usernames[10][16];
char list[10][16];
int BACKLOG;  // Backlog is the maximum number of the clients that can connect
int main(int argc,char *argv[]) {


if(argc!=4) {
	printf("Wrong format :::: Enter in the following order : ./server localhost PORT-NO Max_client\n");
	}

   BACKLOG=atoi(argv[3]);
	printf("%d\n",BACKLOG);
//Variables definitions begin
fd_set master_set;
fd_set read_set;
int fdmax,i,j;  //maximum file descriptor number
int sockfd,new_fd,numbytes,bytes_sent;
 char buf[MAXDATASIZE];
struct addrinfo hints, *servinfo , *p;
 struct sockaddr_storage their_addr;//connector's address
 socklen_t sin_size;
 struct sigaction sa;
 int yes=1;
int val=0;
uint16_t count=0;
 char s[INET6_ADDRSTRLEN];
 int rv,size,k=0;
int t=0;
char username_buffer[16];
char recv_buffer[25];
char reject_message[512];
char online_message[100]="Our newest member is:";
char offline_message[100]="The following user has left us unceremoniously:";
char my_buffer[16];
char message_buffer[530];
  struct SBCP_message sbcp_msg;
  struct SBCP_attribute sbcp_attr1;
  struct SBCP_attribute sbcp_attr2;
  struct SBCP_message sbcp_msg1;
  struct SBCP_attribute sbcp_attr11;
// Variables definitions end
char user[16];
int g,g1,x;

int comp_res;
 


memset(&hints,0,sizeof hints);
hints.ai_family =AF_UNSPEC;// We are not specifying the ADDRESS TYPE	
hints.ai_socktype =SOCK_STREAM;// Using the Stream Socket
hints.ai_flags =AI_PASSIVE ; // AI_PASSIVE REFERS TO "MY-IP ADDRESS"

FD_ZERO(&master_set);
FD_ZERO(&read_set);





if((rv =getaddrinfo(NULL,argv[2],&hints,&servinfo )) !=0) {
	fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));						
	return 1;								
}


/*ai_next refers to the pointer to the next node
in the linked-list that we have obtained
*/


for(p=servinfo ;p!=NULL ; p->ai_next ) {
	if((sockfd = socket(p->ai_family , p->ai_socktype, p->ai_protocol))==-1){				
		err_sys("server :socket");		
		continue;
	}
												
if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1) {
		err_sys("setsockopt");										
		exit(1);									 
}												
if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1) {
		close(sockfd);											
		err_sys("server:bind");										
		continue;									 
}
 break    ; // break when you get a usable configuration
}
	freeaddrinfo(servinfo) ;//servinfo is no longer required								
if(p==NULL) {
	fprintf(stderr,"server:failed to bind\n") ;							
exit(1);
 }
if(listen(sockfd,BACKLOG) ==-1) {
		err_sys("listen");
		exit(1);																				
}
sa.sa_handler =sigchld_handler;	
sigemptyset(&sa.sa_mask);
sa.sa_flags= SA_RESTART ;									
if(sigaction(SIGCHLD ,&sa,NULL)==-1) {								
err_sys("sigaction");
exit(1);
}


 printf("server waiting for connections...\n");

//add listener socket to the master set
FD_SET(sockfd,&master_set);

fdmax = sockfd; //sockfd is the biggest value till now
while(1) {
     read_set=master_set;
     if(select(fdmax+1,&read_set,NULL,NULL,NULL) ==-1) { 
	perror("select");
	exit(4);
      } 


  for(i=0;i<=fdmax;i++) {
	if(FD_ISSET(i,&read_set)) {
		if(i==sockfd) {
	         	sin_size=sizeof their_addr;
		  	new_fd=accept(sockfd,(struct sockaddr *)&their_addr,&sin_size);
	
			if(new_fd==-1) {
		   		 perror("accept");
			} 
			else {
			   	FD_SET(new_fd,&master_set);
		    	   	if(new_fd>fdmax) {
			   		fdmax=new_fd;
		     	    	}
				printf("selectserver :new connection from %s on socket %d\n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s,INET6_ADDRSTRLEN),new_fd);
			 }
	         } 
 
		else {
			if((numbytes =recv(i,buf,sizeof buf,0)) <=0) {
				int f,k1,z1;int flag=1; int h;
				//printf(" The number of bytes are %d\n",numbytes);
		  	 	if(numbytes ==0) {
					for(h=0;h<i;h++) {
						if(!(strcmp(list[h],username_buffer))) {
							flag=0;
						}
					}
						if(flag!=0) {	
						for(g1=0;g1<=fdmax;g1++) {
			   	        				if(FD_ISSET(g1,&master_set)) {
										if(g1!=sockfd && g1!=i) {
							    size= pack(buf,"cchhhhshhs",'3','6',sbcp_msg.length,2,sbcp_attr1.length,count,list[i],4,sbcp_attr2.length,offline_message);
									
									if(send(g1,buf,size,0) ==-1) {
					    					perror("send");
				         				} 
								      } 
									} 
									}
					strcpy(user,list[i]);
					count--;
					for(f=0;f<10;f++) {
						if(!strcmp(existing_usernames[f],user)) {
		
							for(k1=f;k1<9;k1++) {
								strcpy(existing_usernames[k1],existing_usernames[k1+1]);
							}
						}	
					}
					//for(z1=0;z1<=9;z1++) {
						//	printf("%s\n",existing_usernames[z1]);
						//}
					memset(list[i],0,sizeof(list[i]));
					//printf("selectserver:socket %d hung up\n",i);
				}
						
		   	  	} 
		    	 	else {
					perror("recv");
		     	  	}
				close(i);
				FD_CLR(i,&master_set);
			}  
			else {
				//printf("I am here\n");
		unpack(buf,"cchhhshhs",&sbcp_msg.vrsn,&sbcp_msg.type,&sbcp_msg.length,&sbcp_attr1.type,&sbcp_attr1.length,username_buffer,&sbcp_attr2.type,&sbcp_attr2.length,message_buffer);
				//printf("%c\n",sbcp_msg.type);
				//printf("%s\n",recv_buffer);
				//printf("This is my username_buffer :%s\n",username_buffer);
			
				if(sbcp_msg.type=='2')  { // this means that you have got a send message on your hand
							int u,current_pos;
							   comp_res=0;
							int t9=0;
						//printf("I am here\n");int z;
						
						//printf("count is=%d\n",count);
						//strcat(welcome_message,recv_buffer);
						//printf("%s",welcome_message);

						if(count==0) {
							strcpy(existing_usernames[0],username_buffer);
							strcpy(list[i],username_buffer);
							current_pos=1;
							char welcome_message[512]="From now on, you will be recognized as";
					//		printf("count second :%d",count);
							 pack(buf,"cchhhhshhs",'3','7',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,welcome_message);
					//		printf("count again :%d\n",count);							
							t9=1;
							count++;
					//		printf("Count bhosdi %d \n",count);
						}
					
                                                else {
							
							for(u=0;u<10;u++) {
							     if(!strcmp(existing_usernames[u],username_buffer)) {
								comp_res=10;
							      }
							 }
						  }

													
					        if(t9!=1) {
						if(comp_res!=10 && count <BACKLOG) {
								strcpy(list[i],username_buffer);
								strcpy(existing_usernames[current_pos],username_buffer);

								//printf("%s\n",username_buffer);
							 	current_pos++;
								count++;
								for(x=0;x<=9;x++) {
									strcat(active_users_string,existing_usernames[x]) ;
									strcat(active_users_string," ");
								
								}
								//printf("%s\n",active_users_string);
								for(g=0;g<=fdmax;g++) {
			   	        				if(FD_ISSET(g,&master_set)) {
										if(g!=sockfd && g!=i) {
							    size= pack(buf,"cchhhhshhs",'3','8',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,online_message);
									if(send(g,buf,size,0) ==-1) {
					    					perror("send");
				         				} 
								      } 
									} 
									}
											
									
							 	char welcome_message[512]="Online users in the system are:\n";
							 	pack(buf,"cchhhhshhs",'3','7',sbcp_msg.length,2,sbcp_attr1.length,count,active_users_string,4,sbcp_attr2.length,welcome_message);
								memset(active_users_string,0,sizeof(active_users_string));
								
							 	
						}
						else{
								if(count>=BACKLOG) {
									strcpy(reject_message,"User limit reached....");
									
								}
								if(comp_res==10){
									strcpy(reject_message,"User with the same name already exists....");
								 }		


								 pack(buf,"cchhhhshhs",'3','5',sbcp_msg.length,2,sbcp_attr1.length,count,username_buffer,4,sbcp_attr2.length,reject_message);
								
				
						   }
						}
						//printf("Before removing:\n");
						//for(z=0;z<=9;z++) {
						//	printf("%s\n",existing_usernames[z]);
						//}
						//printf("%d",count);
						if(send(i,buf,100,0)==-1) {
							 		perror("send");
							  	}

				
					
			   
				}
				if(sbcp_msg.type=='9') {
					for(j=0;j<=fdmax;j++) {
			   	        	if(FD_ISSET(j,&master_set)) {
					        	if(j!=sockfd && j!=i) {
								init_sbcp_attribute(&sbcp_attr1,2,username_buffer,16);
								init_sbcp_message(&sbcp_msg,'3','9',&sbcp_attr1);
								init_sbcp_attribute(&sbcp_attr2,4,message_buffer,530);
			size=pack(buf,"cchhhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,count,username_buffer,sbcp_attr2.type,sbcp_attr2.length,message_buffer);
							
							
						        	if(send(j,buf,size,0) ==-1) {
					    				perror("send");
				         			}
						 	}
			                 	}
					}
				}
					

				
				 if(sbcp_msg.type=='4') {
					//printf("I am here at message 3\n");
			        	for(j=0;j<=fdmax;j++) {
			   	        	if(FD_ISSET(j,&master_set)) {
					        	if(j!=sockfd && j!=i) {
								init_sbcp_attribute(&sbcp_attr1,2,username_buffer,16);
								init_sbcp_message(&sbcp_msg,'3','3',&sbcp_attr1);
								init_sbcp_attribute(&sbcp_attr2,4,message_buffer,530);
			size=pack(buf,"cchhhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,count,username_buffer,sbcp_attr2.type,sbcp_attr2.length,message_buffer);
							
							
						        	if(send(j,buf,size,0) ==-1) {
					    				perror("send");
				         			}
						 	}
			                 	}
					}
		        	}
	 	    	}//else against i==sockfd
		  }
	       }
	  }
	}
	return 0;
	}
