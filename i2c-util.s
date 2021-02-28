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

  .global readMULTY // ugly, memory non-efficient, it store each uint8 to uint32 memory cell =)
readMULTY:
   move r1, cnt_read
   st r0, r1, 0

   move r1, resul_pointer
   st r2, r1, 0

   psr
   jump read_intro

   store_mem cnt_read icounter  
next_byteN:
   dec icounter                // icounter -> r0
   jumpr last_byte, 0, eq
   move r2,0
   jump end_byte_select
last_byte:
   move r2,1 // last byte
end_byte_select:

   psr
   jump i2c_read_byte
   push r0

   move r2, icounter
   ld r0, r2, 0
   jumpr next_byteN, 0, gt
  
   psr
   jump i2c_stop_cond 
      
   store_mem cnt_read icounter
   move r2, resul_pointer  
   ld r1, r2, 0
   
next_elem:
   pop r0

   st r0, r1, 0               // Save data in memory 
   sub r1, r1,1                // move pointer to next
   dec icounter                // icounter -> r0
   jumpr next_elem, 0, gt    
   move r2,0
   ret
   

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
   jumpr fail,1,ge
      
   inc resul_pointer  // move pointer to next

   move r2, icounter
   ld r0, r2, 0
   jumpr write_next_byteN, 0, gt
   
   psr
   jump i2c_stop_cond 
   
   move r2, 0 // Ok
   ret
