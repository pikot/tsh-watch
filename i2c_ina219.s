//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "i2c_ina219.h"
#include "stack.s"

.bss   
   .global ina219_error
ina219_error:
   .long 0
   
   .global ina219_sleepstatus
ina219_sleepstatus:
   .long 0
   
   .global ina219_config
ina219_config:
   .long 0

   .global ina219_aggr_current
ina219_aggr_current:
   .long 0

   .global ina219_current
ina219_current:
   .long 0
  
   .global ina219_voltage
ina219_voltage:
   .long 0
   
   .global ina219_calValue
ina219_calValue:
   .long 0
   
   .global ina219_currentDivider_mA
ina219_currentDivider_mA:
   .long 0

   .global ina219_current_table
ina219_current_table:
   .long 0
  // .skip 240, 0
   // 60 * 4(bites)

   .global ina219_current_pointer
ina219_current_pointer:
   .long 0
   
.text
  .global ina219_readdata
ina219_readdata:

    move r0, ina219_error  // test for init error SENSOR_STATUS_HW_ERROR
    ld r0, r0, 0x00
    jumpr  ina219_sensor_found, SENSOR_STATUS_HW_ERROR, lt
    jumpr  ina219_sensor_found, SENSOR_STATUS_HW_ERROR, gt
    ret
ina219_sensor_found:

#ifdef MAIN_CORE_INIT_SENSORS
   psr
   jump powerUp   //  wake up on init state
#else
   psr
   jump Start_INA219   //  wake up on init state
#endif

   psr 
   jump getBusVoltage_raw
   psr
   jump getCurrent_raw
   psr 
   jump ina219_powerDown     


  /*
   move r2, ina219_current_pointer
   ld r0, r2, 0
   jumpr plus_pointer, 59, lt
   move r0, 0x0
   jump pointer_magic
plus_pointer:
   add r0, r0, 1
pointer_magic:
   move r2, ina219_current_pointer
   st r0, r2, 0
   
   move r2, ina219_current_table
   add r1, r2, r0 // move pointer to num from  ina219_current_poiter
*/
   move r2, ina219_current    // Read current
   ld r0, r2, 0
  // st r0, r1, 0                // Save counter in minute table
   
   move r2, ina219_aggr_current    // Read  counter 
   ld r1, r2, 0
   add r0, r0, r1              // Increment 
   st r0, r2, 0                // Save counter in memory
   
   ret


#ifndef MAIN_CORE_INIT_SENSORS

   .global Start_INA219
Start_INA219:
  // store ina219_error 0
///setCalibration_32V_2A_INA219:
   store ina219_calValue 4096
   store ina219_currentDivider_mA 10
   psr
   jump setCalibration
   
  // move r2, 70                  // Wait 
  // psr
  // jump waitMs
   move r1, INA219_ADDR
   push r1
   move r1, INA219_REG_CONFIG
   push r1
   /*
   move r1, 0
   or r1,r1,INA219_CONFIG_BVOLTAGERANGE_32V
   or r1,r1,INA219_CONFIG_GAIN_8_320MV
   or r1,r1,INA219_CONFIG_BADCRES_12BIT
   or r1,r1,INA219_CONFIG_SADCRES_12BIT_1S_532US
   or r1,r1,INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS
   */
   move r1, 0x399F // BVOLTAGERANGE_32V | GAIN_8_320MV | BADCRES_12BIT | SADCRES_12BIT_1S_532US | MODE_SANDBVOLT_CONTINUOUS
 //  move r0, ina219_config
   //st r1, r0, 0x0
  // ld r1,ina219_config,0x0 
   push r1
   psr
   jump write16   
   add r3,r3,3 // remove call parameters from stack
   move r0,r2 // test for error in r2
   jumpr fail_config,1,ge
   
 //  psr
   //jump read_config
   ret
#endif

  .global setCalibration
setCalibration:
   move r1,INA219_ADDR   
   push r1
   move r1,INA219_REG_CALIBRATION
   push r1
   move r2, ina219_calValue   
   ld  r1, r2, 0
   push r1 
   psr
   jump write16
   add r3, r3, 3 // remove call parameters from stack
   move r0, r2 // test for error in r2
   jumpr fail_config, 1, ge
   ret
   
fail_config:
   store ina219_error SENSOR_STATUS_CFG_ERROR
   ret

   .global ina219_powerDown
ina219_powerDown:    
   psr
   jump read_config
   
   move r1,INA219_ADDR
   push r1
   move r1,INA219_REG_CONFIG
   push r1
   move r2,ina219_config   
   ld  r1,r2,0
   and r1,r1,INA219_CONFIG_MODE_POWERDOWN_MASK
   push r1
   psr  
   jump write16
   add r3,r3,3 // remove call parameters from stack
   
   move r0,r2 // test for error in r2
   jumpr fail_config,1,ge
   ret

   .global powerUp
powerUp: 
   psr
   jump read_config
   
   move r1, INA219_ADDR
   push r1
   move r1, INA219_REG_CONFIG
   push r1
   move r2, ina219_config   
   ld  r1, r2, 0
   or r1, r1, INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS
   push r1
   psr
   jump write16
   add r3, r3, 3 // remove call parameters from stack
   move r0, r2 // test for error in r2
   jumpr fail_config, 1, ge
   
 //  store ina219_error SENSOR_WSTATUS_AWAKE
   ret

   .global read_config
read_config: 
   move r1, INA219_ADDR
   push r1
   move r1, INA219_REG_CONFIG
   push r1
   psr
   jump read16
   add r3, r3, 2 // remove call parameters from stack
   move r1, r0 // save result
   move r0, r2 // test for error
   jumpr fail_read, 1, ge

   move r2, ina219_config // store result
   st r1, r2, 0
   ret
  
   .global getBusVoltage_raw
getBusVoltage_raw:
   move r1, INA219_ADDR
   push r1
   move r1, INA219_REG_BUSVOLTAGE
   push r1
   psr
   jump read16
   add r3, r3, 2 // remove call parameters from stack
   
   move r1, r0 // save result
   move r0, r2 // test for error
   jumpr fail_read, 1, ge
  
   rsh r2, r1, 0x03  
   move r1, ina219_voltage // store result
   st r2,r1,0
   ret

  .global getCurrent_raw
getCurrent_raw:
/*
   psr 
   jump setCalibration
   move r2, 70                  // Wait 
   psr
   jump waitMs
  */ 
   move r1, INA219_ADDR
   push r1
   move r1, INA219_REG_CURRENT
   push r1
   psr
   jump read16
   add r3, r3, 2 // remove call parameters from stack
  
   move r1, r0 // save result
   
   move r0, r2 // test for error
   jumpr fail_read, 1, ge
   
   move r2, ina219_current// store result
   st r1,r2,0
   ret

   .global fail_read
fail_read:
   store ina219_error SENSOR_STATUS_READ_ERROR
   ret
   
/*
   .global getShuntVoltage_raw
getShuntVoltage_raw:
   move r1,INA219_ADDR
   push r1
   move r1,INA219_REG_SHUNTVOLTAGE
   push r1
   psr
   jump read16
   add r3,r3,2 // remove call parameters from stack
   move r1,r0 // save result
   move r0,r2 // test for error
   jumpr fail_rc,1,ge
   move r2,ina219_shuntvoltage// store result
   st r1,r2,0
   ret
   */




   
