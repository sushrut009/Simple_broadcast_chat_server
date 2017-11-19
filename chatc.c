/*
 * Program Name: Client.c
 * Authors: Sushrut Kaul, Ishan Tyagi
 * Department of Electrical and Computer Engineering
 * Texas A&M University,College Station
 * 
*/

/*
 * Standard header file declarations 
 * 
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<stdarg.h>

#define DATA_SIZE_LIMIT 100 // set limit to the Data size to be handled
#define MESSAGE_LEN_LIMIT 512 //512 Bytes is the maximum message length allowed



//structures to be used 
	//struct {
	//char username[16];
	//char message[512];
	//char reason[32];
	//unsigned int client_count :16;
	//}attribute_payload;

	struct SBCP_attribute{
	uint16_t type;
	uint16_t length;
	char *attr_payload;
	};
	
	struct SBCP_message{
	uint8_t vrsn; // we have assigned a bit field to version.vrsn=3 in our case
	uint8_t type; //assign type=2 for JOIN,3 for forward and 4 for send
	uint16_t length;
	struct SBCP_attribute *attribute;
	};

	void packi16(unsigned char *buf, unsigned int i)
	{
    	*buf++ = i>>8; *buf++ = i;
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

unsigned int unpacki16(char *buf){
return (buf[0]<<8) | buf[1];
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


//write methods to initialize your structures 
//following is a method to initialize my SBCP_attribute structure

	void init_sbcp_attribute(struct SBCP_attribute *struct_ptr, int type_attr ,char *ptr_to_buffer,int len)
	{
		(struct_ptr->type)= type_attr;
		(struct_ptr->attr_payload)= ptr_to_buffer; 
		struct_ptr->length= len;
	}

	void init_sbcp_message(struct SBCP_message *struct_msg_ptr,int version,int type_msg,struct SBCP_attribute *attr123,int len)
	{
		(struct_msg_ptr->vrsn)=version;
		(struct_msg_ptr->type)= type_msg;
		(struct_msg_ptr->attribute)=attr123;
		(struct_msg_ptr->length)=len;
	}

/* Step 1 : Create static variables to maintain the state across calls */

static char my_buffer[DATA_SIZE_LIMIT];
static char *buffer_pointer;
static int  buffer_count; 


void err_sys(const char* x)
{
	perror(x);
	exit(1);
}


/*The following function is used to write n bytes.
  This function offers an advantage over the standard
  write method call. It sends 'n' bytes while the 
  standard write can sometimes send less and that 
  condition is not an error.
  */ 

int writen(int sock_d,char *str,int n)
{
	ssize_t bytes_sent;
	size_t bytes_remaining=n;
	while(bytes_remaining!=0) {
		bytes_sent = write(sock_d,str,bytes_remaining);
		if(bytes_sent<=0)  {
			if(errno== EINTR && bytes_sent <0)
				bytes_sent=0;
			else
				return -1;
		}
		str=str+bytes_sent;
		bytes_remaining=bytes_remaining-bytes_sent;
	}
	printf("wrote %d bytes \n",n);
	return n;
}


/*
   my_read function takes the socket descriptor
   and pointer to a character where the read char 
   will be stored. This function is used by our
   readline function
   */

int my_read(int sock_d,char *ptr)
{

	//Initially , the buffer_count value is zero.
	if(buffer_count<=0) {
up:		//label

		buffer_count=	read(sock_d,my_buffer,sizeof(my_buffer));
		/*buffer_count is updated in the previous statement.
		  It can be positive,negative or zero. Each value corresponds
		  to a specific meaning checked below */


		//CASE A: BUFFER_COUNT is less than zero.
		//Caution: Indicates error condition

		if(buffer_count < 0) {
			//EINTR requires us to call read again .
			//So, we jump back to label 'up'
			if(errno == EINTR) {
				goto up;
			}
			else {
				return -1;
			}
		} 


		//CASE B: Buffer_count==0 
		//Indicates an end of file condition

		else if (buffer_count==0)
			return 0;


		//CASE C:When Buffer_count>0,we set set the buffer pointer 
		//equal to a pointer to the string from which we are reading
		buffer_pointer= my_buffer;
	}

	buffer_count=buffer_count-1; //one element read from the buffer
	*ptr=*buffer_pointer;        //assign pointer to the character pointer
	buffer_pointer=buffer_pointer+1; // Increment pointer
	return 1;
}


