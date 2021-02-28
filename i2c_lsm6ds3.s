#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.s"

.set LSM6DS3_ADDR, 0x6A

.set LSM6DS3_ACC_GYRO_WHO_AM_I_REG, 0X0F

.set LSM6DS3_ACC_GYRO_WHO_AM_I, 0X69
.set LSM6DS3_C_ACC_GYRO_WHO_AM_I, 0x6A

// * Register      : CTRL1_XL,  * Address       : 0X10
.set LSM6DS3_ACC_GYRO_FS_XL_2g, 0x00
.set LSM6DS3_ACC_GYRO_FS_XL_4g, 0x08

.set LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN, 0x00
//.set LSM6DS3_ACC_GYRO_ODR_XL_12Hz, 0x10
.set LSM6DS3_ACC_GYRO_ODR_XL_26Hz, 0x20
//.set LSM6DS3_ACC_GYRO_ODR_G_12Hz, 0x10
//.set LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED, 0x02

// * Register      : CTRL2_G,  * Address       : 0X11
.set LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN, 0x00

.set LSM6DS3_ACC_GYRO_XL_HM_MODE,  0x10
// * Register      : CTRL7_G,  * Address       : 0X16  
.set LSM6DS3_ACC_GYRO_G_HM_MODE,   0x80     

/************** Device Register  *******************/
.set LSM6DS3_ACC_GYRO_INT1_CTRL, 0X0D
.set LSM6DS3_ACC_GYRO_CTRL1_XL, 0X10
.set LSM6DS3_ACC_GYRO_CTRL2_G,  0X11
.set LSM6DS3_ACC_GYRO_CTRL6_C,  0X15
.set LSM6DS3_ACC_GYRO_CTRL7_G,  0X16

.set LSM6DS3_ACC_GYRO_CTRL10_C, 0X19
.set LSM6DS3_ACC_GYRO_TAP_CFG1, 0X58
.set LSM6DS3_ACC_GYRO_STEP_COUNTER_L,0X4B
.set LSM6DS3_ACC_GYRO_STEP_COUNTER_H,0X4C
/********/
.set IMU_HW_ERROR,  0X1
.set IMU_CFG_ERROR, 0X2
.set IMU_PED_ERROR, 0X4
.set IMU_LOWPOWER_ERROR, 0X8
.set IMU_BAD_RET_ERROR, 0X16


.bss   
   .global lsm6ds3_error
lsm6ds3_error:
   .long 0
   
   .global lsm6ds3_error_cnt
lsm6ds3_error_cnt:
   .long 0
   
   .global lsm6ds3_driver_sign
lsm6ds3_driver_sign:
   .long 0
   
//64K steps ought to be enough for anyone =)
   .global lsm6ds3_step_count
lsm6ds3_step_count:
   .long 0
   
   .global lsm6ds3_inited
lsm6ds3_inited:
   .long 0

   .global lsm6ds3_reseted_steps
lsm6ds3_reseted_steps:
   .long 0
  
.text
   .global lsm6ds3_config_pedometer
lsm6ds3_config_pedometer:
   store lsm6ds3_error 0

   if_val_eq ds3231_hour 0 skip_set_step //ammm, try to reset counter on new day logic
   store lsm6ds3_reseted_steps 0x00              //yep, each time reset this value if not 1th hour of day, not beautiful but reliable
skip_set_step:
   store lsm6ds3_driver_sign 0
   read_i2c LSM6DS3_ADDR LSM6DS3_ACC_GYRO_WHO_AM_I_REG fail_lsm6ds3
   move r0, lsm6ds3_driver_sign
   st r1, r0, 0
   
   move r0, r1 // test for LSM6DS3_ACC_GYRO_WHO_AM_I
   jumpr ok_lsm6ds3,LSM6DS3_ACC_GYRO_WHO_AM_I,eq
   jumpr ok_lsm6ds3,LSM6DS3_C_ACC_GYRO_WHO_AM_I,eq
   store lsm6ds3_error IMU_BAD_RET_ERROR
   ret
fail_lsm6ds3:
   store lsm6ds3_error IMU_HW_ERROR
   inc lsm6ds3_error_cnt
   ret
ok_lsm6ds3:
   move r2, ds3231_hour        // some logic for reset counter (can't use if_val_eq 
   ld r0, r2, 0x00             // 
   jumpr skip_reset_steps, 0, gt 
    
   move r2, lsm6ds3_reseted_steps
   ld r0, r2, 0x00
   jumpr skip_reset_steps, 0x01, eq
   move r0, 0x01
   st r0, r2, 0x00
   psr
   jump drop_step_count
     
skip_reset_steps:

   if_val_noteq lsm6ds3_inited 0x1 set_lsm6ds3_lowpower
   jump lsm6ds3_read_step_counter  // ugly but otherwise "error: relocation out of range" happened
