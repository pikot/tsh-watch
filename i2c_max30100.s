#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.s"

.set MAX30100_ADDR, 0x57

.set EXPECTED_PART_ID, 0x11

.set MAX30100_SAMPRATE_50HZ,  0x00
//.set MAX30100_SAMPRATE_100HZ, 0x01
.set MAX30100_MC_SHDN, 0x80 // 1 << 7

// LED Configuration 
.set MAX30100_LED_CURR_7_6MA, 0x02
.set MAX30100_LED_CURR_27_1MA, 0x08
.set MAX30100_LED_CURR_50MA, 0x0f

.set MAX30100_SPC_PW_200US_13BITS, 0x00
.set MAX30100_SPC_PW_1600US_16BITS, 0x03
.set MAX30100_SPC_SPO2_HI_RES_EN,  0x40

.set MAX30100_MODE_HRONLY, 0x02
.set MAX30100_MODE_SPO2_HR, 0x03

.set MAX30100_FIFO_DEPTH, 0x10             //  link with MAX30100_FIFO_DEPTH_MINUS_ONE (don't want to call sub)
.set MAX30100_FIFO_DEPTH_MINUS_ONE, 0xf    //  link with MAX30100_FIFO_DEPTH (don't want to call sub)

.set MAX30100_REG_FIFO_WRITE_POINTER, 0x02
.set MAX30100_REG_FIFO_OVERFLOW_COUNTER, 0x03
.set MAX30100_REG_FIFO_READ_POINTER,  0x04
.set MAX30100_REG_FIFO_DATA, 0x05
.set MAX30100_REG_MODE_CONFIGURATION, 0x06
.set MAX30100_REG_SPO2_CONFIGURATION, 0x07
.set MAX30100_REG_LED_CONFIGURATION,  0x09
.set MAX30100_REG_PART_ID, 0xff

// https://github.com/oxullo/Arduino-MAX30100/blob/master/src/MAX30100.cpp
//https://github.com/viggi1000/RakshBluemix/blob/3b64cc5819357a9f7e7492c5afc7fd98b4f5671e/max30100_photon/firmware/MAX30100.cpp

.set ARRAY_SIZE, 10 // in uint32 or = 800 byte, same set in max30100_raw_data size

.bss   
   .global max30100_error
max30100_error:
   .long 0
   .global max30100_toRead
max30100_toRead:
   .long 0
   .global max30100_r2
max30100_r2:
   .long 0
   .global max30100_r3
max30100_r3:
   .long 0
   .global max30100_flags
max30100_flags:
   .long 0
   .global max30100_data_pointer // 
max30100_data_pointer:
   .long 0
   .global max30100_raw_data
max30100_raw_data:
   .skip 5// need set 800
   .global max30100_raw_dataEnd
max30100_raw_dataEnd:
   .long 0

//  DUMMY, only for switch off sensor if it soldered
.text
    .global max30100_init
max30100_init:
    ret                     // tmp off


   /* 
    .global max30100_resume
max30100_resume:
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read 0x7f 0
    ret
*/
    .global max30100_shutdown
max30100_shutdown:
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read 0x7f MAX30100_MC_SHDN
    ret
    
fail_max30100_read:
   store max30100_error SENSOR_STATUS_HW_ERROR
   ret 
    
 
