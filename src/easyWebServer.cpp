#include <Arduino.h>

extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#include "easyWebServer.h"
#include "FS.h"

espconn webServerConn;
esp_tcp webServerTcp;

void webServerDebug( const char* format ... ) {
    char str[200];
    
    va_list args;
    va_start(args, format);
    
    vsnprintf(str, sizeof(str), format, args);
    Serial.print( str );
    
    va_end(args);
}

//***********************************************************************
void ICACHE_FLASH_ATTR webServerInit( void ) {
    webServerConn.type = ESPCONN_TCP;
    webServerConn.state = ESPCONN_NONE;
    webServerConn.proto.tcp = &webServerTcp;
    webServerConn.proto.tcp->local_port = WEB_PORT;
    espconn_regist_connectcb(&webServerConn, webServerConnectCb);
    sint8 ret = espconn_accept(&webServerConn);
    
    SPIFFS.begin(); // start file system for webserver
    
    if ( ret == 0 )
        webServerDebug("web server established on port %d\r\n", WEB_PORT );
    else
        webServerDebug("web server on port %d FAILED ret=%d\r\n", WEB_PORT, ret);
    
    return;
}

//***********************************************************************
void ICACHE_FLASH_ATTR webServerConnectCb(void *arg) {
  struct espconn *newConn = (espconn *)arg;

  espconn_regist_recvcb(newConn, webServerRecvCb);
  espconn_regist_sentcb(newConn, webServerSentCb);
  espconn_regist_reconcb(newConn, webServerReconCb);
  espconn_regist_disconcb(newConn, webServerDisconCb);
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerRecvCb(void *arg, char *data, unsigned short length) {
    
    char outGoing[500];
    uint16_t outIndex = 0;
    struct espconn *activeConn = (espconn *)arg;
    
    char get[5] = "GET ";
    char path[200];
    //webServerDebug("request=%s\n", data);
    if ( strstr( data, get) != NULL ) {
        char *endOfPath = strstr( data, " HTTP" );
        uint32_t pathLength = endOfPath - ( data + strlen( get ) );
        strncpy( path, data + strlen( get ), pathLength );
        path[ pathLength ] = 0;
        //webServerDebug("0path=%s pathLength=%u\n", path, pathLength );
        if( strcmp( path, "/" ) == 0 ) {
            strcpy( path, "/index.html");
        }
        //webServerDebug("1 path=%s\n", path );
    }
    
    char ch;
    char header[200];
    char *extension = strstr( path, ".") + 1;
    
    //webServerDebug("extension=%s<--\n", extension);
    
    File f = SPIFFS.open( path, "r" );
    if ( !f ) {
        //webServerDebug("webServerRecvCb(): file not found ->%s\n", path);
        sprintf( outGoing, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h2>File Not Found!</h2>");
    }
    else {
        //webServerDebug("22file Size=%u\n", f.size() );
        //webServerDebug("path=%s\n", path );
        
        sprintf( header, "%s", "HTTP/1.0 200 OK\r\n" );
        sprintf( header, "%s%s", header, "Server: easyWebServer\r\n");
        
        if ( strcmp( extension, "html") == 0 )
            strcat( header, "Content-Type: text/html\r\n" );
        else if ( strcmp( extension, "css") == 0 )
            strcat( header, "Content-Type: text/css\r\n" );
        else if ( strcmp( extension, "js")  == 0 )
            strcat( header, "Content-Type: application/javascript\r\n" );
        else if ( strcmp( extension, "png")  == 0 )
            strcat( header, "Content-Type: image/png\r\n" );
        else if ( strcmp( extension, "jpg")  == 0 )
            strcat( header, "Content-Type: image/jpeg\r\n" );
        else if ( strcmp( extension, "gif")  == 0 )
            strcat( header, "Content-Type: image/gif\r\n" );
        else if ( strcmp( extension, "ico")  == 0 )
            strcat( header, "Content-Type: image/x-icon\r\n" );
        else
            //webServerDebug("webServerRecvCb(): Wierd file type. path=%s", path );
        
        strcat( header, "Content-Length: " );
        sprintf(header, "%s%u\r\n\r\n", header, f.size() );

        //webServerDebug("header len=%u header=\n%s\n", strlen(header), header );
        
        outIndex = strlen(header);
        strncpy(outGoing, header, outIndex );
        
        while ( f.available() ) {
            outGoing[outIndex++] = f.read();
            //msgLength++;
        }
        
        outGoing[outIndex] = 0;
        
        //webServerDebug("outGoing=\n%s\n", outGoing);
    }
    
    espconn_send(activeConn, (uint8*)outGoing, outIndex);
    espconn_disconnect( activeConn );
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerSentCb(void *arg) {
  //data sent successfully
//  DEBUG_MSG("webServer sent cb \r\r\n");
  struct espconn *requestconn = (espconn *)arg;
  espconn_disconnect( requestconn );
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerDisconCb(void *arg) {
//  DEBUG_MSG("In webServer_server_discon_cb\r\n");
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerReconCb(void *arg, sint8 err) {
//  DEBUG_MSG("In webServer_server_recon_cb err=%d\r\n", err );
}








