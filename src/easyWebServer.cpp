#include <Arduino.h>
#include <SimpleList.h>

extern "C" {
#include "user_interface.h"
#include "espconn.h"
#include "osapi.h"
}

#include "easyWebServer.h"
#include "FS.h"


espconn webServerConn;
esp_tcp webServerTcp;

SimpleList<webServerConnectionType> connections;

os_timer_t  cleanUpTimer;

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
    
    os_timer_setfn( &cleanUpTimer, &cleanUpCb, NULL);
    os_timer_arm( &cleanUpTimer, 1000, 1 );

    return;
}

//***********************************************************************
void ICACHE_FLASH_ATTR cleanUpCb( void *arg ) {
    uint32_t now = system_get_time();
    
    SimpleList<webServerConnectionType>::iterator connection = connections.begin();
    while ( connection != connections.end() ) {
        if ( now > connection->timeCreated + CONNECTION_TIMEOUT ) {
            webServerDebug("cleanUpCb(): cleaned out idle connection=0x%x\n", connection->esp_conn);
            espconn_disconnect( connection->esp_conn );
            connection = connections.erase( connection );
        }
        else {
            connection++;
        }
    }
}

//***********************************************************************
void ICACHE_FLASH_ATTR webServerConnectCb(void *arg) {
    webServerConnectionType newConn;
    newConn.esp_conn = (espconn *)arg;
    
    espconn_regist_recvcb(newConn.esp_conn, webServerRecvCb);
    espconn_regist_sentcb(newConn.esp_conn, webServerSentCb);
    espconn_regist_reconcb(newConn.esp_conn, webServerReconCb);
    espconn_regist_disconcb(newConn.esp_conn, webServerDisconCb);
    
    newConn.timeCreated = system_get_time();
    
    connections.push_back(newConn);
    
    webServerDebug("webServerConnectCb(): registered new connection conn=0x%x\n", arg);
}

