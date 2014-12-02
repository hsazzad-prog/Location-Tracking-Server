// to compile this code gcc -o Rtrg_gps Rtrg_gps.c libiw.so.29
/*
 * UTM location tracking source code designed By ADEL ALI adelali3@lycos.com		
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/ioctl.h>
#include <math.h>
#include "rtsock.h"

#define	SENDRATE	1000		// 2000 packets per second


//add server
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

//#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold




//until here






struct packet {				// packet contents for sending
	char mac[20];
	char essid[20];        
	int sl;
	float dis;
};
struct packet p[15];
static int g_rtfd = -1;

struct index_s {
        int index_sp;
        char mac[20];
        };
struct index_s ind;

int
DecodeIPAddress(char *s, struct sockaddr_in *sin)
{
	int  port;
	char ip[1024];
	struct hostent *he;

	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	switch(sscanf(s, "%1023[^:]:%i", ip, &port)) {
	case 0:
		if(sscanf(s, ":%i", &port) == 1) {
			sin->sin_port = htons(port);
		}
		break;
	case 1:
		port = 0;
	case 2:
		sin->sin_port = htons(port);
		he = gethostbyname(ip);
		if(he) {
			memcpy(&sin->sin_addr.s_addr, he->h_addr_list[0], 4);
		} else {
			return -1;
		}
		break;
	}
	return 0;
}

char *
EncodeIPAddress(char *s, struct sockaddr_in *sin)
{
	int x;

	x = ntohl(sin->sin_addr.s_addr);
	sprintf(s, "%d.%d.%d.%d:%d", 
		(x>>24)&0xFF, (x>>16)&0xFF, (x>>8)&0xFF, x&0xFF, 
		htons(sin->sin_port)
	);
	return s;
}

int
CreateUDPSocket()
{
	int sock;

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}
	return sock;
}

void
DestroySocket(int sock)
{
	if(sock < 0) {
		return;
	}
	close(sock);
}

int
BindSocket(int sock, char *s)
{
	struct sockaddr_in localAddr;

	if(sock < 0) {
		return sock;
	}
	if(DecodeIPAddress(s, &localAddr) < 0) {
		fprintf(stderr, "cannot resolve %s\n", s);
		return -1;
	}
	if(bind(sock, &localAddr, sizeof(localAddr)) < 0 ) {
		perror("bind failed");
		return -1;
	}
	return sock;
}

int
ConnectSocket(int sock, char *s) 
{
	struct sockaddr_in farAddr;

	if(sock < 0) {
		return sock;
	}
	if(DecodeIPAddress(s, &farAddr) < 0) {
		fprintf(stderr, "cannot resolve %s\n", s);
		return -1;
	}
	if(connect(sock, &farAddr, sizeof(farAddr)) < 0) {
		perror("connect failed");
		return -1;
	}
	return sock;
}

int
CreateRtSocket(int sock, int *params, int paramsLength)
{
	int *ioctlParams, index, i;

	if(sock < 0) {
		return sock;
	}
	if(g_rtfd < 0) {
		if((g_rtfd=open("/dev/rtsock0", O_RDWR)) < 0 ) {
		    perror("rtsock0 open");
		    return -1;
		}
	}
	ioctlParams = calloc(paramsLength+3, sizeof(int));
	ioctlParams[0] = sock;		// fd of sock
	ioctlParams[1] = 0;		// operation
	ioctlParams[2] = 0;		// index of mysock in rt space
	if(params) {
		for(i=0; i<paramsLength; i++) {
			ioctlParams[3+i] = params[i];
		}
	}
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0) {
		perror("ioctl(RTSOCK_IOCINTERFACE, insert)");
		free(ioctlParams);
		close(sock);
		return -1;
	}
	index = ioctlParams[2]; 	// this parameter is return by ioctl
	free(ioctlParams);
	return index;
}

void
DestroyRtSocket(int sock, int index)
{
	long ioctlParams[3];

	if(sock < 0 || index < 0) {
		return;
	}
	ioctlParams[0] = sock;
	ioctlParams[1] = 2;
	ioctlParams[2] = index;
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0) {
		perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
	}
}


//add server


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




//until here


int
RedirectRtSocket(int sock)
{
	long ioctlParams[1];

	if(sock < 0) {
		return sock;
	}
	ioctlParams[0] = sock;
	if(ioctl(g_rtfd, RTSOCK_IOCREDIRECTFD, ioctlParams) < 0) {
		perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
		return -1;
	}
	return sock;
}

int
ResetRtSocket(int sock)
{
	long ioctlParams[1];

	if(sock < 0) {
		return sock;
	}
	ioctlParams[0] = sock;
	if(ioctl(g_rtfd, RTSOCK_IOCRESETFD, ioctlParams) < 0) {
		perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
		return -1;
	}
	return sock;
}

int
UpdateRtSocket(int sock, int index, int *params, int paramsLength)
{
	int *ioctlParams, i;

	if(sock < 0 || index < 0) {
		return -1;
	}
	ioctlParams = calloc(paramsLength+3, sizeof(int));
	ioctlParams[0] = sock;		// fd of sock
	ioctlParams[1] = 1;		// operation
	ioctlParams[2] = index;		// index of mysock in rt space
	if(params) {
		for(i=0; i<paramsLength; i++) {
			ioctlParams[3+i] = params[i];
		}
	}
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0) {
		perror("ioctl(RTSOCK_IOCINTERFACE, update)");
		free(ioctlParams);
		return -1;
	}
	free(ioctlParams);
	return sock;
}

int
main(int argc, char *argv[])
{

//add server

        int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	//struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char l[INET6_ADDRSTRLEN];
	int rv;

//until here





       // int  sock, rtsock, count, params[2], fd,index_sp,mimo_flag=0,mimo_flag1=0,mimo_flag2=0, average[10];
	// int  sock, rtsock, count, params[2], fd,index_sp,mimo_flag=0,mimo_flag1=0,mimo_flag2=0, average[10];
	int  sock, rtsock, i,j, sd, count, params[2], fd,index_s, index_sp,mimo_flag=0,mimo_flag1=0,mimo_flag2=0, average[10];
	int  addrLen,k,sum=0,qsum=0,sum1=0,qsum1=0,sum2=0,qsum2=0, number=0, add=0;
        int   a_macadd1, a_macadd2, a_macadd3, a_macadd4, a_macadd5,a_macadd6;
	
        FILE *f, *f1 = "/root/database.c";
        
        double alfa;
        
        float a,b,c,Xmn,Ymn,Ymn1,Ymn2,X1,X2,X3,X4,X5,Y1,Y2,Y3,Y4,Y5,Distance,dp1,dp2,tambah, averaged=0.0;
        float Distance12,Distance13,Distance14,Distance15,Distance21,Distance23,Distance24,Distance25;
        float Distance31,Distance32,Distance34,Distance35,Distance41,Distance42,Distance43,Distance45;
        float Distance51,Distance52,Distance53,Distance54;

	struct sockaddr_in addr;
        struct hostent *hp;
        struct in_addr *ptr;

        char nearestmobile, mobile1, mobile2, mobile3, mobile4, mobile5;
	char *s, s1[1024];

         if((f=fopen("/root/ssf.c","r+"))==NULL)
		{
		printf("cannot open this file\n");exit(-1);
		}
	if(argc < 2) {
		fprintf(stderr, "usage: Rtrg_gps [ru|rr|tu|tr] [<address>]\n");
		exit(-1);
	}
	s = (argc < 3)? "" : argv[2];
	if(strcmp(argv[1], "ru") == 0) {
		if((sock = BindSocket(CreateUDPSocket(), s)) < 0) {
			exit(-1);
		}
		addrLen = sizeof(addr);
		if(getsockname(sock, &addr, &addrLen) < 0 ) {
		    	close(sock);
		    	return -1;
		}
	
		
	       printf("Rtrg_gps will receive request at this address %s\n", EncodeIPAddress(s1, &addr));
               for(;;){
              recv(sock,&ind.index_sp, sizeof(ind.index_sp), 0);
               //printf("I receive ur index \n");
              for(i=0; i< ind.index_sp;i++)   {
              recv(sock, &p[i], sizeof(p), 0);
              if(strcmp(p[i].essid,"mimos")==0)
               // {a=p[i].dis;mimo_flag++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,a);
                 {a=p[i].dis;tambah=p[i].sl;mimo_flag++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f Got from : %s\n",p[i].mac,p[i].essid,p[i].sl,a,ind.mac);
                      
                   //  a=a+p[i].dis;
                   //  counta++;
            //   add += p[i].sl;
            //   averaged = (float)add / 10;
              
              // n[i]=0;    
              // a+= n[i];  
            //  printf(" entah macam mana %f\n", averaged);
}
              else if(strcmp(p[i].essid,"mimos1")==0) 
               {b=p[i].dis;mimo_flag1++;fprintf(f,"%s ESSID: %s \nSignal level: %d \n Distance: %f Got from : %s\n",p[i].mac,p[i].essid,p[i].sl,b,ind.mac);}
              else if(strcmp(p[i].essid,"mimos2")==0) 
               {c=p[i].dis;mimo_flag2++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f Got from : %s\n",p[i].mac,p[i].essid,p[i].sl,c,ind.mac);}
              printf("%s ESSID: %s Signal level: %d Distance: %f Got from : %s\n",p[i].mac,p[i].essid,p[i].sl,p[i].dis,ind.mac);
       //       printf("%02d %02d %02d %02d %02d %02d\n",a_macadd1, a_macadd2, a_macadd3, a_macadd4, a_macadd5,a_macadd6); 


//add server
                        
		        inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		        printf("server: got connection from %s\n", s);  

//until here



               //add += p[i].sl;
               //averaged = (float)add / i;
              
              // n[i]=0;    
              // a+= n[i];  
              //printf(" %f\n", averaged);

//}
//}

             //  add += tambah;
            //   averaged = (float)add / 3;
             //  printf(" entah macam mana %f\n", averaged);

//printf("enter referend point x,y : " );
//scanf  ("%f,%f", &Xinput, &Yinput);        // key in location of other point
//printf ("You enter %f, %f \n", Xinput, Yinput);
                    
             // calculate mobile node location
            if(mimo_flag > 0 && mimo_flag1>0 && mimo_flag2 >0 ){
     
             alfa=acos((a*a+100-b*b)/(20*a));
             if( isnan(alfa)==0){
             Xmn=a*cos(alfa);Ymn1=a*sin(alfa);Ymn2=-Ymn1;
// 	     Distance=sqrt(((Xmn-Xinput)*(Xmn-Xinput))+((Ymn-Yinput)*(Ymn-Yinput)));
             dp1=sqrt(Xmn*Xmn + (10-Ymn1)*(10-Ymn1));
             dp2=sqrt(Xmn*Xmn + (10-Ymn2)*(10-Ymn2));
  	     if (fabsf(dp1-c) < fabsf(dp2-c))
             Ymn=Ymn1;
             else
             Ymn=-Ymn2;
 	     if ( Xmn < 0 )
 	     Xmn = -Xmn;
             else
             Xmn = Xmn;


	     

printf("alfa is %f, dp1 is %f,dp2 is %f. The location of mobile node is (%f,%f) \n",alfa,dp1,dp2,Xmn,Ymn);
fprintf(f,"The location of mobile node is (%f,%f)\n",Xmn,Ymn);

//write up the location for every mobile node in their on file. please create the file


//else if(strcmp(p[i].essid,"mimos1")==0) 

if (strcmp(ind.mac,"00:13:02:0b:34:de")==0) {
f1=fopen("DATABASE","r+");
Ymn=Y1;
Xmn=X1;
//		{
//	printf("cannot open this file\n");
//  exit(-1);
//		}
 //  else  {
//         if (Ymn = Y1){
        printf (" Mobile node is Mobile Node 1. The Location is %f, %f \n", X1, Y1);
 //       fprintf (f1," Mobile node is Mobile Node 1. The Location is %f, %f \n", X1, Y1);
//}
//}
}
else {
 printf("SALAH!\n");
}
//else if (strcmp(ind.mac,"00:13:02:0b:34:de")==0) {
//        Xmn = X2;
//        Ymn = Y2;
//        printf (" Mobile node is Mobile Node 2. The Location is %f, %f \n", X2, Y2);
//        fprintf (f," Mobile node is Mobile Node 2. The Location is %f, %f \n", X2, Y2);
//}
// else if (strcmp(ind.mac,"00:13:02:0b:34:de")==0) {
//        Xmn = X3;
//        Ymn = Y3;
//        printf (" Mobile node is Mobile Node 3. The Location is %f, %f \n", X3, Y3);
//        fprintf (f," Mobile node is Mobile Node 3. The Location is %f, %f \n", X3, Y3);
//}      

//else if (strcmp(ind.mac,"00:13:02:0b:34:de")==0) {
//        Xmn = X4;
//        Ymn = Y4;
//        printf (" Mobile node is Mobile Node 4. The Location is %f, %f \n", X4, Y4);
//        fprintf (f," Mobile node is Mobile Node 4. The Location is %f, %f \n", X4, Y4);
//}
//else if (strcmp(ind.mac,"00:13:02:0b:34:de")==0) {
//        Xmn = X5;
//        Ymn = Y5;
//        printf (" Mobile node is Mobile Node 5. The Location is %f, %f \n", X5, Y5);
//        fprintf (f," Mobile node is Mobile Node 5. The Location is %f, %f \n", X5, Y5);
//}



		
	if (Ymn=Y1){
 	Distance12=sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2)); //measure the distance
        Distance13=sqrt((X1-X3)*(X1-X3)+(Y1-Y3)*(Y1-Y3)); //measure the distance
        Distance14=sqrt((X1-X4)*(X1-X4)+(Y1-Y4)*(Y1-Y4)); //measure the distance
        Distance15=sqrt((X1-X5)*(X1-X5)+(Y1-Y5)*(Y1-Y5)); //measure the distance
 	}
        else if	(Ymn=Y2){
 	Distance21=sqrt((X2-X1)*(X2-X1)+(Y2-Y1)*(Y2-Y1)); //measure the distance
        Distance23=sqrt((X2-X3)*(X2-X3)+(Y2-Y3)*(Y2-Y3)); //measure the distance
        Distance24=sqrt((X2-X4)*(X2-X4)+(Y2-Y4)*(Y2-Y4)); //measure the distance
        Distance25=sqrt((X2-X5)*(X2-X5)+(Y2-Y5)*(Y2-Y5)); //measure the distance
 	}
        else if	(Ymn=Y3){
 	Distance31=sqrt((X3-X1)*(X3-X1)+(Y3-Y1)*(Y3-Y1)); //measure the distance
        Distance32=sqrt((X3-X2)*(X3-X2)+(Y3-Y2)*(Y3-Y2)); //measure the distance
        Distance34=sqrt((X3-X4)*(X3-X4)+(Y3-Y4)*(Y3-Y4)); //measure the distance
        Distance35=sqrt((X3-X5)*(X3-X5)+(Y3-Y5)*(Y3-Y5)); //measure the distance
 	}
        else if (Ymn=Y4){
 	Distance41=sqrt((X4-X1)*(X4-X1)+(Y4-Y1)*(Y4-Y1)); //measure the distance
        Distance42=sqrt((X4-X2)*(X4-X2)+(Y4-Y2)*(Y4-Y2)); //measure the distance
        Distance43=sqrt((X4-X3)*(X4-X3)+(Y4-Y3)*(Y4-Y3)); //measure the distance
        Distance45=sqrt((X4-X5)*(X4-X5)+(Y4-Y5)*(Y4-Y5)); //measure the distance
 	}
        else if	(Ymn=Y5){
 	Distance51=sqrt((X5-X1)*(X5-X1)+(Y5-Y1)*(Y5-Y1)); //measure the distance
        Distance52=sqrt((X5-X2)*(X5-X2)+(Y5-Y2)*(Y5-Y2)); //measure the distance
        Distance53=sqrt((X5-X3)*(X5-X3)+(Y5-Y3)*(Y5-Y3)); //measure the distance
        Distance54=sqrt((X5-X4)*(X5-X4)+(Y5-Y4)*(Y5-Y4)); //measure the distance
 	}

//nearest distance

{
if (Ymn=Y1){
    if (Distance12 < Distance13 && Distance12 < Distance14 && Distance12 < Distance15) {
        nearestmobile = mobile2;
        printf (" The nearest mobile is Mobile2 with distance %s\n ", Distance12);
        fprintf (f," The nearest mobile is Mobile2 with distance %s\n ", Distance12);
         }
          else if (Distance13 < Distance12 && Distance13 < Distance14 && Distance13 < Distance15) {
          nearestmobile = mobile3;
          printf (" The nearest mobile is Mobile3 with distance %s\n ", Distance13);
          fprintf (f," The nearest mobile is Mobile3 with distance %s\n ", Distance13);
          }
             else if (Distance14 < Distance12 && Distance14 < Distance13 && Distance14 < Distance15) {
             nearestmobile = mobile4;
             printf (" The nearest mobile is Mobile4 with distance %s\n ", Distance14);
             fprintf (f," The nearest mobile is Mobile4 with distance %s\n ", Distance14);
             }
                 else if (Distance15 < Distance12 && Distance15 < Distance13 && Distance15 < Distance14) {
                 nearestmobile = mobile5;
                 printf (" The nearest mobile is Mobile5 with distance %s\n ", Distance15);
                 fprintf (f," The nearest mobile is Mobile5 with distance %s\n ", Distance15);
                 }
       }


  else if (Ymn=Y2){
    if (Distance21 < Distance23 && Distance21 < Distance24 && Distance21 < Distance25) {
        nearestmobile = mobile1;
        printf (" The nearest mobile is Mobile1 with distance %s\n ", Distance21);
        fprintf (f," The nearest mobile is Mobile1 with distance %s\n ", Distance21);
         }
          else if (Distance23 < Distance21 && Distance23 < Distance24 && Distance23 < Distance25) {
          nearestmobile = mobile3;
          printf (" The nearest mobile is Mobile3 with distance %s\n ", Distance23);
          fprintf (f," The nearest mobile is Mobile3 with distance %s\n ", Distance23);
          }
             else if (Distance24 < Distance21 && Distance24 < Distance23 && Distance24 < Distance25) {
             nearestmobile = mobile4;
             printf (" The nearest mobile is Mobile4 with distance %s\n ", Distance24);
             fprintf (f," The nearest mobile is Mobile4 with distance %s\n ", Distance24);
             }
                 else if (Distance25 < Distance21 && Distance25 < Distance23 && Distance25 < Distance24) {
                 nearestmobile = mobile5;
                 printf (" The nearest mobile is Mobile5 with distance %s\n ", Distance25);
                 fprintf (f," The nearest mobile is Mobile5 with distance %s\n ", Distance25);
                 }
       }


   
  else if (Ymn=Y3){
    if (Distance31 < Distance32 && Distance31 < Distance34 && Distance31 < Distance35) {
        nearestmobile = mobile1;
        printf (" The nearest mobile is Mobile1 with distance %s\n ", Distance31);
        fprintf (f," The nearest mobile is Mobile1 with distance %s\n ", Distance31);
         }
          else if (Distance32 < Distance31 && Distance32 < Distance34 && Distance32 < Distance35) {
          nearestmobile = mobile2;
          printf (" The nearest mobile is Mobile2 with distance %s\n ", Distance32);
          fprintf (f," The nearest mobile is Mobile2 with distance %s\n ", Distance32);
          }
             else if (Distance34 < Distance31 && Distance34 < Distance32 && Distance34 < Distance35) {
             nearestmobile = mobile4;
             printf (" The nearest mobile is Mobile4 with distance %s\n ", Distance34);
             fprintf (f," The nearest mobile is Mobile4 with distance %s\n ", Distance34);
             }
                 else if (Distance35 < Distance31 && Distance35 < Distance32 && Distance35 < Distance34) {
                 nearestmobile = mobile5;
                 printf (" The nearest mobile is Mobile5 with distance %s\n ", Distance35);
                 fprintf (f," The nearest mobile is Mobile5 with distance %s\n ", Distance35);
                 }
       }


  else if(Ymn=Y4){
    if (Distance41 < Distance42 && Distance41 < Distance43 && Distance41 < Distance45) {
        nearestmobile = mobile1;
        printf (" The nearest mobile is Mobile1 with distance %s\n ", Distance41);
        fprintf (f," The nearest mobile is Mobile1 with distance %s\n ", Distance41);
         }
          else if (Distance42 < Distance41 && Distance42 < Distance43 && Distance42 < Distance45) {
          nearestmobile = mobile2;
          printf (" The nearest mobile is Mobile2 with distance %s\n ", Distance42);
          fprintf (f," The nearest mobile is Mobile2 with distance %s\n ", Distance42);
          }
             else if (Distance43 < Distance41 && Distance43 < Distance42 && Distance43 < Distance45) {
             nearestmobile = mobile3;
             printf (" The nearest mobile is Mobile3 with distance %s\n ", Distance43);
             fprintf (f," The nearest mobile is Mobile3 with distance %s\n ", Distance43);
             }
                 else if (Distance45 < Distance41 && Distance45 < Distance42 && Distance45 < Distance23) {
                 nearestmobile = mobile5;
                 printf (" The nearest mobile is Mobile5 with distance %s\n ", Distance45);
                 fprintf (f," The nearest mobile is Mobile5 with distance %s\n ", Distance45);
                 }
       }


   
  else if(Ymn=Y5){
    if (Distance51 < Distance52 && Distance51 < Distance53 && Distance51 < Distance54) {
        nearestmobile = mobile1;
        printf (" The nearest mobile is Mobile1 with distance %s\n ", Distance51);
        fprintf (f," The nearest mobile is Mobile1 with distance %s\n ", Distance51);
         }
          else if (Distance52 < Distance51 && Distance52 < Distance53 && Distance52 < Distance54) {
          nearestmobile = mobile2;
          printf (" The nearest mobile is Mobile2 with distance %s\n ", Distance52);
          fprintf (f," The nearest mobile is Mobile2 with distance %s\n ", Distance52);
          }
             else if (Distance53 < Distance51 && Distance53 < Distance52 && Distance53 < Distance54) {
             nearestmobile = mobile3;
             printf (" The nearest mobile is Mobile3 with distance %s\n ", Distance53);
             fprintf (f," The nearest mobile is Mobile3 with distance %s\n ", Distance53);
             }
                 else if (Distance54 < Distance51 && Distance54 < Distance52 && Distance54 < Distance53) {
                 nearestmobile = mobile4;
                 printf (" The nearest mobile is Mobile4 with distance %s\n ", Distance54);
                 fprintf (f," The nearest mobile is Mobile4 with distance %s\n ", Distance54);
                 }
       }

}



 //	{ 	
 //	Ymn=Ymn1;
 //	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn1-Yinput)*(Ymn1-Yinput)); //measure the distance
 //	}
 //       else
 //       {	 
 //	Ymn=Ymn2;
 //	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn2-Yinput)*(Ymn2-Yinput)); //measure the distance
 //	}

//printf("alfa is %f, dp1 is %f,dp2 is %f. The location of mobile node is (%f,%f). The distance between mobile node (%f,%f) and (%f,%f) is %f \n",alfa,dp1,dp2,Xmn,Ymn, Xinput, Yinput, Xmn, Ymn, Distance);
//fprintf(f,"The location of mobile node is , and the nearest distance is \n");
//fclose(f);
//return (i=0); 

}
else
printf("Please wait ...... \n");                    
} //end if

else
printf("Please wait ...... \n");

} //end for loop
} //end infinate for loop	 
}	
}



//comment.. this coding has been add on the distance measurement by using signal strength instead of calculating the distance at the mobile node 20 April 09, 11:32 pm .............. Result = NO ERROR!

// comment putting the distance coding in this Rtrg_gps.c coding to combine the coding into one program.. Result = NO ERROR! 21 April 09, 4:11 am

//new 10hb mei.. midnite... try wat loop forever.. so just started the location server n it will forever receive signal from others

//success to make it forever's loop! and most important it's mean that location server automatically receive signal from other mobile node and it will only stop if the location server is 'exit'.. now.. try to see the file inside root's file and see the mobile node.. what happend if the location already been calculated.. is the mobile node still keep sending the data to location server or not.. if not.. changes it to forever..

//there is a problem occur in writing up the data into file.. try check in the 'tu' coding whether there is something to changes or what often this matterz.. erm.. so now.. it prove that the data will be forever loop.

//nex step is to check up the file at root.. need to build up the database structure.. not yet done the average part so do it fast! target this done by this Friday!

//wednesday.. filter the (negatif y axis)....... success!!!

//thursday.. success updating the file :------->

// "r" open text file for reading
// "w" create text file for writing; discard previous contents if any
// "a" append; open or create text file for writing at end of file
// "r+" open text file for update (ie., reading and writing)
// "w+" create text file for update; discard previous contents of any
// "a+" append; open or create text file for update, writing at end

// update mode permits reading and writing the same file; fflush or a file-positioning function must be called between a read and a write or vice versa. If the mode includes b after the initial letter, as in"rb" or "w+b", that indicates a binary file. Filenames are limited to FILENAME_MAX characters. At most FOPEN_MAX files may be open at once.


//friday.. success 1 quater n forever loop.. but Dr. Sharifah want the file update after 10 reading.. now i can do the update only when the location server is restarted by using "r+".. so need to modified this coding into 10 times reading..

//da tambah coding mac add tp tak tau knp tak keluar mac add tu. padahal patutnya keluar mcm mane y kuar kat server

//GOOD!!! now tis server can ready which connection are from.. this server know the ip add of the mobile node that send the data

//error accur......ip salah!

//da berjaya.. doktor adel ajar.. buat struct baru.. then now.. edit bahagian comparison utk compare.. kalau mac add dia ni.. ape dia.. n kalau ni apa dia.. n so on....