set_lsm6ds3_lowpower:
   // set low-power mode
   
   update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL6_C fail_lsm6ds3_lowpower 0xef LSM6DS3_ACC_GYRO_XL_HM_MODE
   update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL7_G fail_lsm6ds3_lowpower 0x7f LSM6DS3_ACC_GYRO_G_HM_MODE

   jump ok_lsm6ds3_lowpower
fail_lsm6ds3_lowpower:
   store lsm6ds3_error IMU_LOWPOWER_ERROR
   ret 
ok_lsm6ds3_lowpower:

   //update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL1_XL fail_lsm6ds3_config 0x0f 0x20
 // update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL2_G fail_lsm6ds3_config 0x0f 0x00
 //  ret
   // Step 1: Configure ODR-26Hz and FS-2g
   move r1,LSM6DS3_ADDR
   push r1
   move r1,LSM6DS3_ACC_GYRO_CTRL1_XL
   push r1
   move r1, 0
   or r1,r1,LSM6DS3_ACC_GYRO_FS_XL_2g
   or r1,r1,LSM6DS3_ACC_GYRO_ODR_XL_26Hz
   push r1
   psr
   jump write8   
   add r3, r3, 3 // remove call parameters from stack
   move r0, r2 // test for error in r2
   jumpr fail_lsm6ds3_config,1,ge

 //  // test to look at power consumption
   // write_i2c LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL2_G LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN fail_lsm6ds3_config
 
    
   // Step 2: Set bit Zen_G, Yen_G, Xen_G, FUNC_EN, PEDO_RST_STEP(1 or 0)
   move r1, LSM6DS3_ADDR
   push r1
   move r1, LSM6DS3_ACC_GYRO_CTRL10_C
   push r1
   move r1, 0x3C    // Read current
   push r1
   psr
   jump write8
   add r3,r3,3 // remove call parameters from stack
   move r0, r2 // test for error in r2
   jumpr fail_lsm6ds3_config,1,ge
  
   // Step 3:  Enable pedometer algorithm
   move r1,LSM6DS3_ADDR
   push r1
   move r1,LSM6DS3_ACC_GYRO_TAP_CFG1
   push r1
   move r1,0x40
   push r1
   psr
   jump write8   
   add r3,r3,3 // remove call parameters from stack
   move r0,r2 // test for error in r2 
   jumpr fail_lsm6ds3_config,1,gt
 
    //Step 4: Step Detector interrupt driven to INT1 pin, set bit INT1_FIFO_OVR
   move r1,LSM6DS3_ADDR
   push r1
   move r1,LSM6DS3_ACC_GYRO_INT1_CTRL
   push r1
   move r1,0x00
   push r1
   psr
   jump write8  
   add r3,r3,3 // remove call parameters from stack
   move r0,r2 // test for error in r2
   jumpr fail_lsm6ds3_config,1,ge
   store lsm6ds3_inited 0x1
   
   jump lsm6ds3_read_step_counter

fail_lsm6ds3_config:
   store lsm6ds3_error IMU_CFG_ERROR
   ret
          
   .global  lsm6ds3_read_step_counter
lsm6ds3_read_step_counter:
   move r1,LSM6DS3_ADDR
   push r1
   move r1,LSM6DS3_ACC_GYRO_STEP_COUNTER_L  // i dont wan't to read twice registers via i2c, let it be bit magic instead
   push r1
   psr
   jump read16
   add r3,r3,2 // remove call parameters from stack 
   
   move r1,r0 // save result
   move r0,r2 // test for error
   jumpr fail_lsm6ds3_read_counter,1,ge   
   move r0, r1

   and r1,r1,0XFF00 // bit swap
   rsh r1,r1,8
   lsh r0,r0,8
   or r0,r0,r1
   
   move r1, lsm6ds3_step_count
   st r0, r1, 0               // Save counter in memory 
   ret


fail_lsm6ds3_read_counter:
   store lsm6ds3_error IMU_PED_ERROR
   ret 

   .global lsm6ds3_poweroff
lsm6ds3_poweroff:
   update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL1_XL fail_lsm6ds3_config 0x0f 0x00
   update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL2_G  fail_lsm6ds3_config 0x0f 0x00
ret

   .global drop_step_count 
drop_step_count:
   write_i2c LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL10_C 0x3E fail_lsm6ds3_config  
ret
   // PEDO_THS_REG  -- Pedometer minimum threshold and internal full-scale configuration register (r/w).

   /* 
- only accelerometer active and gyroscope in power-down
- only gyroscope active and accelerometer in power-down
- both accelerometer and gyroscope sensors active with independent ODR 



*/

    
   
