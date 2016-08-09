#include <Arduino.h>

extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#include "easyWebServer.h"
#include "FS.h"

espconn webServerConn;
esp_tcp webServerTcp;

char outGoing[500];
uint16_t outIndex = 0;

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
    //received some data from webServer connection
    String request( data );
    
    struct espconn *activeConn = (espconn *)arg;
    
    String get("GET ");
    String path;
    
    if ( request.startsWith( get ) ) {
        uint16_t endFileIndex = request.indexOf(" HTTP");
        path = request.substring( get.length(), endFileIndex );
        if( path.equals("/") )
            path = "/index.html";
    }
    
    String msg = "";
    char ch;
    uint16_t msgLength = 0;
    
    String header;
    String extension = path.substring( path.lastIndexOf(".") + 1 );
    
    File f = SPIFFS.open( path, "r" );
    if ( !f ) {
        webServerDebug("webServerRecvCb(): file not found ->%s\n", path.c_str());
        msg = "File-->" + path + "<-- not found - this should be 404\r\n";
    }
    else {
        webServerDebug("file Size=%u\n", f.size() );
        webServerDebug("path=%s\n", path.c_str() );
        
        header += "HTTP/1.0 200 OK\r\n";
        //header += "Server: easyWebserver on esp8266\r\n";
        //header += "Date: Wed, 03 Aug 2016 16:58:45 GMT\r\n";
  
        if ( extension.equals("html") )
            header += "Content-Type: text/html\r\n";
        else if ( extension.equals("css") )
            header += "Content-Type: text/css\r\n";
        else if ( extension.equals("js") )
            header += "Content-Type: application/javascript\n";
        else if ( extension.equals("png") )
            header += "Content-Type: image/png\r\n";
        else if ( extension.equals("jpg") )
            header += "Content-Type: image/jpeg\r\n";
        else if ( extension.equals("gif") )
            header += "Content-Type: image/gif\r\n";
        else if ( extension.equals("ico") )
            header += "Content-Type: image/x-icon\r\n";
        else
            webServerDebug("webServerRecvCb(): Wierd file type. path=%s", path.c_str());
        
        header += "Content-Length: ";
        header += f.size(); //msg.length();
        header += "\r\n\r\n";

        webServerDebug("header.length()=%u\n", header.length() );
        
        strncpy(outGoing, header.c_str(), header.length() );
        outIndex = header.length();
        
        while ( f.available() ) {
            outGoing[outIndex++] = f.read();
            msgLength++;
        }
        
        outGoing[outIndex] = 0;
        
        webServerDebug("outGoing=\n%s\n", outGoing);
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








