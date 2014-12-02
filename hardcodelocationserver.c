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


	int  sock, rtsock, i,j, sd, count, params[2], fd,index_sp,mimo_flag=0,mimo_flag1=0,mimo_flag2=0, average[10];
	int  addrLen,k,sum=0,qsum=0,sum1=0,qsum1=0,sum2=0,qsum2=0, number=0, add=0;
        int   a_macadd1, a_macadd2, a_macadd3, a_macadd4, a_macadd5,a_macadd6;
	
        FILE* f, ff;
        
        double alfa;
        
        float a,b,c,Xmn,Ymn,Ymn1,Ymn2,X1,X2,X3,X4,X5,Y1,Y2,Y3,Y4,Y5,Distance,dp1,dp2,tambah, averaged=0.0;
        float Distance1,mobile_A,mobile_B,Distance15,Distance2,Distance23,Distance24,Distance25;
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
              recv(sock,&index_sp, sizeof(index_sp), 0);
               //printf("I receive ur index \n");
              for(i=0; i< index_sp;i++)   {
              recv(sock, &p[i], sizeof(p), 0);
              if(strcmp(p[i].essid,"mimos")==0)
              {a=p[i].dis;tambah=p[i].sl;mimo_flag++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,tambah,a);}
              else if(strcmp(p[i].essid,"mimos1")==0) 
               {b=p[i].dis;mimo_flag1++;fprintf(f,"%s ESSID: %s \nSignal level: %d \n Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,b);}
              else if(strcmp(p[i].essid,"mimos2")==0) 
               {c=p[i].dis;mimo_flag2++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,c);}
              printf("%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,p[i].dis);
            
                    
             // calculate mobile node location
            if(mimo_flag > 0 && mimo_flag1>0 && mimo_flag2 >0 ){
     
             alfa=acos((a*a+6.4-b*b)/(5.06*a));
             if( isnan(alfa)==0){
             Xmn=a*cos(alfa);Ymn1=a*sin(alfa);Ymn2=-Ymn1;
             dp1=sqrt(Xmn*Xmn + (2.7-Ymn1)*(2.7-Ymn1));
             dp2=sqrt(Xmn*Xmn + (2.7-Ymn2)*(2.7-Ymn2));
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

 	Distance1=sqrt(((Xmn-3.5)*(Xmn-3.5))+((Ymn-1.2)*(Ymn-1.2)));
        Distance2=sqrt(((Xmn-2.8)*(Xmn-2.8))+((Ymn-8.7)*(Ymn-8.7)));

printf("Distance for mobile node with mobile A is %f \n",Distance1);		
fprintf(f,"Distance for mobile node with mobile A is %f \n",Distance1);
printf("Distance for mobile node with mobile B is %f \n",Distance2);		
fprintf(f,"Distance for mobile node with mobile B is %f \n",Distance2);
	
//nearest distance

{
if (Distance1>Distance2){
        nearestmobile = mobile_B;
        printf (" The nearest mobile is Mobile B with distance %f\n ", Distance2);
        fprintf (f," The nearest mobile is Mobile B with distance %f\n ", Distance2);
         }
          else {
          nearestmobile = mobile_A;
          printf (" The nearest mobile is Mobile A with distance %f\n ", Distance1);
          fprintf (f," The nearest mobile is Mobile A with distance %f\n ", Distance1);
          }
}



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
