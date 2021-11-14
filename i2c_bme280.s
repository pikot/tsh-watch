//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "i2c_bme280.h"
#include "stack.s"

.bss

    .global bme280_error 
bme280_error:
    .long 0

    .global bme280_DEBUG 
bme280_DEBUG:
    .long 0
    
   .global bme280_inited
bme280_inited:
   .long 0
   
#ifndef MAIN_CORE_INIT_SENSORS
    .global bme280_calib_dig_t1
bme280_calib_dig_t1:
    .long 0
    .global bme280_calib_dig_t2
bme280_calib_dig_t2:
    .long 0
    .global bme280_calib_dig_t3
bme280_calib_dig_t3:
    .long 0
    .global bme280_calib_dig_p1
bme280_calib_dig_p1:
    .long 0
    .global bme280_calib_dig_p2
bme280_calib_dig_p2:
    .long 0
    .global bme280_calib_dig_p3
bme280_calib_dig_p3:
    .long 0
    .global bme280_calib_dig_p4
bme280_calib_dig_p4:
    .long 0
    .global bme280_calib_dig_p5
bme280_calib_dig_p5:
    .long 0
    .global bme280_calib_dig_p6
bme280_calib_dig_p6:
    .long 0
    .global bme280_calib_dig_p7
bme280_calib_dig_p7:
    .long 0
    .global bme280_calib_dig_p8
bme280_calib_dig_p8:
    .long 0
    .global bme280_calib_dig_p9
bme280_calib_dig_p9:
    .long 0
    .global bme280_calib_dig_h1
bme280_calib_dig_h1:
    .long 0
    .global bme280_calib_dig_h2
bme280_calib_dig_h2:
    .long 0
    .global bme280_calib_dig_h3
bme280_calib_dig_h3:
    .long 0
    .global bme280_calib_dig_h4
bme280_calib_dig_h4:
    .long 0
    .global bme280_calib_dig_h5
bme280_calib_dig_h5:
    .long 0
    .global bme280_calib_dig_h6
bme280_calib_dig_h6:
    .long 0
#endif

    .global bme280_data_press_MSB_LSB
bme280_data_press_MSB_LSB:
    .long 0
    .global bme280_data_press_XLSB_temp_MSB
bme280_data_press_XLSB_temp_MSB:
    .long 0
    .global bme280_data_temp_LSB_temp_XLSB
bme280_data_temp_LSB_temp_XLSB:
    .long 0
    .global bme280_data_hum_MSB_LSB
bme280_data_hum_MSB_LSB:
    .long 0

.text
    .global bme280_readdata
bme280_readdata:

    move r0, bme280_error  // test for init error SENSOR_STATUS_HW_ERROR
    ld r0, r0, 0x00
    jumpr  bme280_sensor_found, SENSOR_STATUS_HW_ERROR, lt
    jumpr  bme280_sensor_found, SENSOR_STATUS_HW_ERROR, gt
    ret
bme280_sensor_found:

#ifndef MAIN_CORE_INIT_SENSORS
    read_i2c BME280_ADDR BMP280_REG_ID bme280_fail_init

  //  move r2, bme280_DEBUG
  //  st r1, r2, 0

    move r0, r1 // test for BME280_CHIP_ID
//    jumpr bme280_read_calib, BMP280_CHIP_ID, eq
    jumpr bme280_read_calib, BME280_CHIP_ID, eq 

bme280_fail_init:
   store bme280_error SENSOR_STATUS_HW_ERROR
   ret
   
bme280_fail_config:
   store bme280_error SENSOR_STATUS_CFG_ERROR
   ret

//   if_val_noteq bme280_inited 0x1 bme280_read_calib
//   jump bme280_readData  // ugly but otherwise "error: relocation out of range" happened
   if_val_eq bme280_inited 0x1 bme280_readData

bme280_read_calib:    
   
   move r1, i2c_MultyHandler       // set handler for multyread
   move r0, processMultyFromStack
   st r0, r1, 0 

   move r1, BME280_ADDR
   push r1
   move r1, BME280_REG_T_CAL
   push r1 
   move r0, 26
   move r2, bme280_calib_dig_h1  
   psr
   jump readMultyToStack
   add r3,r3,2 // remove call parameters from stack

   // same handler in i2c_MultyHandler == processMultyFromStack, do not set it again
   move r1, BME280_ADDR
   push r1
   move r1, BME280_REG_H_CAL
   push r1 
   move r0, 10
   move r2, bme280_calib_dig_h6
   psr
   jump readMultyToStack
   add r3,r3,2 // remove call parameters from stack

bme280_setSampling: 
   write_i2c BME280_ADDR BME280_REG_CTRL_MEAS 0 bme280_fail_config

   // change in to hardcode !!, to short this stuff )
   move r0, 0x00 // STANDBY // 0x00 - 0.5ms
   lsh r0, r0, 0x05  //
   and r0, r0, 0xE0 // mask
   move r1, 0x00 // FILTER  // 0x00 - filter off
   lsh r1, r1, 0x02  //
   and r1, r1, 0x1C // mask
   or  r0, r0, r1
   write_i2c BME280_ADDR BMP280_REG_CONFIG r0 bme280_fail_config
   
  //Set ctrl_hum first, then ctrl_meas to activate ctrl_hum
    write_i2c BME280_ADDR BME280_REG_CTRL_HUMIDITY 0x01 bme280_fail_config // 1 * oversampling

    move r0, 0x01 // TEMP_OVERSAMPLE // 1 * oversampling
    lsh r0, r0, 0x05  //
    and r0, r0, 0xE0 // mask
    move r1, 0x01 // PRESS_OVERSAMPLE  // 1 * oversampling
    lsh r1, r1, 0x02  //
    and r1, r1, 0x1C // mask
    or  r0, r0, r1
    or  r0, r0, 0x00 // sleep mode 0x00, NORMAL_MODE 0x03
    write_i2c BME280_ADDR BME280_REG_CTRL_MEAS r0 bme280_fail_config

    store bme280_inited 0x1
   
#endif // MAIN_CORE_INIT_SENSORS

    .global bme280_readData
bme280_readData:

    move r1, i2c_MultyHandler       // set handler for multy
    move r0, processMultyFromStack
    st r0, r1, 0 
    move r1, BME280_ADDR
    push r1
    move r1, BME280_REG_DATA
    push r1 
    move r0, BME280_P_T_H_DATA_LEN        //   BME280_P_T_H_DATA_LEN
    move r2, bme280_data_hum_MSB_LSB  
    psr
    jump readMultyToStack
    add r3,r3,2 // remove call parameters from stack

    ret

    .global bme280_forceRead
bme280_forceRead:
    read_i2c BME280_ADDR BME280_REG_CTRL_MEAS bme280_fail_read
    or r0, r1, 0x01 // set forced mode
   
    move r2, bme280_DEBUG
    st r0, r2, 0
   
    write_i2c BME280_ADDR BME280_REG_CTRL_MEAS r0 bme280_fail_read
    ret
  
bme280_fail_read:
   store bme280_error SENSOR_STATUS_READ_ERROR
   ret
   
