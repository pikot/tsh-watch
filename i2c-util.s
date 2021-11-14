/*
 * I2C ULP utility routines
 */

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

#include "stack.s"


.bss
  .global cnt_read
cnt_read:
  .long 0
  
  .global icounter
icounter:
  .long 0

  .global resul_pointer
resul_pointer:
  .long 0
  
odd_sign:
   .long 0
   
.text

write_intro:
	psr
	jump i2c_start_cond

	ld r2,r3,20 // Address
	lsh r2,r2,1
	psr
	jump i2c_write_byte
	jumpr popfail,1,ge

	ld r2,r3,16 // Register
	psr
	jump i2c_write_byte
	jumpr popfail,1,ge
	ret


.global write8
write8:
	psr
	jump write_intro

write_b:
	ld r2,r3,8 // data byte
	psr
	jump i2c_write_byte
	jumpr fail,1,ge

	psr
	jump i2c_stop_cond

	move r2, 0 // Ok
	ret


.global write16
write16:
	psr
	jump write_intro

	ld r2,r3,8 // data byte 1
	rsh r2,r2,8
	psr
	jump i2c_write_byte
	jumpr fail,1,ge

	jump write_b


read_intro:
	psr
	jump i2c_start_cond

	ld r2,r3,16 // Address
	lsh r2,r2,1
	psr
	jump i2c_write_byte
	jumpr popfail,1,ge
  
	ld r2,r3,12 // Register
	psr
	jump i2c_write_byte
	jumpr popfail,1,ge

	psr
	jump i2c_start_cond

	ld r2,r3,16
	lsh r2,r2,1
	or r2,r2,1 // Address Read
	psr
	jump i2c_write_byte
	jumpr popfail,1,ge

	ret
popfail:
	pop r1 // pop caller return address
	move r2,1
	ret

.global read8
read8:
	psr
	jump read_intro

	move r2,1 // last byte
	psr
	jump i2c_read_byte
	push r0

	psr
	jump i2c_stop_cond

	pop r0

	move r2,0 // OK
	ret
fail:
	move r2,1
	ret

.global read16
read16:
	psr
	jump read_intro

	move r2,0
	psr
	jump i2c_read_byte
	push r0

	move r2,1 // last byte
	psr
	jump i2c_read_byte
	push r0

	psr
	jump i2c_stop_cond

	pop r0
	pop r2 // first byte
	lsh r2,r2,8
	or r2,r2,r0
	move r0,r2

	move r2,0 // OK
	 ret

  /*  
    .global readMULTY // 
readMULTY:
     ret
*/

    .global i2c_MultyHandler
i2c_MultyHandler:
    .long 0

  
i2c_fail:
    move r2,1
    ret

  
   // - why i use stack here and do not write directly in memory??
   // - i want to write  common code for reading different sensor, as short as possible, and for some sensors i want to use filter of date that i will be skipped 
   // (filtering stack as i think is much better understandable than write on fly)
   
  .global readMultyToStack // 
readMultyToStack:
    move r1, cnt_read
    st r0, r1, 0
    move r1, resul_pointer
    st r2, r1, 0
   
    store odd_sign 0
   
    psr
    jump read_intro
  
    store_mem cnt_read icounter 
   
next_byteN:
    dec icounter                // icounter -> r0
 //  push r0

    jumpr last_byte, 0, eq
    move r2, 0
    jump end_byte_select
last_byte:
    move r2, 1 // last byte
end_byte_select:

   psr
   jump i2c_read_byte
   push r0
   
   inc odd_sign         // compress data logic
   and r0, r0, 0x1      //
   jumpr  odd_bit_rmulty, 0x1, eq  

   pop r0
   pop r1
   
   lsh r1,r1,8  // compress data to uint16
   or r1,r1,r0

   push r1

odd_bit_rmulty: 
    move r2, icounter
    ld r0, r2, 0
    jumpr next_byteN, 0, gt

    psr
    jump i2c_stop_cond 
    // in stack now packed data in next func i should read data to memory (skip some data for example)

    move r1, i2c_MultyHandler
    ld r2, r1, 0x0
    jump r2

   
    .global i2cMultyHandlerEnd
i2cMultyHandlerEnd:
     ret

   
    .global processMultyFromStack // 
processMultyFromStack:
   // ahtung! should go strongly  after readMultyToStack,  cnt_read -- global value that set in that func once
   // and in stack shoul be cnt_read packed bytes

    move r2, cnt_read
   
    ld r0, r2, 0
    and r0, r0, 0x1      // detect odd or not odd
   
    ld r1, r2, 0
    rsh r1, r1, 1        //  = r1 / 2
  
    add r0, r0, r1       // cnt word in memory, in r0 iterater, do not use r0 below!

    move r2, resul_pointer  
    ld r1, r2, 0

next_elem:
    pop r2
    st  r2, r1, 0               // Save data in memory 
    sub r1, r1, 1              // move pointer to next memory cell
    sub r0, r0, 1
    jumpr next_elem, 0, gt    

    move r2, 0
   
    jump i2cMultyHandlerEnd
   // -- ret // this is end of handler
   

    .global processBytesFromStack // 
