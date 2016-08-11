#ifndef   _EASY_WEB_SERVER_H_
#define   _EASY_WEB_SERVER_H_

extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#include "FS.h"


#define WEB_PORT                80
#define BUFFER_SIZE             1400
#define CONNECTION_TIMEOUT      3000000  //microseconds

struct webServerConnectionType {
    espconn     *esp_conn;
    File        file;
    char        buffer[BUFFER_SIZE];
    uint32_t    timeCreated;
};

void webServerDebug( const char* format ... );

void webServerInit( void );
webServerConnectionType* ICACHE_FLASH_ATTR webServerFindConn( espconn *conn );

void webServerConnectCb(void *arg);
void webServerRecvCb(void *arg, char *data, unsigned short length);
void webServerSentCb(void *arg);
void webServerDisconCb(void *arg);
void webServerReconCb(void *arg, sint8 err);

void ICACHE_FLASH_ATTR webServerCleanUpCb( void *arg );

#endif //_EASY_WEB_SERVER_H_
