//To get this info in "C", requires being familiar with the TCP/IP
//network routines. It also can be platform specific. The following
//program works on my Solaris 7 box. For more info, read Chapter 8 of Unix
//Network Programming by W. Richhard Stevens: 

//(I placed the include files in double quotes simply because the form wipes out anything in double braces . Please change back to normal "C")*/

#include "iwlib.h"
#include "stdio.h"
#include "sys/socket.h" 
#include "netinet/in.h"
#include "netdb.h"
#include "arpa/inet.h" /* inet_ntoa repository */

void main(argc, argv)
int argc;
char **argv;
{

struct hostent *hp;
struct in_addr *ptr;

/* get the server name */
if ((hp = gethostbyname("skinner")) == NULL)
{
fprintf(stderr, "%s: server unknown.\n", *argv);
exit (1);
}

/* get the address */
if(hp->h_addrtype = AF_INET)
{
while((ptr=(struct in_addr *) *hp->h_addr_list++) != NULL)
fprintf(stderr, "address: %s \n", inet_ntoa(*ptr));
printf("%s\n",inet_ntoa(*ptr));
}

exit(0);
}