//***********************************************************************
webServerConnectionType* ICACHE_FLASH_ATTR webServerFindConn( espconn *conn ) {
    webServerDebug("webServerFindConn() findConn=0x%x\n", conn );
    
    int i = 0;
    webServerConnectionType *ret = NULL;
    
    SimpleList<webServerConnectionType>::iterator connection = connections.begin();
    while ( connection != connections.end() ) {
        webServerDebug("\twebServerFindConn() %d, conn=0x%x", i++, connection->esp_conn );
        if ( connection->esp_conn == conn ) {
            webServerDebug(" <--- ");
            ret = connection;
        }
        webServerDebug("\n");
        connection++;
    }
    
    if ( ret == NULL )
        webServerDebug("findConnection(espconn): Did not Find\n");
    
    return ret;
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerRecvCb(void *arg, char *data, unsigned short length) {
    
//    char *outGoing;
//    uint16_t outIndex = 0;
//    struct espconn *activeConn = (espconn *)arg;
    webServerConnectionType *conn = webServerFindConn( (espconn*)arg );
    if ( conn == NULL ) {
        webServerDebug("webServerRecvCb(): err - could not find Connection?\n");
        return;
    }
    
    char get[5] = "GET ";
    char path[200];
    
    webServerDebug("request=\n%s\n\n", data);
    if ( strstr( data, get) != NULL ) {   // GET request
        char *endOfPath = strstr( data, " HTTP" );
        uint32_t pathLength = endOfPath - ( data + strlen( get ) );
        strncpy( path, data + strlen( get ), pathLength );
        path[ pathLength ] = 0;
        if( strcmp( path, "/" ) == 0 ) {
            strcpy( path, "/index.html");
        }
        webServerDebug("path=%s\n", path );
    } else {
        webServerDebug("webServerRecvCb(): request not GET ->%s\n", data);
        sprintf( conn->buffer, "HTTP/1.0 501 Not Implemented\r\nContent-Type: text/html\r\n\r\n<h2>Can only accept ->GET<- requests</h2>");
        espconn_send(conn->esp_conn, (uint8*)conn->buffer, strlen(conn->buffer));
        return;
    }
    
    //char ch;
    //char header[200];
    char *extension = strstr( path, ".") + 1;
    
    webServerDebug("extension=%s<--\n", extension);
    
    conn->file = SPIFFS.open( path, "r" );
    if ( !conn->file ) {
        webServerDebug("webServerRecvCb(): file not found ->%s\n", path);
        sprintf( conn->buffer, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h2>File Not Found!</h2>");
        espconn_send(conn->esp_conn, (uint8*)conn->buffer, strlen(conn->buffer));
        return;
    }
    
    webServerDebug("file Size=%u\n", conn->file.size() );
    webServerDebug("path=%s\n", path );
    
    sprintf( conn->buffer, "%s", "HTTP/1.0 200 OK\r\n" );
    sprintf( conn->buffer, "%s%s", conn->buffer, "Server: easyWebServer\r\n");
    
    if ( strcmp( extension, "html") == 0 )
        strcat( conn->buffer, "Content-Type: text/html\r\n" );
    else if ( strcmp( extension, "css") == 0 )
        strcat( conn->buffer, "Content-Type: text/css\r\n" );
    else if ( strcmp( extension, "js")  == 0 )
        strcat( conn->buffer, "Content-Type: application/javascript\r\n" );
    else if ( strcmp( extension, "png")  == 0 )
        strcat( conn->buffer, "Content-Type: image/png\r\n" );
    else if ( strcmp( extension, "jpg")  == 0 )
        strcat( conn->buffer, "Content-Type: image/jpeg\r\n" );
    else if ( strcmp( extension, "gif")  == 0 )
        strcat( conn->buffer, "Content-Type: image/gif\r\n" );
    else if ( strcmp( extension, "ico")  == 0 )
        strcat( conn->buffer, "Content-Type: image/x-icon\r\n" );
    else {
        webServerDebug("webServerRecvCb(): Unrecognized file type. path=%s\n", path );
        sprintf( conn->buffer, "HTTP/1.0 415 Unsupported Media Type\r\nContent-Type: text/html\r\n\r\n<h2>Unsupported Media Type!</h2>");
        espconn_send(conn->esp_conn, (uint8*)conn->buffer, strlen(conn->buffer));
        return;
    }
    
    strcat( conn->buffer, "Content-Length: " );
    sprintf(conn->buffer, "%s%u\r\n\r\n", conn->buffer, conn->file.size() );
    
    uint16_t bufferIndex = strlen( conn->buffer );
    while ( conn->file.available() && bufferIndex < ( BUFFER_SIZE - 1) ) {
        conn->buffer[ bufferIndex++ ] = conn->file.read();
    }
    conn->buffer[bufferIndex] = 0;
    
    sint8 err = espconn_send(conn->esp_conn, (uint8*)conn->buffer, bufferIndex );
    if ( err != 0 ) {
        webServerDebug("webServerRecvCb(): espconn_send failed err=%d\n", err);
    }
    
    return;
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerSentCb(void *arg) {
  //data sent successfully
    webServerConnectionType *conn = webServerFindConn( (espconn*)arg );
    if ( conn == NULL ) {
        webServerDebug("webServerSentCb(): err - could not find Connection?\n");
        return;
    }
    
    if ( !conn->file ) {
        webServerDebug("webServerSentCb(): no file, service completed\n");
        espconn_disconnect( conn->esp_conn );
        connections.erase( conn );
        return;
    }
    
    if ( !conn->file.available() ) {
        webServerDebug("webServerSentCb(): %s completed\n", conn->file.name() );
        espconn_disconnect( conn->esp_conn );
        connections.erase( conn );
        return;
    }
    sprintf( conn->buffer, "");
    
    uint16_t bufferIndex = strlen( conn->buffer );
    while ( conn->file.available() && bufferIndex < ( BUFFER_SIZE - 1) ) {
        conn->buffer[ bufferIndex++ ] = conn->file.read();
    }
    conn->buffer[bufferIndex] = 0;
    
    sint8 err = espconn_send(conn->esp_conn, (uint8*)conn->buffer, bufferIndex );
    if ( err != 0 ) {
        webServerDebug("webServerSentCb(): espconn_send failed err=%d\n", err);
    }

    return;
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerDisconCb(void *arg) {
//  DEBUG_MSG("In webServer_server_discon_cb\r\n");
}

/***********************************************************************/
void ICACHE_FLASH_ATTR webServerReconCb(void *arg, sint8 err) {
//  DEBUG_MSG("In webServer_server_recon_cb err=%d\r\n", err );
}








