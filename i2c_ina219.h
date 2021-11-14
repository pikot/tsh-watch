//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _I2C_INA219_
#define _I2C_INA219_

#define INA219_ADDR 0x40

#define INA219_REG_CONFIG       0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE   0x02
#define INA219_REG_POWER        0x03
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

#define INA219_CONFIG_GAIN_8_320MV   0x1800 // Gain 8, 320mV Range
#define INA219_CONFIG_BADCRES_12BIT  0x0180 // 12-bit bus res = 0..4097
#define INA219_CONFIG_SADCRES_12BIT_1S_532US     0x0018 // 1 x 12-bit shunt sample
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS  0x07 //< shunt and bus voltage continuous 
#define INA219_CONFIG_BVOLTAGERANGE_16V 0x0000 // 0-16V Range
#define INA219_CONFIG_BVOLTAGERANGE_32V 0x2000 // 0-32V Range
//.set INA219_CONFIG_MODE_POWERDOWN     0x00
#define INA219_CONFIG_MODE_POWERDOWN_MASK 0xFFF8 // we don't  have abiliti to call Logical NOT operation

#endif
