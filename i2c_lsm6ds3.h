//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _I2C_LSM6DS3_
#define _I2C_LSM6DS3_

#define  LSM6DS3_ADDR 0x6A

#define  LSM6DS3_ACC_GYRO_WHO_AM_I_REG 0X0F

#define  LSM6DS3_ACC_GYRO_WHO_AM_I   0X69
#define  LSM6DS3_C_ACC_GYRO_WHO_AM_I 0x6A

// * Register      : CTRL1_XL,  * Address       : 0X10
#define  LSM6DS3_ACC_GYRO_FS_XL_2g 0x00
#define  LSM6DS3_ACC_GYRO_FS_XL_4g 0x08

#define  LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN 0x00
//.set LSM6DS3_ACC_GYRO_ODR_XL_12Hz, 0x10
#define  LSM6DS3_ACC_GYRO_ODR_XL_26Hz 0x20
//.set LSM6DS3_ACC_GYRO_ODR_G_12Hz, 0x10
//.set LSM6DS 3_ACC_GYRO_PEDO_RST_STEP_ENABLED, 0x02

// * Register      : CTRL2_G,  * Address       : 0X11
#define  LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN 0x00

#define  LSM6DS3_ACC_GYRO_XL_HM_MODE  0x10
// * Register      : CTRL7_G,  * Address       : 0X16  
#define  LSM6DS3_ACC_GYRO_G_HM_MODE   0x80     

/************** Device Register  *******************/
#define  LSM6DS3_ACC_GYRO_INT1_CTRL 0X0D
#define  LSM6DS3_ACC_GYRO_CTRL1_XL 0X10
#define  LSM6DS3_ACC_GYRO_CTRL2_G  0X11
#define  LSM6DS3_ACC_GYRO_CTRL6_C  0X15
#define  LSM6DS3_ACC_GYRO_CTRL7_G  0X16

#define  LSM6DS3_ACC_GYRO_CTRL10_C 0X19
#define  LSM6DS3_ACC_GYRO_TAP_CFG1 0X58
#define  LSM6DS3_ACC_GYRO_STEP_COUNTER_L 0X4B
#define  LSM6DS3_ACC_GYRO_STEP_COUNTER_H 0X4C
/********/

#endif
