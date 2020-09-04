

/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   emulation of 6522 VIA
   Copyright (C) 2012 Gordon JC Pearce
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * | Register #  | RS3 | RS2 | RS1 | RS0 | Register NAME |                DESCRIPTION                  |
 * |_____________|_____|_____|_____|_____|_______________|______Write___________|____READ______________|
 * |       0     |  0  |  0  |  0  |  0  |   ORB/IRB     |     Output Reg B     |  Input Register B    |
 * |       1     |  0  |  0  |  0  |  1  |   ORA/IRA     |     Output Reg A     |  Input Register A    |
 * |       2     |  0  |  0  |  1  |  0  |     DDRB      |           Data Direction Register A         | 
 * |       3     |  0  |  0  |  1  |  1  |     DDRA      |           Data Direction Register B         |
 * |       4     |  0  |  1  |  0  |  0  |    T1C-L      | T1 Low-Order Latches | T1 Low-Order Counter |
 * |       5     |  0  |  1  |  0  |  1  |    T1C-H      |           T1 High-Order Counter             |
 * |       6     |  0  |  1  |  1  |  0  |    T1L-L      |           T1 Low-Order Latches              |
 * |       7     |  0  |  1  |  1  |  1  |    T1L-H      |           T1 High-Order Latches             |
 * |       8     |  1  |  0  |  0  |  0  |    T2C-L      | T2 Low-Order Latches | T2 Low-Order Counter |
 * |       9     |  1  |  0  |  0  |  1  |    T2C-H      |           T2 High-Order Counter             |
 * |       A     |  1  |  0  |  1  |  0  |      SR       |               SHIFT Register                |
 * |       B     |  1  |  0  |  1  |  1  |     ACR       |           Auxiliary  CONTROL Register       |
 * |       C     |  1  |  1  |  0  |  0  |     PCR       |           Peripheral CONTROL Register       |
 * |       D     |  1  |  1  |  0  |  1  |     IFR       |           INTERRUPT FLAG Register           |
 * |       E     |  1  |  1  |  1  |  0  |     IER       |           INTERRUPT Enable Register         |
 * |       F     |  1  |  1  |  1  |  1  |   ORA/IRA     |        Same as REG 1, but no Handshake      |
 * +-------------+-----+-----+-----+-----+---------------+---------------------------------------------+
 *             
 *             
 *             
 *            #E20D     INTERRUPT FLAG REGISTER
 *            +---------+---------+---------+---------+---------+---------+---------+---------+
 *            | (bit 7) | (bit 6) | (bit 5) | (bit 4) | (bit 3) | (bit 2) | (bit 1) | (bit 0) | 
 *            |   IRQ   | Timer 1 | Timer 2 |   CB1   |   CB2   | Shift R |   CA1   |   CA2   |
 *            +---------+---------+---------+---------+---------+---------+---------+---------+
 *            
 *            
 *            There are three basic interrupt operations:  
 *             a) setting the interrupt flag within IFR
 *             b) enabling the interrupt by way of a corresponding bit in the IER, and 
 *             c) signaling the microprocessor using IRQB.  
 *             
 *             An Interrupt Flag can be set by conditions internal to the chip or by inputs to the  
 *             chip from external sources.    
 *             Normally, an Interrupt Flag will remain set until the interrupt is serviced.  
 *             To determine the source of an interrupt, the microprocessor will examine each flag in order, 
 *             from highest to lowest priority.  
 *             This is accomplished by reading the contents of the IFR into the microprocessor accumulator, 
 *             shifting the contents either left or right and then using conditional branch instructions to  
 *             detect  an  active  interrupt.    
 *             Each Interrupt Flag has a corresponding  Interrupt  Enable  bit  in  the  IER.    
 *             The  enable  bits  are  controlled  by  the  microprocessor  (set  or  reset).    
 *             If  an  Interrupt  Flag  is  a  Logic  1,  and  the  corresponding  Interrupt Enable bit is 
 *             a Logic 1, the IRQB will go to a Logic 0. 
 *             
 *             FR7 is not a flag.  
 *             Therefore, IFR7 is not directly cleared by writing a Logic 1 into its bit position.  
 *             It can be cleared, however,  by  clearing  all  the  flags  within  the  register,  or  by  
 *             disabling  all  active  interrupts.
 *             
 *             TO DO
 *             Reading from VIA 6522
 *             
