//to compile gcc -o PL-latest PL-latest.c /usr/lib/libm.a /lib/libiw.so.25
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

#include <rtsock.h>

struct packet {				// packet contents for sending
	       int sl,qt;
	       char mc1[20];
               };

static int g_rtfd = -1;
#include "iwlib.h"
struct iwreq		wrq;
  char		buffer[(sizeof(struct iw_quality) +
			sizeof(struct sockaddr)) * IW_MAX_SPY];
  char		temp[128];
  struct sockaddr *	hwa;
  struct iw_quality *	qual;
  iwrange	range;
  int		has_range = 0;
  int		n;
static int
print_spy_info(int	skfd,
	       char *	ifname,
	       char *	args[],
	       int	count)
 {
    /* Avoid "Unused parameter" warning */
  args = args; count = count;
  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = IW_MAX_SPY;
  wrq.u.data.flags = 0;
  iw_get_ext(skfd, ifname, SIOCGIWSPY, &wrq);
  n = wrq.u.data.length;
  if(iw_check_mac_addr_type(skfd, ifname) < 0)
    {
     fprintf(stderr, "%-8.8s  Interface doesn't support MAC addresses\n\n", ifname);
     return(-2);
    }
  if(iw_get_range_info(skfd, ifname, &(range)) >= 0)
  has_range = 1;
  hwa = (struct sockaddr *) buffer;
  qual = (struct iw_quality *) (buffer + (sizeof(struct sockaddr) * n));  
  return(0);
 }

int
DecodeIPAddress(char *s, struct sockaddr_in *sin)
       {
	int  port;
	char ip[1024];
	struct hostent *he;
	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	switch(sscanf(s, "%1023[^:]:%i", ip, &port)) 
        {
	 case 0:
		if(sscanf(s, ":%i", &port) == 1) 
                  {
		   sin->sin_port = htons(port);
		  }
		break;
	 case 1:
		port = 0;
	 case 2:
		sin->sin_port = htons(port);
		he = gethostbyname(ip);
		if(he) 
                  {
		   memcpy(&sin->sin_addr.s_addr, he->h_addr_list[0], 4);
		  } 
                else 
                  {
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
		htons(sin->sin_port));
	return s;
       }

int
CreateUDPSocket()
       {
	int sock;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
          {
	   return -1;
	  }
	return sock;
       }

void
DestroySocket(int sock)
       {
	if(sock < 0)
	   {
            return;
	   }
	close(sock);
       }

int
BindSocket(int sock, char *s)
       {
	struct sockaddr_in localAddr;
	if(sock < 0) 
          {
	   return sock;
	  }
	if(DecodeIPAddress(s, &localAddr) < 0) 
          {
	   fprintf(stderr, "cannot resolve %s\n", s);
	   return -1;
	  }
	if(bind(sock, &localAddr, sizeof(localAddr)) < 0 ) 
          {
	   perror("Bind Socket failed");
	   return -1;
	  }
	return sock;
       }

int
ConnectSocket(int sock, char *s) 
       {
	struct sockaddr_in farAddr;
	if(sock < 0) 
          {
	   return sock;
	  }
	if(DecodeIPAddress(s, &farAddr) < 0) 
          {
	   fprintf(stderr, "cannot resolve %s\n", s);
	   return -1;
	  }
	if(connect(sock, &farAddr, sizeof(farAddr)) < 0) 
          {
	   perror("Connection failed");
	   return -1;
	  }
	return sock;
       }

int
CreateRtSocket(int sock, int *params, int paramsLength)
       {
	int *ioctlParams, index, i;
	if(sock < 0) 
          {
	   return sock;
	  }
	if(g_rtfd < 0) 
          {
	   if((g_rtfd=open("/dev/rtsock0", O_RDWR)) < 0 ) 
             {
	      perror("rtsock0 open");
	      return -1;
	     }
	   }
	ioctlParams = calloc(paramsLength+3, sizeof(int));
	ioctlParams[0] = sock;		// fd of sock
	ioctlParams[1] = 0;		// operation
	ioctlParams[2] = 0;		// index of mysock in rt space
	if(params) 
          {
	   for(i=0; i<paramsLength; i++) 
              {
	       ioctlParams[3+i] = params[i];
	      }
	  }
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0)
          {
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
	if(sock < 0 || index < 0) 
          {
	   return;
	  }
	ioctlParams[0] = sock;
	ioctlParams[1] = 2;
	ioctlParams[2] = index;
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0)
          {
	   perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
	  }
       }

