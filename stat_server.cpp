//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "stat_server.hpp"
#include "LITTLEFS.h"
#include "utils.h"

#ifdef NEED_SERVER_STAT

// ugly but wtf, i don't want to parse all
int parseLastDateResponse(WiFiClientSecure *client, char *lastDate, int lastDateSize) 
{
    int l = -1;

    if (!client->find("HTTP/1.1")) // skip HTTP/1.1
        return l;
    int st = client->parseInt(); // parse status code
    if (st == 200 && client->find("\"ldate\" : \"")) {
        l = client->readBytesUntil('"', lastDate, lastDateSize);
        lastDate[l] = 0; // terminate the C string
    }
    printf_w("parseLastDateResponse: l = %d,  date = %s, stop\n", l, lastDate);

    return l;
}

int32_t getLastDate(String server, String uid, String token)
{
    int32_t l_time = -1;//time_t
    printf_w("getLastDate\n");
    size_t allLen = 0;
    String headerTxt =  makeHeader(server, uid, token, "/action/last_tshdata.php", allLen);
    WiFiClientSecure client;
    if (!client.connect(server.c_str(), PORT)) {
        return l_time;   
    }
    
    printf_w("getLastDate: header -- %s\n", headerTxt.c_str() );

    client.print(headerTxt);
    client.print("\r\n");
   
    delay(20);
    long tOut = millis() + TIMEOUT;
     
    while (client.connected() && tOut > millis()) {
        if (client.available()) {
            char lastDate[32];
            if (-1 == parseLastDateResponse(&client, lastDate, sizeof(lastDate)))
                  break;
            
            printf_w("getLastDate: %s\n", lastDate);            
            l_time = atoi(lastDate);

            break;
        }
    }
    client.stop();

    printf_w("getLastDate: l_time %d\n", l_time);
    return l_time;
}

bool sendStatFile(String server, String uid, String token,  String date, String filename, String urlDir) 
{
    printf_w("sendStatFile: start send -- %s\n", filename.c_str());

    File _file = LITTLEFS.open(filename);
    if (!_file) {
        printf_w("can't open dir '%s'\n", filename);
        return false;
    }
    
    String bodyPic = makeBody("dateFile", date);
    String bodyEnd = String("--") + BOUNDARY + String("--\r\n");

    size_t allLen = bodyPic.length() +  _file.size()  + bodyEnd.length();
    String headerTxt =  makeHeader(server, uid, token, urlDir, allLen);
    printf_w("sendStatFile: header '%s', len %d\n", headerTxt.c_str(), headerTxt.length());
    printf_w("sendStatFile: header '%s', len %d\n", bodyPic.c_str(), bodyPic.length());

    WiFiClientSecure client;
    client.setInsecure();    
    if (!client.connect(server.c_str(), PORT)) 
    {
        printf_w("connection failed %s:%d\n", server.c_str(), PORT);  
        return false;
    }

    
    printf_w("sendStatFile: start send header\n");
    client.print(headerTxt  + bodyPic);

    printf_w("sendStatFile: stop send header\n");


    int ii = 0;
    int nextPacketSize = 0;
    int maxPacketSize  = 2048;
    while ((nextPacketSize = _file.available())) {
        //char _data = _file.read();
        //if (-1 == _data)
          //  break;
        if (nextPacketSize > maxPacketSize) 
            nextPacketSize = maxPacketSize;
        
        String buffer = "";
        for (int i=0; i < nextPacketSize; i++) 
            buffer += (char)_file.read();
        
        client.print( buffer );
        printf_w("sendStatFile: byte %d, iter %d\n", nextPacketSize, ii);
        ii++;
    }
    _file.close();
    printf_w("sendStatFile: stop  send data, %d \n", ii);
    client.print("\r\n" + bodyEnd);
    delay(20);
    long tOut = millis() + TIMEOUT;
    if  (client.connected() && tOut > millis()) {
        char status[64] = {0};
        client.readBytesUntil('\r', status, sizeof(status));
        printf_w("Status: %s\n", status);
        if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
            printf_w( "Unexpected response: %s\n", status);
            client.stop();
            return false;
        }
    }
    while (client.connected() && tOut > millis()) {

        if (client.available()) {
          
       //     String serverRes = client.readStringUntil('\r');
            String serverRes = client.readString();
            printf_w( "RETURN GET:" );
            printf_w( serverRes.c_str() );
            printf_w( "\n" );
            client.stop();
            return true;//(serverRes);
        }
    }
    client.stop();
    return true;
}

String makeHeader(String server, String uid, String token, String url, size_t length)
{
    String  data;
    data =  F("POST ");
    data += url;
    data += F(" HTTP/1.1\r\n");
    data += F("Cache-control: no-cache\r\n");
    data += F("Content-Type: multipart/form-data; boundary=");
    data += BOUNDARY;
    data += "\r\n";
    data += F("User-Agent: tsh-watch/0.0.1\r\n");
    data += F("Accept: */*\r\n");
    data += F("Host: ");
    data += server;
    data += F("\r\n");
    
    data += F("X-uID: ");
    data += uid;
    data += F("\r\n");
    data += F("X-Token: ");
    data += token;
    data += F("\r\n");
    
//    data += F("Accept-encoding: gzip, deflate\r\n");
    data += F("Connection: keep-alive\r\n"); 
    if (length > 0) {
        data += F("Content-Length: ");
        data += String(length);
        data += "\r\n";
    }
    data += "\r\n";
    return(data);
}

String makeBody(String content , String message)
{
    String data;
    data = "--";
    data += BOUNDARY;
    data += F("\r\n");
    if (content == "dateFile") {
        data += F("Content-Disposition: form-data; name=\"dataFile\"; filename=\"");
        data += message;
        data += F(".csv\"\r\n");
        data += F("Content-Type: text/plain\r\n");
        data += F("\r\n");
    }
    else {
        data += "Content-Disposition: form-data; name=\"" + content +"\"\r\n";
        data += "\r\n";
        data += message;
        data += "\r\n";
    }
    return(data);
}
#endif
