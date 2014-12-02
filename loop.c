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

int
main(int argc, char *argv[])
{
	int  sock, rtsock, i,j, count, params[2], fd,index_sp,mimo_flag=0,mimo_flag1=0,mimo_flag2=0;
	int  addrLen,k,sum=0,qsum=0,sum1=0,qsum1=0,sum2=0,qsum2=0;
	FILE* f;
        double alfa;
        float a,b,c,Xmn,Ymn,Ymn1,Ymn2,Xinput,Yinput,Distance,dp1,dp2;
	struct sockaddr_in addr;
	char *s, s1[1024];

         if((f=fopen("/root/ssf.c","w+"))==NULL)
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
                {a=p[i].dis;mimo_flag++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,a);}
              else if(strcmp(p[i].essid,"mimos1")==0) 
               {b=p[i].dis;mimo_flag1++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,b);}
              else if(strcmp(p[i].essid,"mimos2")==0) 
               {c=p[i].dis;mimo_flag2++;fprintf(f,"%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,c);}
              printf("%s ESSID: %s Signal level: %d Distance: %f\n",p[i].mac,p[i].essid,p[i].sl,p[i].dis);
	      
                                     } //end for loop

                                
printf("enter referend point x,y : " );
scanf  ("%f,%f", &Xinput, &Yinput); //key in location of other point
printf ("You enter %f, %f \n", Xinput, Yinput);


             // calculate mobile node location
            if(mimo_flag > 0 && mimo_flag1>0 && mimo_flag2 >0 ){
     
             alfa=acos((a*a+100-b*b)/(20*a));
             if( isnan(alfa)==0){
             Xmn=a*cos(alfa);Ymn1=a*sin(alfa);Ymn2=-Ymn1;
 	     Distance=sqrt(((Xmn-Xinput)*(Xmn-Xinput))+((Ymn-Yinput)*(Ymn-Yinput)));
             dp1=sqrt(Xmn*Xmn + (10-Ymn1)*(10-Ymn1));
             dp2=sqrt(Xmn*Xmn + (10-Ymn2)*(10-Ymn2));
             if (fabsf(dp1-c) < fabsf(dp2-c))
 	{ 	
 	Ymn=Ymn1;
 	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn1-Yinput)*(Ymn1-Yinput)); //measure the distance
 	}
        else
        {	 
 	Ymn=Ymn2;
 	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn2-Yinput)*(Ymn2-Yinput)); //measure the distance
  	}

printf("alfa is %f, dp1 is %f,dp2 is %f. The location of mobile node is (%f,%f). The distance between mobile node (%f,%f) and (%f,%f) is %f \n",alfa,dp1,dp2,Xmn,Ymn, Xinput, Yinput, Xmn, Ymn, Distance);
fprintf(f,"The location of mobile node is (%f,%f), and the nearest distance is %d\n",Xmn,Ymn,Distance);
fclose(f);
return (i=0);
  }
else
printf("Please wait ...... \n");                          } //end if
else
printf("Please wait ...... \n");
} //end infinate for loop
	 
}	
}