int
RedirectRtSocket(int sock)
       {
	long ioctlParams[1];
	if(sock < 0) 
          {
	   return sock;
	  }
	ioctlParams[0] = sock;
	if(ioctl(g_rtfd, RTSOCK_IOCREDIRECTFD, ioctlParams) < 0)
         {
	  perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
	  return -1;
	  }
	return sock;
        }

int 
ResetRtSocket(int sock)
        {
	long ioctlParams[1];
	if(sock < 0) 
          {
	   return sock;
	  }
	ioctlParams[0] = sock;
	if(ioctl(g_rtfd, RTSOCK_IOCRESETFD, ioctlParams) < 0)
          {
	   perror("ioctl(RTSOCK_IOCINTERFACE, destroy)");
	   return -1;
	  }
	return sock;
        }

int
UpdateRtSocket(int sock, int index, int *params, int paramsLength)
       {
	int *ioctlParams, i;
	if(sock < 0 || index < 0)
          {
	   return -1;
	  }
	ioctlParams = calloc(paramsLength+3, sizeof(int));
	ioctlParams[0] = sock;		// fd of sock
	ioctlParams[1] = 1;		// operation
	ioctlParams[2] = index;		// index of mysock in rt space
	if(params) 
          {
	   for(i=0; i<paramsLength; i++) 
              {
	       ioctlParams[3+i] = params[i];
	      }
	  }
	if(ioctl(g_rtfd, RTSOCK_IOCINTERFACE, ioctlParams) < 0)
          {
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
 int  sock, rtsock, i, count, params[2], fd;
 int  addrLen,sum=0,qsum=0,sum1=0,qsum1=0,sum2=0,qsum2=0;
 struct packet pp;
 struct sockaddr_in addr;
 float tm,tm1,tm2,stm,stm1,stm2;
 char *s;
 FILE* ff;
 struct tmp
	{
	char x[20];
	float vr;
	};
 struct tmp tv[3];

             /* socket code
	if(argc < 2) 
          {
	   fprintf(stderr, "usage: rtsocku [ru|rr|tu|tr] [<address>]\n");
	   exit(-1);
	  }
	s = (argc < 3)? "" : argv[2];
	if(strcmp(argv[1], "ru") == 0) 
          {
	   if((sock = BindSocket(CreateUDPSocket(), s)) < 0) 
             {
	      exit(-1);
	     }
	   addrLen = sizeof(addr);
	   if(getsockname(sock, &addr, &addrLen) < 0 ) 
             {
              close(sock);
	      return -1;
	     }
	   count = 0;
	   for(;;) 
              {
	       recv(sock, &p, sizeof(p), 0);
              }
	   } 
        else if(strcmp(argv[1], "tu") == 0) 
           {
	    if((sock = ConnectSocket(BindSocket(CreateUDPSocket(), ""), s)) < 0)
              {
	       exit(-1);
	      } */

             //**********newcode

 float dist1,dist2,dist3=0.0,avgdist1,avgdist2,avgdist3;
 int skfd,jj=0,tt,hh;
 if((ff=fopen("/root/distancestable.c","w"))==NULL)
		{
		printf("cannot open this file\n");exit(-1);
		}
           // calculate the distance 10 times
 
 for (hh=0;hh<10;hh++)
     {
     if((skfd = iw_sockets_open()) < 0)
       {
        perror("socket");
        return(-1);
       }
     for(jj=0;jj<100;jj++)
        {
         char str[20]="";
         float avg;
         tt=0;
         iw_enum_devices(skfd, &print_spy_info, NULL, 0);
         for(i = 0; i < n; i++)
            {
             if(strncmp(str,iw_pr_ether(temp, hwa[i].sa_data),17)!=0)
               {
                strncpy(pp.mc1,iw_pr_ether(temp, hwa[i].sa_data),17);
                pp.mc1[17]='\0';
                printf("\n");
                if(strcmp(pp.mc1,"00:0C:F1:31:D8:D6")==0 || strcmp(pp.mc1,"00:02:2D:1C:57:79")==0 || strcmp(pp.mc1,"00:02:2D:1C:57:92")==0)
                  {
                   pp.sl=qual[i].level-0x100;
                   strncpy(str,iw_pr_ether(temp, hwa[i].sa_data),17);
                   str[17]='\0';

           //********* read signal strength

	           if(jj<99)
                     {
                      if(tt==0)
		        {
                         printf("MAC1= %s : %d\t",pp.mc1,pp.sl);
			 fprintf(ff,"MAC1= %s : %d\n",pp.mc1,pp.sl);
                         sum=pp.sl+sum;
		         qsum=pp.sl*pp.sl + qsum;
                        }
                      else 
                      if(tt==1)
                        {
                         printf("MAC2= %s : %d\t",pp.mc1,pp.sl);
                         fprintf(ff,"MAC2= %s : %d\n",pp.mc1,pp.sl);
			 sum1=pp.sl+sum1;
			 qsum1=pp.sl*pp.sl + qsum1;
                        }
                      else 
                      if(tt==2)
                        {
                         printf("MAC3= %s : %d\n",pp.mc1,pp.sl);
			 fprintf(ff,"MAC3= %s : %d\n",pp.mc1,pp.sl);
                         sum2=pp.sl+sum2;
			 qsum2=pp.sl*pp.sl + qsum2;
                        }
                      tt++;
                    }
                  else
                   {
                      strcpy(tv[tt].x,pp.mc1);
                      if (tt==0)
                        {
                         sum=pp.sl+sum;stm=sum;
                         qsum=pp.sl*pp.sl + qsum; tm=qsum;
                         tv[tt].vr=tm/100 - stm*stm/10000;
                         avg=stm/100;
                         printf("avg level MAC1= %.2f\n", avg);
			 printf("var MAC1= %.2f\n",tv[tt].vr);
                         fprintf(ff,"var MAC1= %.2f\t avg MAC1 = %.2f\n",tv[tt].vr,avg); /* print variance on screen */
                         //printf("MAC1= %s : %d\n",pp.mc1,pp.sl);
                        }
                      else 
                      if (tt==1)
                        { 
                         sum1=pp.sl+sum1;stm1=sum1;
                         qsum1=pp.sl*pp.sl + qsum1;tm1=qsum1;
                         tv[tt].vr=tm1/100 - stm1*stm1/10000;
                         avg=stm1/100;
                         printf("avg level MAC2= %.2f\n", avg);
			 printf("var MAC2= %.2f\n",tv[tt].vr);
			 fprintf(ff,"var MAC2= %.2f\t avg MAC2 = %.2f\n",tv[tt].vr,avg); /* print variance on screen */
                         //printf("MAC2= %s : %d\n",pp.mc1,pp.sl);
                        }
                     else 
                     if (tt==2)
                        {
                         sum2=pp.sl+sum2;stm2=sum2;
                         qsum2=pp.sl*pp.sl + qsum2;tm2=qsum2;
                         tv[tt].vr=tm2/100 - stm2*stm2/10000;
                         avg=stm2/100;
                         printf("avg level MAC3= %.2f\n", avg);
    			 printf("var MAC3= %.2f\n",tv[tt].vr);
			 fprintf(ff,"var MAC3 = %.2f\t  avg MAC3 = %.2f\n",tv[tt].vr,avg); /* print variance on screen */
                         //printf("MAC3= %s : %d\n",pp.mc1,pp.sl);
                        }
                     tt++;
                    }  // close else of 100
               } //strcmp close
            } //to avoid repeat mac address
         }  // for all hardware mac (n)
      usleep(100000);

     } // for close 10 readings from hardware
   close(skfd);
 
              // store the data in file

 float pl=-12.4,p0=47.76; /* - tv[0].vr */
 tv[0].vr=pow(10,(stm/100  +  p0)/pl);
 //printf("dist 1 = %f \n", tv[0].vr); /* print distance on screen */
 tv[1].vr=pow(10,(stm1/100  + p0)/pl);
 tv[2].vr=pow(10,(stm2/100 + p0)/pl);
      //sum of distances
 dist1=dist1+tv[0].vr;
 dist2=dist2+tv[1].vr;
 dist3=dist3+tv[2].vr;
 printf("Nohh= %d dist1= %f\t  dist2= %f\t  dist3= %f\n",hh,tv[0].vr,tv[1].vr,tv[2].vr);
 fprintf(ff,"hh=%d dist1= %f\t  dist2= %f\t  dist3= %f\n",hh,tv[0].vr,tv[1].vr,tv[2].vr);
 sum=0;sum1=0;sum2=0;
 qsum=0;qsum1=0;qsum2=0;

  } // close the repeat 10 times distance calculation, hh
 printf("\n");
 printf("\n");
 avgdist1=dist1/10;avgdist2=dist2/10;avgdist3=dist3/10;
 printf("avgdist1= %f\t  avgdist2= %f\t  avgdist3= %f\n",avgdist1,avgdist2,avgdist3);
 fprintf(ff,"avgdist1= %f\t  avgdist2= %f\t  avgdist3= %f\n",avgdist1,avgdist2,avgdist3);
               /**************************************/

     /*The New code for 3D & 2D starts here */
 float a[4],b[4],c[4];
 float f,g;
 float q12,q13,q23,x12,x13,x23,y12,y13,y23,z12,z13,z23;
 int nn;
 /* Assume distance to MAC3 has been determined and it is accurate  */
 //avgdist1=3.309456;
 avgdist3=6.220128616;
 a[0]=0;a[1]=0;a[2]=0;a[3]= pow(avgdist2,2); /* coordinates and distance for node 1 */
 b[0]=4;b[1]=5;b[2]=0;b[3]= pow(avgdist1,2); /* coordinates and distance for node 2 */
 c[0]=-4;c[1]=5;c[2]=0;c[3]= pow(avgdist3,2); /* coordinates and distance for node 3 */

 x12=b[0]-a[0];x13=c[0]-a[0];x23=c[0]-b[0]; /* difference of x coord */
 y12=b[1]-a[1];y13=c[1]-a[1];y23=c[1]-b[1]; /* difference of y coord */
 z12=b[2]-a[2];z13=c[2]-a[2];z23=c[2]-b[2]; /* difference of z coord */

 q12= a[3]-b[3]+pow(b[0],2)-pow(a[0],2)+ pow(b[1],2)-pow(a[1],2)+pow(b[2],2)-pow(a[2],2); 
 q12=q12/2; /*  RHS of 1st equation   */
 q13= a[3]-c[3]+pow(c[0],2)-pow(a[0],2)+ pow(c[1],2)-pow(a[1],2)+pow(c[2],2)-pow(a[2],2);
 q13=q13/2; /*  RHS of 2nd equation   */
 q23= b[3]-c[3]+pow(c[0],2)-pow(b[0],2)+ pow(c[1],2)-pow(b[1],2)+pow(c[2],2)-pow(b[2],2);
 q23=q23/2; /*  RHS of 3rd equation   */

 nn = 2;
 if (((a[nn]) ==0.0) && ((b[nn]) ==0.0) && ((c[nn]) ==0.0)) /* test for 2D or 3D */
        /******************************************************************/
	/*    generate three distance equation of each known nodes to     */
	/*    the unknown node.                                           */
	/******************************************************************/
 	{
 	float ux,uy,uz; /* 2D calculations  */
 	/***************************************************************/
 	/* Equations are of the form:				       */
 	/*       x12 ux  + y12 uy   = q12			       */
	/*       x13 ux  + y13 uy   = q13                              */
 	/*	 x23 ux  + y23 uy   = q23                              */
 	/* Only two equations will be used because of only two unknown */
 	/* values. Cancellation and substitution of at least two       */
 	/* equations will obtain a solution for x and then substitute  */
 	/* for y.	     					       */
 	/***************************************************************/
 	f = x13 - (y13*x12)/y12;
 	g = q13- (y13*q12)/y12;
 	ux = g/f;
 	uy = (q12 - (x12*ux))/y12;
 	uz = 0.0;
 	printf("\n My  2D coordinates are: %.1f %.1f %.1f\n",ux,uy,uz);
 	printf("\n");
 	}
 else
	{    
        float h,l,m,t,u,e,j,k,p,hh,gg; /* 3D calculations  */
        float uz1,uz2,ux1,ux2,uz,uy1,uy2,xx1,yy1,zz1;
	printf("\n x12= %f",x12);
	/***********************************************************************/
	/* The simplified distance equations are of the form:		       */
 	/*       x12 ux  + y12 uy  +  z12 uz  = q12			       */
 	/*       x13 ux  + y13 uy  +  z13 uz  = q13                            */
 	/*	  x23 ux  + y23 uy  +  z23 uz  = q23                           */
 	/* All three equations will be used.                                   */
 	/* Cancellation and substitution of two equations  will obtain a       */
 	/* quadratic equation for z which will then used to obtain one or      */
 	/* two solutions. This/these  values will then be substituted into     */
 	/* the original distance equation to obtain values for x and y.        */
 	/***********************************************************************/

	f = q13 - q12*x13/x12;
	g = z13 - z12*x13/x12;
	h = y13 - x13*y12/x12;
	l = q12/x12;
	m = x12*h;
	t = l - (y12*f)/m;
	u = (y12*g)/h - z12;
	u = u/x12;
	printf("\n f,u,g,h=%f, %f,%f,%f",f,u,g,h);
	e = pow(u,2) + (pow(g,2)/pow(h,2)) + 1;
	printf("\n the e= %f",e);
	j = - 2*a[2] - 2*u*a[0] - 2*g*f/pow(h,2) + 2*a[1]*g/h + 2*t*u;
	k=pow(t,2) + pow(a[0],2) - 2*t*a[0] + pow(f,2)/pow(h,2) + pow(a[1],2) - 2*a[1]*f/h + pow(a[2],2) - a[3];
	hh = pow(j,2);
 	gg = 4*e*k;
	p=pow(j,2) - 4*e*k;

        if(hh == gg) /* solutions of quadratic equations are same and real */
          {
    	   uz1 = -j/(2*e);
    	   uz2 = uz1;
    	   ux1 = t + (uz1*u);
    	   ux2 = ux1;
           uy1 = f - g*uz1;
           uy1 = uy1/h;
           uy2 = uy1;
           printf("\n My  3D coordinates are: %.1f .1%f %.1f\n",ux1,uy1,uz1);
          }
        else
        if (hh > gg) /* solutions of quadratic equations are real but not equal */
          {
           float distance_calc1; 
           uz = sqrt(p);
           uz1 = uz - j;
           uz1 = uz1/(2*e);
	   ux1 = t + uz1*u;
	   uy1 = f - (g*uz1);
           uy1 = uy1/h;
           uz2 = -j - uz;
	   uz2 = uz2/(2*e);
           ux2 = t + uz2*u;
           uy2 = f - (g*uz1);
           uy2 = uy2/h;
           /*********************************************************************/
           /* to determine which solution for z satisfies the distance formula  */
           /*********************************************************************/
           xx1 = ux1 - a[0]; /* difference in x */
           yy1 = uy1 - a[1]; /* difference in y */
           zz1 = uz1 - a[2]; /* difference in z */

           distance_calc1 = pow(xx1,2) + pow(yy1,2) + pow(zz1,2);
           if (distance_calc1 == a[3])
	       printf("\n My 3D coordinates are:%.1f %.1f %.1f\n",ux1,uy1,uz1);
           else 
               printf("\n My  3D coordinates are:%.1f %.1f %.1f\n",ux2,uy2,uz2);
           }
        else 
        if (hh < gg) 
            printf ("\n My  3D coordinates cannot be determined \n  "); 
        
  }
 
 	
//********	 

//*********end newcode

//close the send tu pract 
  
 return 0;
}