*** VIA6522 READ: via_rreg read of T1 Low-Order Latches  - UNHANDLED ****
Writing to VIA 6522
*** VIA6522 >WRITE<  of T1 Low-Order Latches - UNHANDLED 06 <= 00

Reading from VIA 6522
*** VIA6522 READ: via_rreg read of  T1 High-Order Latches   UNHANDLED *****
Writing to VIA 6522
*** VIA6522 >WRITE< :of 1 High-Order Latches  - UNHANDLED 07 <= 00

Reading from VIA 6522
*** VIA6522 READ: via_rreg read T1 Low Order COUNTER - UNHANDLED 
Writing to VIA 6522
*** VIA6522 >WRITE<: of T1 Low-Order Latches - UNHANDLED 04 <= 00
* 
Reading from VIA 6522
*** VIA6522 READ: via_rreg read T1 High-Order COUNTER  - UNHANDLED ****
Writing to VIA 6522
*** VIA6522 >WRITE<  of T1 High-Order Counter - UNHANDLED 05 <= 00
 */

   

#define VIA6522_DEBUG 1

#include "via.h"
#include "Arduino.h"

/*
VIA is clocked at 2MHz
T2 is programmed to divide by 5000
on the analogue board there is a 4052 dual mux with pin 3 going to VCF8
and pin 13 feeding the ADC

portB
 4  3       pin 3   pin 13
 0  0  0     mic    compressed
 0  1  1    line    uncompressed
 1  0  2    gnd     pitch wheel
 1  1  3    gnd     modwheel
note that pin 4 also drives the top of the volume control, so selecting the
sample input mutes the audio.

Register maps
bit   Port A               Port B
0     LED segment          bank 0/1
1     LED segment          upper/lower
2     LED segment          mic/line
3     anode 1 active low   sample/play
4     anode 2 active low   drive select/motor on (active low)
5     keypad row 0         DOC CA3 input << THIS IS FOR SOME SINCHRONIZATION, CHECK DOC5503 Model
6     keypad row 1         disk ready
7     keypad row 2         (not used, clock to ACIA)

*/
extern unsigned long get_cpu_cycle_count();
unsigned long via_cycles, via_t2=0;

void via_init() {
  via_t2 = 0;
    
  via.orb = 0x00;
  via.irb = 0x00;
  via.ora = 0x00; 
  via.ira = 0x00;
  via.ddra = 0x00; 
  via.ddrb = 0x00;
  via.t2l = 0x00;
  via.t2h = 0x00;
  via.ier = 0x00; 
  via.ifr = 0x00;
  via.pcr = 0x00;
}


uint8_t via_irq() {
 // I NEED TO IMPLEMENT THIS: NEED TO USE IER AS WELL
 //if((via.ifr & 0x80) == 0x80) {
 //           Serial.printf("VIA 6522: IRQ Fired");
 //           Serial.printf(" get_cpu_cycle_count() = %ld\n", get_cpu_cycle_count());
 //           }
  return((via.ifr & 0x80) == 0x80);  // mask 1000_0000 for bit 7, IRQ 
}


uint8_t via_run() {
  
  if (get_cpu_cycle_count() < via_cycles) return(0); // not ready yet

  
// via_cycles is set by timer2
// The following code is meant to trigger an interrupt IRQ when Timer 2 finishes

  if (via.ier & 0x20) { // mask is 0010_0000, so we are checking bit 5 of IER, which is Timer 2
    via.ifr |= 0xa0;    // the OR mask is 1010_0000, IRQ and timer 2 interrupt interrupt flag
                        // at this point the IRQ has been set. The portion of the model that clears timer 2 will also need to clear IRQ
    //  return (1);  // fire interrupt
  }
  via_cycles = get_cpu_cycle_count() + (via_t2>>2);  // half, because the clock frequency is 2MHz
  return(0);
}


