#ifndef TRG_GPS_H
#define TRG_GPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iwlib.h"		/* Header */
#include <sys/time.h>
#include "rtsock.h" 


int
DecodeIPAddress(char *s, struct sockaddr_in *sin);

char *
EncodeIPAddress(char *s, struct sockaddr_in *sin);

int
CreateUDPSocket();


void
DestroySocket(int sock);
void status(const char *stat);
void *waitREQUEST();
const char* getRefer();

int
BindSocket(int sock, char *s);


int
ConnectSocket(int sock, char *s); 

int
CreateRtSocket(int sock, int *params, int paramsLength);

void
DestroyRtSocket(int sock, int index);


int
RedirectRtSocket(int sock);

int
ResetRtSocket(int sock);

int
UpdateRtSocket(int sock, int index, int *params, int paramsLength);


void *tracker(void *threadid);
//void request(int stat);
#ifdef __cplusplus
}
#endif
#endif

