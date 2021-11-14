//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "i2c_max30100.h"
#include "stack.s"


// https://github.com/oxullo/Arduino-MAX30100/blob/master/src/MAX30100.cpp
//https://github.com/viggi1000/RakshBluemix/blob/3b64cc5819357a9f7e7492c5afc7fd98b4f5671e/max30100_photon/firmware/MAX30100.cpp

//.set ARRAY_SIZE, 100 // samples, set 4*ARRAY_SIZE  in max30100_raw_data size to count bytes

.bss   
   .global max30100_error
max30100_error:
   .long 0

   .global max30100_fifo_filled
max30100_fifo_filled:
   .long 0
   
   .global max30100_loop_counter
max30100_loop_counter:
   .long 0
   
   .global max30100_get_all_fifo_counter
max30100_get_all_fifo_counter:
   .long 0

    .global max30100_toRead
max30100_toRead:
   .long 0
   
    .global max30100_data_size
max30100_data_size:
   .long 0
   
    .global max30100_free_data_size
max30100_free_data_size:
   .long 0
   
    .global max30100_data_size_after
max30100_data_size_after:
   .long 0
   
   .global max30100_fifo_num_available_samples
max30100_fifo_num_available_samples:
   .long 0
   .global max30100_fifo_overflow_counter
max30100_fifo_overflow_counter:
   .long 0
   .global max30100_flags
max30100_flags:
   .long 0


   .global max30100_data_pointer // 
max30100_data_pointer:
   .long 0
   
   .global max30100_raw_data
max30100_raw_data:
   //.skip 8
   .skip 500 // in bytes! // 500 = 125 samples // i need 25 samples per second, 1 sample 4 bite (even if i need only RED data, second int16 unused due to ulp memory architecture)
   .global max30100_raw_dataEnd
max30100_raw_dataEnd:
   .long 0

.text
    .global max30100_buffer_is_full
max30100_buffer_is_full:
    ret
    
    .global max30100_init
max30100_init:
    move r0, max30100_error  // test for init error SENSOR_STATUS_HW_ERROR
    ld r0, r0, 0x00
    jumpr  max30100_sensor_found, SENSOR_STATUS_HW_ERROR, lt
    jumpr  max30100_sensor_found, SENSOR_STATUS_HW_ERROR, gt
    ret
max30100_sensor_found:

//    store max30100_readFifoData 0
  ///  psr                     // tmp off
//    jump max30100_shutdown  // tmp off
  //  ret                     // tmp off

#ifndef MAIN_CORE_INIT_SENSORS
    read_i2c MAX30100_ADDR MAX30100_REG_PART_ID fail_max30100_init
    move r0, r1  // take result from r1
    jumpr max30100_find_ok, MAX30100_EXPECTED_PART_ID, eq
    
    .global fail_max30100_init
fail_max30100_init:
    store max30100_error 1
    ret
      
    .global max30100_find_ok
max30100_find_ok:

    move r1, max30100_flags
    ld r0, r1, 0
    move r2, r0     
    and r0, r0, 0x1               // sensor is inited
    jumpr reset_sensors, 0x0, eq
    and r0, r2, 0x2               // buffer is full  
    jumpr max30100_buffer_is_full, 0x2, eq

    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read_1 0x7f 0 // resume sensor

    jump max30100_readFifoData
    
reset_sensors:
// 0x43 = MAX30100_MODE_SPO2_HR | Bit 6: Reset Control (RESET)
    write_i2c MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION 0x40 fail_max30100_init // reset
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_init 0x7f MAX30100_MC_SHDN

    /*
    read_i2c MAX30100_ADDR MAX30100_REG_SPO2_CONFIGURATION fail_max30100_init
    and r0, r1, 0xfc // take result from r1
    or r0, r0, MAX30100_SPC_PW_1600US_16BITS
    and r0, r0, 0xe3 // 
    move r1, MAX30100_SAMPRATE_50HZ
    lsh r1, r1, 2
    or r0, r0, r1
    or r0, r0, MAX30100_SPC_SPO2_HI_RES_EN 
    */
    // MAX30100_SPC_PW_200US_13BITS
    move r0, 0x43 // = MAX30100_SPC_PW_1600US_16BITS (0x03) & MAX30100_SAMPRATE_50HZ (0x0 << 2) & MAX30100_SPC_SPO2_HI_RES_EN (0x40)
    write_i2c MAX30100_ADDR MAX30100_REG_SPO2_CONFIGURATION r0 fail_max30100_init
