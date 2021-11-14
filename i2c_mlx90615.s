//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "i2c_mlx90615.h"
#include "stack.s"



.bss
   .global mlx90615_error
mlx90615_error:
   .long 0
   .global mlx90615_obj_temperature
mlx90615_obj_temperature:
   .long 0
   .global mlx90615_amb_temperature
mlx90615_amb_temperature:
   .long 0

   .global mlx90615_b1
mlx90615_b1:
   .long 0
   .global mlx90615_b2
mlx90615_b2:
   .long 0
   .global mlx90615_pec
mlx90615_pec:
   .long 0
.text


   .global mlx90615_readdata
mlx90615_readdata:
 //  psr 
  // jump mlx90615_wake
    move r0, mlx90615_error  // test for init error SENSOR_STATUS_HW_ERROR
    ld r0, r0, 0x00
    jumpr  mlx90615_sensor_found, SENSOR_STATUS_HW_ERROR, lt
    jumpr  mlx90615_sensor_found, SENSOR_STATUS_HW_ERROR, gt
    ret
    
mlx90615_sensor_found:
    move r2, 300    // Valid data will be available typically 0.3 seconds after the device has woken up.
    psr
    jump waitMs

    psr
    jump mlx90615_getAmbientTemp

    psr
    jump mlx90615_getObjectTemp

    psr
    jump mlx90615_sleep
ret
   
    .global mlx90615_getObjectTemp
mlx90615_getObjectTemp:
    move r1, i2c_MultyHandler       // set handler for multy
    move r0, processBytesFromStack
    st r0, r1, 0 
   
    move r1,MLX90615_ADDR
    push r1
    move r1,MLX90615_OBJECT_TEMPERATURE
    push r1
    move r0, 3
    move r2, mlx90615_pec  
    psr
    jump readMultyToStack
    add r3,r3,2 // remove call parameters from stack

    move r1, mlx90615_b1  // save result
    ld r0, r1, 0
    move r1, mlx90615_b2  // save result
    ld r2, r1, 0
   
    lsh r2,r2,8
    or r2,r2,r0

    move r0, mlx90615_obj_temperature
    st r2, r0, 0
    ret
   
   .global mlx90615_getAmbientTemp
mlx90615_getAmbientTemp:
    move r1, i2c_MultyHandler       // set handler for multy
    move r0, processBytesFromStack
    st r0, r1, 0 

    move r1,MLX90615_ADDR
    push r1
    move r1,MLX90615_AMBIENT_TEMPERATURE
    push r1
    move r0, 3
    move r2, mlx90615_pec  
    psr
    jump readMultyToStack
    add r3,r3,2 // remove call parameters from stack

    move r1, mlx90615_b1  // save result
    ld r0, r1, 0
    move r1, mlx90615_b2  // save result
    ld r2, r1, 0

    lsh r2,r2,8
    or r2,r2,r0

    move r0, mlx90615_amb_temperature
    st r2, r0, 0
    ret
   
fail_mlx90615:
    store mlx90615_error 1
    ret
   
    .global mlx90615_sleep
mlx90615_sleep:
// SA=0x5B, Command=0xC6, PEC=0x6D (SA_W=0xB6 Command=0xC6 PEC=0x6D)// hardcode for sleep command, adress  fixed
    move r1, MLX90615_ADDR  // SA_W=0xB6
    push r1
    move r1, 0xC6  // Command=0xC6
    push r1
    move r1, 0x6D  // PEC=0x6D
    push r1
    psr
    jump write8
 
    add r3,r3,3 // remove call parameters from stack
    move r0,r2 // test for error in r2
    jumpr fail_mlx90615,1,ge
    ret


    .global mlx90615_wake
mlx90615_wake:
    move r0, mlx90615_error  // test for init error SENSOR_STATUS_HW_ERROR
    ld r0, r0, 0x00
    jumpr  mlx90615_sensor_found_wake, SENSOR_STATUS_HW_ERROR, lt
    jumpr  mlx90615_sensor_found_wake, SENSOR_STATUS_HW_ERROR, gt
    ret
mlx90615_sensor_found_wake:
    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S + SCL_PIN, 1, 1) // SET 0 to scl

    move r2, 55    // wait 10ms to wake up,  Valid data will be available typically 0.3 seconds after the device has woken up.
    psr
    jump waitMs

    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TC_REG, RTC_GPIO_ENABLE_W1TC_S + SCL_PIN, 1, 1) // SET 1 to scl

    ret
    
    /*
 wake_stretch_read: // read 1 from scl -- this is good sign that i2c waked
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + SCL_PIN, 1) 
    jumpr wake_stretch_read,1,lt
  ret
  */
//   https://github.com/joaobarroca93/MLX90615/blob/9f5d3a79b2e3032abb9d0bd01e18334830f1d5dc/MLX90615.cpp
