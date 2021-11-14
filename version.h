//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _VERSION_
#define _VERSION_

#define IS_V1_3 1

#ifdef IS_V1_3
#define CURRENT_LOG_VERSION 2
#define ULP_SCL_PIN 9 
#define ULP_SDA_PIN 8
#else 
#define CURRENT_LOG_VERSION 1
#define ULP_SCL_PIN 9 
#define ULP_SDA_PIN 6
#endif

#define CURRENT_HR_VERSION 1

#define MAIN_CORE_INIT_SENSORS 1 

#define SENSOR_STATUS_OK         0x00
#define SENSOR_STATUS_HW_ERROR   0X01
#define SENSOR_STATUS_CFG_ERROR  0X02
#define SENSOR_STATUS_READ_ERROR 0X03
#define SENSOR_STATUS_INIT_ERROR 0X04

#endif