/*
    move r1,  MAX30100_LED_CURR_50MA // redLed
    lsh r1, r1, 4
    or r0, r1, MAX30100_LED_CURR_50MA // irLed MAX30100_LED_CURR_27_1MA
  */
    move r0, 0xff // = MAX30100_LED_CURR_50MA(0xf) & MAX30100_LED_CURR_50MA (0xf)
    write_i2c MAX30100_ADDR MAX30100_REG_LED_CONFIGURATION r0 fail_max30100_init
    write_i2c MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION MAX30100_MODE_HRONLY fail_max30100_read_1

    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_WRITE_POINTER 0x0 fail_max30100_read_1
    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_OVERFLOW_COUNTER 0x0 fail_max30100_read_1
    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_READ_POINTER 0x0 fail_max30100_read_1
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read_1 0x7f 0
    set_flag max30100_flags 1 // sensor is inited #1, data is full #2
    
    move r2, 50    // wait some time 
    psr
    jump waitMs
#else
    move r1, max30100_flags
    ld r0, r1, 0
    //move r2, r0     
   // and r0, r0, 0x1               // sensor is inited
   // jumpr reset_sensors, 0x0, eq
    and r0, r0, 0x2               // buffer is full  
    jumpr max30100_buffer_is_full, 0x2, eq
    
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read_1 0x7f 0 // resume sensor

#endif
//   jump buffer_is_full
    jump max30100_readFifoData  // test
    
    .global fail_max30100_read_1
fail_max30100_read_1:
    store max30100_error 2
    ret
    
    .global buffer_is_full
buffer_is_full:
    psr 
    jump max30100_shutdown

    move r1, max30100_flags    
    ld r0, r1, 0
    or r0, r0, 0x2 // data is full
    st r0, r1, 0
    ret

    .global max30100_readFifoData
max30100_readFifoData:
    store max30100_loop_counter 0 
    store max30100_get_all_fifo_counter 0 

    move r2, max30100_raw_data  // i need to use some iteration!
    move r1, max30100_data_pointer
    st r2, r1, 0 
  
   .global max30100_readFifoData_loop
max30100_readFifoData_loop:
    inc max30100_loop_counter
/*
    move r2, max30100_data_pointer // cnt word in r2 // need test for error!
    ld r2, r2, 0
    move r1, max30100_raw_data
    sub r0, r2, r1

    jumpr buffer_is_full, MAX30100_ARRAY_SIZE, ge
 //   move r1, max30100_error
   // st  r0, r1, 0
*/
    read_i2c MAX30100_ADDR MAX30100_REG_FIFO_OVERFLOW_COUNTER max30100_readFifoData
    move r2, max30100_fifo_overflow_counter 
    st r1, r2, 0 
    move r0, r1
    jumpr fulled_fifo, 0, gt

    read_i2c MAX30100_ADDR MAX30100_REG_FIFO_WRITE_POINTER max30100_readFifoData
    move r0, max30100_toRead
    st r1, r0, 0

    read_i2c MAX30100_ADDR MAX30100_REG_FIFO_READ_POINTER max30100_readFifoData

    move r0, max30100_toRead
    ld r0, r0, 0

// or do --         samp = ((32+wptr)-rptr)%32;    /// here 16 instead 32??

    sub r2, r0, r1            // = FIFO_WR_PTR – FIFO_RD_PTR
    psr
    jump abs_value
    
    and r0, r2, 0xfffe        // I read only even number samples (due to shrink policy I want to balanced data, each 2d sample)
    jumpr max30100_readFifoData_loop, 0, eq    //

    move r1, max30100_fifo_num_available_samples
    st r0, r1, 0

    jump get_all_fifo
  //   ret
  
fulled_fifo:
     inc  max30100_fifo_filled
     move r0, 2  // get 2 samples at least
     move r1, max30100_fifo_num_available_samples
     st r0, r1, 0
    