processBytesFromStack:
   // ahtung! should go strongly  after readMultyToStack,  cnt_read -- global value that set in that func once
   // and in stack shoul be cnt_read packed bytes

    st r2, r1, 0 //
    move r2, resul_pointer  
    ld r1, r2, 0

    move r2, cnt_read
    ld r0, r2, 0
    move r2, icounter
    st r0, r2, 0 //
    and r0, r0, 0x1      // detect odd or not odd
    jumpr next_bytes_elem, 0, eq
    
    pop r2
    st  r2, r1, 0               // Save data in memory 
    sub r1, r1, 1               // move pointer to next memory cell
    dec icounter                // icounter -> r0
    jumpr it_was_one_byte, 0, eq
next_bytes_elem:

    pop r2
    move r0, r2

    and r0, r0, 0xff
    st  r0, r1, 0               // Save data in memory 
    sub r1, r1, 1              // move pointer to next memory cell
    rsh r2, r2, 8
    st  r2, r1, 0               // Save data in memory 
    sub r1, r1, 1              // move pointer to next memory cell
    dec icounter                // icounter -> r0
    dec icounter

    
    jumpr next_bytes_elem, 0, gt    
it_was_one_byte:
    move r2, 0
    jump i2cMultyHandlerEnd
   // -- ret // this is end of handler

/*   
  .global process16Bit25HzFifo30100FromStack // 
process16Bit25HzFifo30100FromStack:
   // ahtung! should go strongly  after readMultyToStack,  cnt_read -- global value that set in that func once
   // and in stack shoul be cnt_read packed bytes
    move r1, cnt_read  // icounter = cnt_read / 2 ;// i have  2 * uint8 in one uint16 (32)
    ld r2, r1, 0
    rsh r2, r2, 1
    move r1, icounter
    st r2, r1, 0 //
   
    store odd_sign 0

    move r2, resul_pointer  
    ld r1, r2, 0
    ld r1, r1, 0 // r1 -- shouldn't be not used from here !!!!

next_elemFifo30100:
    inc odd_sign         // compress data logic // without r1!
    and r0, r0, 0x3
    jumpr  Fifo30100_IR_1_byte, 0x02, eq  // store only 2st uint16 (ir data) skip

    pop r0
    jump Fifo30100_skip_store

Fifo30100_IR_1_byte:
    pop r0
   
    st r0, r1, 0               // Save data in memory
    add r1, r1, 1              // move pointer to next
   
Fifo30100_skip_store:
    dec icounter                // icounter -> r0
    jumpr next_elemFifo30100, 0, gt    

    move r2, resul_pointer  
    ld r0, r2, 0
    st r1, r0, 0   /// store result from r1 !!!!!!
    jump i2cMultyHandlerEnd
    // -- ret  // this is end of handler
*/

  .global process16Bit50HzFifo30100FromStack // 
process16Bit50HzFifo30100FromStack:
   // ahtung! should go strongly  after readMultyToStack,  cnt_read -- global value that set in that func once
   // and in stack shoul be cnt_read packed bytes
    move r1, cnt_read  // icounter = cnt_read / 2 ;// i have  2 * uint8 in one uint16 (32)
    ld r2, r1, 0
    rsh r2, r2, 1
    move r1, icounter
    st r2, r1, 0 //
   
    store odd_sign 0

    move r2, resul_pointer  
    ld r1, r2, 0
    ld r1, r1, 0 // r1 -- shouldn't be not used from here !!!!

next_elemFifo30100:
    inc odd_sign         // compress data logic // without r1!
    and r0, r0, 0x1
    jumpr  Fifo30100_IR_1_byte, 0x00, eq  // store only 2st uint16 (ir data) skip

    pop r0
    jump Fifo30100_skip_store

Fifo30100_IR_1_byte:
    pop r0
   
    st r0, r1, 0               // Save data in memory
    add r1, r1, 1              // move pointer to next
   
Fifo30100_skip_store:
    dec icounter                // icounter -> r0
    jumpr next_elemFifo30100, 0, gt    

    move r2, resul_pointer  
    ld r0, r2, 0
    st r1, r0, 0   /// store result from r1 !!!!!!
    jump i2cMultyHandlerEnd
    // -- ret  // this is end of handler
    
   
    .global writeMULTY // i
writeMULTY:
    ld r0,r3,8 // data byte 1
    move r1, cnt_read
    st r0, r1, 0

    move r1, resul_pointer
    st r2, r1, 0
   
    psr
    jump write_intro

    store_mem cnt_read icounter  
write_next_byteN:
    dec icounter                // icounter -> r0

    move r1, resul_pointer
    ld r1,r1,0 // load pointer from memory
    ld r2,r1,0 // data byte
    psr
    jump i2c_write_byte
    jumpr i2c_fail_2, 1, ge
      
    inc resul_pointer  // move pointer to next

    move r2, icounter
    ld r0, r2, 0
    jumpr write_next_byteN, 0, gt
   
    psr
    jump i2c_stop_cond 
   
    move r2, 0 // Ok
    ret


i2c_fail_2://same code but i can't go to first due to  "relocation out of range"
  move r2, 1
  ret
