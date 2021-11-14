//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _I2C_BME280_
#define _I2C_BME280_

#define BME280_ADDR 0x76 

#define BMP280_CHIP_ID 0x58
#define BME280_CHIP_ID 0x60

#define BME280_REG_T_CAL 0x88
#define BME280_REG_P_CAL 0x8e
#define BME280_REG_H_CAL 0xe1

#define BMP280_REG_ID 0xD0


#define BME280_REG_CTRL_HUMIDITY 0xF2 //Ctrl Humidity Reg
#define BME280_REG_CTRL_MEAS     0xF4
#define BMP280_REG_CONFIG        0xF5
#define BME280_REG_DATA          0xF7

#define BME280_P_T_H_DATA_LEN 0x08
#define BME280_P_T_DATA_LEN   0x06


#define BME280_MODE_SLEEP   0x00
#define BME280_MODE_FORCED  0x01
#define BME280_MODE_NORMAL  0x03
#endif