get_all_fifo:
    move r2, max30100_data_pointer // cnt word in r2 // need test for error!
    ld r2, r2, 0
    move r1, max30100_raw_data
    sub r1, r2, r1        // r1 <- datasize that placed in RTC memory from sensor 
    
    move r2, max30100_data_size
    st r1, r2, 0
    
    move r2, MAX30100_ARRAY_SIZE  
    sub  r0, r2, r1      // r0 <- free memory left in buffer
    move  r2, max30100_free_data_size // store for debug
    st r0, r2, 0  
    jumpr buffer_is_full, 0, eq

    move r2, max30100_fifo_num_available_samples
    ld r1, r2, 0
    // use line belove in case 25hz readout
   // rsh r1, r1, 1 // = (fifo_num_available_samples / 2) ; num samples wich will be placed to memory (due to shrink policy I want to balanced data, each 2d sample) 

    move r2, r0   // move to r2 <- free memory left (I use it later)

    sub r0, r0, r1 // free memory left  -  available_samples wich i read 
    jumpr  skip_shrink, MAX30100_ARRAY_SIZE, le   // enough space/////// shoul be less than fifo size,  lt 0 not work (it thinks that value unsigned) 

    move r0, r2 // read all free memory left    
    // use line belove in case 25hz readout
    // lsh r0, r2, 1 // take free memory left *2  - I guarantee that  max30100_fifo_num_available_samples more than (free memory left) * 2
    jump skip_shrink_end
skip_shrink:
    move  r1, max30100_fifo_num_available_samples 
    ld  r0, r1, 0  // take all device buffer
skip_shrink_end:
    lsh r0, r0, 2             // i convert sample to bytes via  * 4 
    
    move r1, i2c_MultyHandler       // set handler for multy
    move r2, process16Bit50HzFifo30100FromStack
    st r2, r1, 0 
    move r1, MAX30100_ADDR
    push r1
    move r1, MAX30100_REG_FIFO_DATA
    push r1 
   // move r0,     XXX  // len in r0 allready     
    move r2, max30100_data_pointer  
    psr
    jump readMultyToStack
    add r3,r3,2 // remove call parameters from stack


    move r2, max30100_data_pointer // 
    ld r2, r2, 0
    move r1, max30100_raw_data
    sub r0, r2, r1
    move r1, max30100_data_size_after
    st  r0, r1, 0
    
    jumpr  houston_we_have_a_problem , MAX30100_ARRAY_SIZE, gt
    
    move r2, 160    // wait some time 
    psr
    jump waitMs
    
    jump  max30100_readFifoData_loop
    
houston_we_have_a_problem:
    move r1, max30100_flags    
    ld r0, r1, 0
    or r0, r0, 0x16 // problem after read, to big data size
    st r0, r1, 0
    jump buffer_is_full


    .global fail_max30100_read
fail_max30100_read:
    store max30100_error 2
    ret
   
    .global max30100_resume
max30100_resume:
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read 0x7f 0
    ret

    .global max30100_shutdown
max30100_shutdown:
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read 0x7f MAX30100_MC_SHDN
  
   // read_i2c MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read // test
   // move r0, max30100_error                                                   //
   // st r1, r0, 0                                                              //
    
    ret

    
      //  read_i2c MAX30100_ADDR MAX30100_REG_LED_CONFIGURATION fail_max30100_init // test
  //  move r0, max30100_error
  //  st r1, r0, 0 
    //ret ///oooooo
    //move r1, max30100_error 
    //st r0, r1, 0


    .global abs_value
abs_value:
    and  r0, r2, 0x8000  //hell yeeee
    jumpr positive_value, 0x0000, eq
    move r0, r2
    psr
    jump not_val
    
    add  r2, r2, 1 
positive_value:
    ret

    .global not_val 
not_val: // yes here we don't have even not bit operation
    // in r0  argument
    move r1, r0
    move r2, 0x00 // here is result
    stage_rst

not_next_bit:
    lsh r2, r2, 1
    and r0, r1, 0x8000
    jumpr not_val_skip, 0x0000, gt
    or r2, r2, 0x01
not_val_skip:
    lsh r1, r1, 1
    
    stage_inc 1
    jumps not_next_bit, 16, lt
    ret 
 
 
