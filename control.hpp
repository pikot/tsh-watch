//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _CONTROL_
#define _CONTROL_

#include "version.h"

#if defined  IS_V1_3 || defined  IS_V1_4
#define CONTROL_LEFT_BUTTON   34
#define CONTROL_RIGHT_BUTTON  36
#define CONTROL_OK_BUTTON     39 
#else
#define CONTROL_LEFT_BUTTON   14
#define CONTROL_RIGHT_BUTTON  33
#define CONTROL_OK_BUTTON     27     
#endif

#endif
