//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#define NEED_SERVER_STAT 

#ifdef NEED_SERVER_STAT

#ifndef _STAT_SERVER_
#define _STAT_SERVER_

#include "WiFiClientSecure.h"

#define PORT        443

#define BOUNDARY    "--------------------------133747188241686651551404"  
#define TIMEOUT     20000


enum stat_type_t{
    STAT_AGG_SENTORS = 0,
    STAT_RAW_HR,
    STAT_MAX
};

String makeHeader(String header, String uid, String token, String url, size_t length);
String   makeBody(String content , String message);

bool     sendStatFile(String header, String uid, String token, String date, String filename, String urlDir);
int32_t    getLastDate(String header, String uid, String token);

#endif // _STAT_SERVER_

#endif  // NEED_SERVER_STAT
