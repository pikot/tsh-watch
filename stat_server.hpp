#define NEED_SERVER_STAT 

#ifdef NEED_SERVER_STAT

#ifndef _STAT_SERVER_
#define _STAT_SERVER_

#include "WiFiClientSecure.h"

//#define SERVER      "vesovoy-control.ru"
#define PORT        443

#define BOUNDARY    "--------------------------133747188241686651551404"  
#define TIMEOUT     20000

String makeHeader(String header, String uid, String token, String url, size_t length);
String   makeBody(String content , String message);

void     sendStatFile(String header, String uid, String token, String date, String filename);
int32_t    getLastDate(String header, String uid, String token);

#endif // _STAT_SERVER_

#endif  // NEED_SERVER_STAT