int readline(int sock_d,char *ptr,int max)
{
	//Readline will call our my_read function for better control
	int bytes_received;
	int i=1;
	char c,*ptr1;
	ptr1=ptr;
	while(i<max)
	{	
		bytes_received =my_read(sock_d,&c); //read a character from buffer into c
		
		if(bytes_received== 1)
		{
			*ptr1=c; 
			ptr1=ptr1+1;
			if(c == '\n') //null termination
				break;
		}
		else if (bytes_received == 0) {
			*ptr1=0; //this is the End of file situation
			return (i-1);
		}	
		else {
			return -1;
		}
	
             
	        /*	
		switch(bytes_received) {
			//case 1:If bytes_received is 1, we got one char from buffer
			case 1:
				*ptr1=c; 
				ptr1=ptr1+1;
				if(c=='\n') // New line character read
					break;     //line read complete
				break;
			case 0: 
				//End of file situation
				*ptr1=0; 
				return (i-1);
			default: 
				return -1;
		}
		*/

		i=i+1; //loop counter
	}
	*ptr1=0; // null termination
	return i; // return the number of bytes read
}



void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family==AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}








int main(int argc,char *argv[])
{
	int sockfd,numbytes,i;
	char buf[DATA_SIZE_LIMIT];
	struct addrinfo hints , *server_info, *p;
	char input_stdin[100];
	char username[16];
	int rv,len,size;
	int bytes_written;
	char s[INET6_ADDRSTRLEN];
	char buffer[1024];
	fd_set read_set,read_set1;
	struct timeval tv;
	char message[MESSAGE_LEN_LIMIT];
	struct SBCP_message sbcp_msg;
	struct SBCP_attribute sbcp_attr1;
	char data[530];
	char data1[20];
	struct SBCP_message sbcp_msg1;
	struct SBCP_attribute sbcp_attr11;
	struct SBCP_attribute sbcp_attr22;
	struct SBCP_attribute sbcp_attr2;
	uint16_t count;
	int cnt=0;
	//zero out the master set and the read set 
	FD_ZERO(&read_set);


	if(argc!=4) {
		fprintf(stderr,"usage :client username hostname port-number\n");
		exit(1);
	}


	


	memset(&hints ,0,sizeof hints);
	hints.ai_family =AF_UNSPEC;     // can be IpV4/IpV6
	hints.ai_socktype =SOCK_STREAM; // Stream Socket


	if((rv=getaddrinfo(argv[2],argv[3],&hints,&server_info)) !=0) {
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
		return 1;
	}

	for(p=server_info; p!= NULL; p=p->ai_next) { // Find and use the first working configuration
		//ai_next is the pointer to next node in the linked list
		if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1) {
			err_sys("client:socket");
			continue;
		}

		if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1) {
			close(sockfd);
			err_sys("client : connect");
			continue;
		}
		break;
	}


	if(p==NULL) {
		fprintf(stderr,"client:failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof s);
	printf("client:connecting to %s and my username is  %s\n",s,argv[1]);
	freeaddrinfo(server_info);//server_info is not longer required


//copy the username from the argv[1] into username
strcpy(username,argv[1]);
len=strlen(username);
//start by sending the join request 
//data for JOIN :  type =2 ,version =3,payload=username entered by the client on the console 
//void init_sbcp_attribute( int type_attr ,int *ptr_to_buffer,int len) --Method to initialize the sbcp_attribute

init_sbcp_attribute(&sbcp_attr1,2,username,20);

//void init_sbcp_message(int version,int type_msg,struct SBCP_attribute *attr123)
init_sbcp_message(&sbcp_msg,'3','2',&sbcp_attr1,24);


size = pack(buf,"cchhhshhs", sbcp_msg.vrsn, sbcp_msg.type, sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,username,3,530," ");

	// send data of username
    if (send(sockfd, buf, size, 0) == -1){
						perror("send");
						exit(1);
}

//Next is the send operation 
init_sbcp_attribute(&sbcp_attr1,4,message,520);
init_sbcp_message(&sbcp_msg,'3','4',&sbcp_attr1,524);
init_sbcp_attribute(&sbcp_attr2,2,username,16);
	
//Add the standard input and the socket to the read set
	
	//FD_SET(0, &read_set); // add the keyboard input to the read_fds set
	//FD_SET(sockfd, &read_set);
	int ret_val;
	while(1) // Infinite loop
	{
		//read_set=master_set;
	    FD_ZERO(&read_set);
            FD_SET(0,&read_set);
            FD_SET(sockfd,&read_set);
	    tv.tv_sec=10;
		if((ret_val=select(sockfd+1,&read_set,NULL,NULL,&tv))==-1) {
			perror("select");
			exit(4);
		}

		if(ret_val==0) {
			//nothing is sec for 10 s elapsed time 
			size=pack(buf,"cchhhshhs",'3','9',300,2,200,username,4,300,message);
			if(send(sockfd,buf,size,0) ==-1) {
					   perror("send");
				         		}
		}
                   

	
		if(FD_ISSET(0,&read_set)) {	
				if((fgets(message,512,stdin)==NULL)|| feof(stdin))  {
				/* fgets() returns a NULL when we type the END-OF-FILE(CNTRL-D)
				   However, this does not take care of the case when we input
				   some text followed by the END-OF-FILE. For that, we use the 
				   feof() function. feof will return true if it encounters an 
				   end of file.*/

		
				printf("\nEnd of file detected\n");
				printf("\nclosing the socket on client side\n");
				printf("Client disconnected\n");
				close(sockfd);
				exit(1);
				}

				else {
				strcpy(username,argv[1]);
				size=pack(buf,"cchhhshhs", sbcp_msg.vrsn, sbcp_msg.type,sbcp_msg.length, sbcp_attr1.type,sbcp_attr1.length,username,sbcp_attr2.type,sbcp_attr2.length,message);
			 	if(send(sockfd,buf,size,0) ==-1) {
					   perror("send");
				         		}

				}
			} 


		    if (FD_ISSET(sockfd,&read_set)){//there is data to read from server
					// receive chat message to server
						
					if ((numbytes = recv(sockfd, buf, DATA_SIZE_LIMIT-1, 0)) <= 0) {
						perror("recv");
						exit(1);
					}

			unpack(buf,"cchhhhshhs",&sbcp_msg1.vrsn,&sbcp_msg1.type,&sbcp_msg1.length,&sbcp_attr11.type,&sbcp_attr11.length,&count,data,&sbcp_attr22.type,&sbcp_attr22.length,data1);
					
					

					if(sbcp_msg1.type=='5') {
						printf("%s\n",data1);
					}
						
					if(cnt==2) cnt=0;
			  		if(sbcp_msg1.type=='7' && cnt!=1) {
					cnt++;
					printf("%s:",data1);
					printf("%s\n",data);
					if(count==0)
						count=1;
					printf("The count is :%d\n",count);
					//printf("%d\n",cnt);
					
					
			   		}

					if(sbcp_msg1.type=='8') {
					  printf("ONLINE:%s",data1);
					  printf("%s\n",data);
					}
				
					if(sbcp_msg1.type=='6') {
					  printf("OFFLINE:%s",data1);
					  printf("%s\n",data);
					}
					
					if(sbcp_msg1.type=='9') {
					printf("IDLE MESSAGE:");
					printf("%s has been idle for more than 10 seconds\n",data);
					}
 
					else if(sbcp_msg1.type=='3') {
					 printf("Received data from %s is : %s \n",data,data1);
					}
		    }
	
		}
	return 0;
	}
					
				


	