uint8_t via_rreg(int reg) {

  uint8_t val=0;

  switch(reg) {
    case 0:
      //val = via.orb;
      val = (via.orb & 0x1f) | 0x40; // fake disk ready
      break;
    case 1:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read T1 IRA - UNHANDLED***\n");
#endif
      val =  0;
     break;
    case 2:
      val = via.ddra;
     break;
    case 3:
      val = via.ddrb;
     break;
    case 4: // T1 Low-Order Counter
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read T1 Low Order COUNTER - UNHANDLED\n");
#endif
      val = 0;
     break;
    case 5: //T1 High-Order COUNTER t1HC
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read T1 High-Order COUNTER  - UNHANDLED ****\n");
#endif
      val = 0;
     break;
    case 6:  //T1 Low-Order Latches t1LL  
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read of T1 Low-Order Latches  - UNHANDLED ****\n");
#endif
      val = 0;
     break;
    case 7:  // T1 High-Order Latches t1HL
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read of  T1 High-Order Latches   UNHANDLED *****\n");
#endif
      val = 0;
     break;
    case 8: //T2 Low-Order Counter t2LC
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read of T2 Low-Order Counter - UNHANDLED *****\n");
#endif
      val = 0;
     break;
    case 9: //  T2 High-Order Counter  t2HC  
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read of  T2 High-Order Counter - UNHANDLED **** \n");
#endif
      val = 0;
     break;
    case 10:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read shift register unhandled\n");
#endif
      val = 0;
      break;
    case 12:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg PCR = %02x\n", via.pcr);
#endif
      val = via.pcr;
      break;
    case 13:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg IFR = %02x\n", via.ifr);
#endif
      val = via.ifr;
      break;
    case 14:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg IER = %02x\n", via.ier);
#endif
      val = via.ier;
      break;  
    case 15:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: via_rreg read IRA no handshake - UNHANDLED ****\n");
#endif
     val = 0;
      break;
    default:
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 READ: >>>SHOULD NOT GET HERE <<< via_rreg(%d, 0x%02x)\n",  reg, val);
#endif
      break;
    }

  return val;
}

