!************************************************************************!
!									 !
! general calling convention:						 !
!									 !
! (1) Register usage is as implied in the assembler names		 !
!									 !
! (2) Stack convention							 !
!									 !
!	  The stack grows towards higher addresses.  The stack pointer	 !
!	  ($sp) points to the next available (empty) location.		 !
!									 !
! (3) Mechanics								 !
!									 !
!	  (3a) Caller at call time:					 !
!	       o  Write any caller-saved stuff not saved at entry to	 !
!		  space on the stack that was reserved at entry time.	 !
!	       o  Do a JALR leaving the return address in $ra		 !
!									 !
!	  (3b) Callee at entry time:					 !
!	       o  Reserve all stack space that the subroutine will need	 !
!		  by adding that number of words to the stack pointer,	 !
!		  $sp.							 !
!	       o  Write any callee-saved stuff ($ra) to reserved space	 !
!		  on the stack.						 !
!	       o  Write any caller-saved stuff if it makes sense to	 !
!		  do so now.						 !
!									 !
!	  (3c) Callee at exit time:					 !
!	       o  Read back any callee-saved stuff from the stack ($ra)	 !
!	       o  Deallocate stack space by subtract the number of words !
!		  used from the stack pointer, $sp			 !
!	       o  return by executing $jalr $ra, $zero.			 !
!									 !
!	  (3d) Caller after return:					 !
!	       o  Read back any caller-saved stuff needed.		 !
!									 !
!************************************************************************!

!vector table
 vector0: .fill 0x00000000 !0
 .fill ti_inthandler !1
 .fill 0x00000000 !2
 .fill 0x00000000
 .fill 0x00000000 !4
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000 !8
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000
 .fill 0x00000000 !15
!end vector table


main:
	addi $sp, $zero, initsp ! initialize the stack pointer
	lw $sp, 0($sp)
	! Install timer interrupt handler into the vector table
        !FIX ME

		
	addi $a0, $zero, 4	!load base for pow
	addi $a1, $zero, 7	!load power for pow
	addi $at, $zero, POW			!load address of pow
	jalr $at, $ra		!run pow
		
	halt	
		
		

POW: 
  addi $sp, $sp, 2   ! push 2 slots onto the stack
  sw $ra, -1($sp)   ! save RA to stack
  sw $a0, -2($sp)   ! save arg 0 to stack
  beq $zero, $a1, RET1 ! if the power is 0 return 1
  beq $zero, $a0, RET0 ! if the base is 0 return 0
  addi $a1, $a1, -1  ! decrement the power
  addi $at, $zero, POW    ! load the address of POW
  jalr $at, $ra   ! recursively call POW
  add $a1, $v0, $zero  ! store return value in arg 1
  lw $a0, -2($sp)   ! load the base into arg 0
  addi $at, $zero, MULT   ! load the address of MULT
  jalr $at, $ra   ! multiply arg 0 (base) and arg 1 (running product)
  lw $ra, -1($sp)   ! load RA from the stack
  addi $sp, $sp, -2  ! pop the RA and arg 0 off the stack
  jalr $ra, $zero   ! return
RET1: addi $v0, $zero, 1  ! return a value of 1
  addi $sp, $sp, -2
  jalr $ra, $zero
RET0: add $v0, $zero, $zero ! return a value of 0
  addi $sp, $sp, -2
  jalr $ra, $zero		
	
MULT: add $v0, $zero, $zero ! zero out return value
AGAIN: add $v0,$v0, $a0  ! multiply loop
  addi $a1, $a1, -1
  beq $a1, $zero, DONE ! finished multiplying
  beq $zero, $zero, AGAIN ! loop again
DONE: jalr $ra, $zero	
		
		


ti_inthandler:
! Code Entering an Interrupt
addi $sp, $sp, 6   ! allocate space on stack for registers leaving blank space on top
sw $k0, -6($sp)
ei                ! save interrupt return and enable interrupts
sw $ra, -5($sp)   ! push return address
sw $fp, -4($sp)   ! push frame pointer
sw $s0, -3($sp)   ! push s registers that will be used
sw $s1, -2($sp)
sw $s3, -1($sp)
!----------------------------




addi $s3, $zero, mem
lw $s3, 0($s3)


addi $s1, $zero, 60           ! will be comparing to 60 for sec and min

lw $s0, 0($s3)             ! load seconds into s0
addi $s0, $s0, 1               ! increment value

beq $s1, $s0, newMinute       ! run newMinute if 60 seconds have elapsed

sw $s0, 0($s3)             ! if branch was unsuccesful save sec value to memory and return
beq $zero, $zero, return       ! jumps to return code otherwise



newMinute:
    sw $zero, 0($s3)       ! writes 0 to the mem[sec]

    lw $s0, 1($s3)         ! load minutes into s0
    addi $s0, $s0, 1         ! increment minutes

    beq $s1, $s0, newHour   ! run newHour if 60 minutes have elapsed

    sw $s0, 1($s3)         ! if branch unsuccesful save min value to memory and return
    beq $zero, $zero, return


newHour:
    addi $s1, $zero, 24        ! we are now comparing to 24
    sw $zero, 1($s3)       ! write 0 to the mem[min]

    lw $s0, 2($s3)          ! load hours into s0
    addi $s0, $s0, 1         ! increment hours

    beq $s1, $s0, newDay    ! run newDay if 24 hours have elapsed

    sw $s0, 2($s3)         ! if branch unsuccesful save hour value to mem and return
    beq $zero, $zero, return


newDay:
    sw $zero, 2($s3)       ! start hours over at 0
    beq $zero, $zero, return

return:	
    ! Code Preparing to Return from Interrupt
    lw $s3, -1($sp)
    lw $s1, -2($sp)
    lw $s0, -3($sp)
    lw $fp, -4($sp)
    lw $ra, -5($sp)
    di
    lw $k0, -6($sp)
    addi $sp, $sp, -6
    reti
    !------------------------------------------
	
mem: .fill 0xF00000     ! Memory index of time state	
initsp: .fill 0xA00000