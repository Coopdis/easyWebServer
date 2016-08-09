#ifndef   _MESH_WEB_SERVER_H_
#define   _MESH_WEB_SERVER_H_

#define WEB_PORT          80

void inline   webServerDebug( const char* format ... ) {
    char str[200];
    
    va_list args;
    va_start(args, format);
    
    vsnprintf(str, sizeof(str), format, args);
    Serial.print( str );
        
    va_end(args);
}

void webServerInit( void );
void webServerConnectCb(void *arg);
void webServerRecvCb(void *arg, char *data, unsigned short length);
void webServerSentCb(void *arg);
void webServerDisconCb(void *arg);
void webServerReconCb(void *arg, sint8 err);

#endif //_MESH_WEB_SERVER_H_