void via_wreg(int reg, uint8_t val) {
  int bc;

  switch(reg) {
    case 0: // PORT B
    Serial.printf("*** VIA6522 WRITE: portb %02x. Page = %0X \n", val, val & 0x03);
 
 #if VIA6522_DEBUG
      bc = val ^ via.orb;
      Serial.println("vvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
      Serial.printf("*** VIA6522 WRITE: New Port B value: %02x.Previous value: %02x. bc is %02x. val & 0x10 = %x \n", val, via.orb, bc, val & 0x10);
      if (bc == 0) Serial.printf("No change ***\n");
      if (bc & 0x01) Serial.printf("**** Bank = %s \n",  (val & 0x01) ?   "0"   : "1");
      if (bc & 0x02) Serial.printf("**** Half = %s \n",  (val & 0x02) ? "lower" : "upper");
      if (bc & 0x04) Serial.printf("**** Input = %s \n", (val & 0x04) ? "line"  : "mic");
      if (bc & 0x08) Serial.printf("**** Mode = %s \n",  (val & 0x08) ? "play"  : "sample");
      if (bc & 0x10) Serial.printf("**** FDC = %s \n",   (val & 0x10) ? "off"   : "on");
      Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
#endif

      via.orb = val;
      return;
    case 1: // port A only used for keypad and display
 #if VIA6522_DEBUG
       Serial.printf("*** VIA6522 >WRITE<: TODO Add Display emulation\n");
 #endif
      return;
    case 2:
 #if VIA6522_DEBUG
      Serial.printf("*** VIA6522 >WRITE<: ddrb=%02x\n", val);
 #endif
      via.ddrb = val;
      break;
    case 3:
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 >WRITE<: ddra=%02x\n", val);
#endif
      via.ddra = val;
      break;
    case 4: // T1 Low-Order Latches 
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 >WRITE<: of T1 Low-Order Latches - UNHANDLED %02x <= %02x\n", reg, val);
#endif
    break;
    case 5: // T1 High-Order Counter     
#if VIA6522_DEBUG
      Serial.printf("*** VIA6522 >WRITE<  of T1 High-Order Counter - UNHANDLED %02x <= %02x\n", reg, val);
#endif
    break;
    case 6: //T1 Low-Order Latches    
#if VIA6522_DEBUG   
      Serial.printf("*** VIA6522 >WRITE<  of T1 Low-Order Latches - UNHANDLED %02x <= %02x\n", reg, val);
#endif
    break;
    case 7: //T1 High-Order Latches    
#if VIA6522_DEBUG   
      Serial.printf("*** VIA6522 >WRITE< :of 1 High-Order Latches  - UNHANDLED %02x <= %02x\n", reg, val);
#endif
      break;
    case 8: // T2 Low-Order Latches FOLLOWING CODE IS SUSPICIOUS: 0x08 register is for latches, not counter
      via.t2l = val;
      via_t2 = via.t2l | (via.t2h<<8);
      via_cycles = get_cpu_cycle_count() + (via_t2>>2);  // half, because the clock frequency is 2MHz
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 REG #8 WRITE: t2 = %04x\n", (int) via_t2);
#endif
      break;
    case 9: // T2 High-Order Counter
      via.t2h = val;
      via_t2 = via.t2l | (via.t2h<<8);
      via_cycles = get_cpu_cycle_count() + (via_t2>>2);  // half, because the clock frequency is 2MHz
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 REG #9 WRITE: t2 = %04x\n", (int) via_t2);
#endif
      break;
    case 10:  // 0xE20A
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 WRITE: sr<=%02x\n",  val);
#endif
      break;
    case 11:  // 0xE20B
#if VIA6522_DEBUG  
     Serial.printf("*** VIA6522 WRITE:  acr<=%02x\n", val);
#endif
      break;
    case 12:  // 0xE20C
      via.pcr = val;
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 WRITE:  pcr<=%02x\n", val);
#endif
      break;
    case 13:  // 0xE20D  IFR
      val &= 0x7f;     // mask is 0111_1111, IFR bit 7 can only be cleared when disabling the interrupt flag that generated it
      via.ifr &= ~val; // bitwise update of IFR with val
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 >>WRITE<< *** STUDY INTERRUPT FLAG REGISTER *** val=%02x ifr<=%02x\n", val, via.ifr);
#endif
      break;
    case 14:  // 0xE20E IER
      // okay, this is a funny one.  If bit 7 is set, the remaining bits
      // that are high are set high in IER.  If it is unset, the remaining
      // bits that are high are cleared.
      
       Serial.printf("*** VIA6522 WRITE: ORIGINAL VALUE FOR ier=%02x\n", via.ier);
      if (val & 0x80) via.ier |= (val & 0x7f);
        else via.ier &= (~val);
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 WRITE:  val=%02x ier=%02x\n", val, via.ier);
      // one write shows val == a6 and ier = 26
      // val is 1010_0110, so (val & 0x80) => via.ier = via.ier | (val & 0x7f)
      //                                      via.ier = 0000_0000 | 0010_0110
      // which means it will return 26, we are enabling SHIFT REGISTER and CA1 interrupts 
      // in addition to T2 INTERRUPT
#endif
      break;
    case 15:  // 0xE20F
#if VIA6522_DEBUG  
      Serial.printf("*** VIA6522 WRITE:  ora<=%02x\n",  val);
#endif
      break;
    default:
#if VIA6522_DEBUG  
    Serial.printf("*** VIA6522 WRITE: >>>STILL UNMANAGED<<< via_wreg(%d, 0x%02x)\n", reg, val);
#endif
    break;  
  }
}
