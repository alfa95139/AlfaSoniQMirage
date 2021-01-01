

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


#E20B ACR Register: used for T2 and T1, and to manage the serial port (SR) which is CB1 and CB2 
------------------
Boot Value 0xCC (1100_1100) 
{ACR7, ACR6} = {1,1} 
T1 = continuous interrupts, squarewave on PB7. PB7 is the clock to the 6850 ACIA 
We will not likely need  T1 to model the ACIA
{ACR5} = {0}
T2 counts a pretermined number of pulses on PB6, used for floppy, we will not need it 
{ACR4, ACR3, ACR2}  = {0,1,1}
Shift in under ext clock: this refer to the Shift Register (CB1 and CB2)


#E20C PCR Register:
------------
Boot Value 0x0E (0000_1110):  cb2 in/falling, cb1 falling, ca2 high out, ca1 falling
PCR7,6,5 = {000} interrupt input mode CB2 (IFR3) on a negative transition of CB2 input.
                 Clear IFR3 on a read or write of the Peripheral B Output Register ORB
                 ---------------------------------------------------------------------
PCR4 = 0         CB1 interrupt flag (IFR4) is set after CB1 falling
                 Clear IFR4 on a read or write of the Peripheral B Output Register ORB
                 ---------------------------------------------------------------------
PCR3,2,1 = {111} Manual output mode. CA2 is held high
PCR0 = 0	 CA1 interrupt flag (IFR1 ) is set after CA1 falling
		 

ABOUT THE MIRAGE MANAGING THE VIA #E20D  INTERRUPT FLAG REGISTER

       +---------+---------+---------+---------+---------+---------+---------+---------+
       | (bit 7) | (bit 6) | (bit 5) | (bit 4) | (bit 3) | (bit 2) | (bit 1) | (bit 0) |
       |   IRQ   | Timer 1 | Timer 2 |   CB1   |   CB2   | Shift R |   CA1   |   CA2   |
       +---------+---------+---------+---------+---------+---------+---------+---------+

Note that before calling the OS
        lda   #$a6      ; SET, T2, SR, CA1: 1010_0110
        sta   $e20e     ; VIA interrupt enable reg

The IRQ Interrupt routine services the IRQ generated by the Mirage VIA in the following order:
- Shift R (when assembling serial data from the keyboard scanner)
- Timer 2: this fires off 400 times per second, and I understand it is used to service all the peripherals
in a regular fashion
- CA1 which is the external Sync 


AF 12/07/2020
Found this: https://github.com/MisterTea/MAMEHub/blob/master/Sources/Emulator/src/mess/drivers/mirage.c
We implement this in the routine in the bottom (via_keydisp() )
LED / switch matrix:
            A           B           C             D         E         F         G        DP
    ROW 0:  LOAD UPPER  LOAD LOWER  SAMPLE UPPER  PLAY SEQ  LOAD SEQ  SAVE SEQ  REC SEQ  SAMPLE LOWER
    ROW 1:  3           6           9             5         8         0         2        ENTER
    ROW 2:  1           4           7             ^         PARAM     v         VALUE    CANCEL
    L. AN:  SEG A       SEG B       SEG C         SEG D     SEG E     SEG F     SEG G    SEG DP 
    R. AN:  SEG A       SEG B       SEG C         SEG D     SEG E     SEG F     SEG G    SEG DP
    
    Column number in VIA port A bits 0-2 is converted to discrete lines by a 74LS145.
    Port A bit 3 is right anode, bit 4 is left anode
    ROW 0 is read on VIA port A bit 5, ROW 1 in port A bit 6, and ROW 2 in port A bit 7.

PA2, PA1, PA0  select on of the eight possibilities (via the 74LS145). This affects either one of the displays, or one of the columns of the keypad.
The corresponding segment needs to be LOW.
PA3 is ANODE 2 RIGHT DISPLAY -> When it is HIGH it selects the left  display. 
PA4 is ANODE 1 LEFT  DISPLAY -> When it is HIGH it selects the right display. 
PA7 (ROW 2), PA6 (ROW 1), PA5 (ROW 0) read the row. This will give the coordinate of the pressed key.
                            For instance, if the selected Segment is G, and (PA7, PA6, PA5) = (1, 1, 0) the selected key would be REC SEQ.
                            Basically the column (segment) brings the value LOW of the row of which the button is pressed.
 */

   

#define VIA6522_DEBUG 0

//#include "bus.h"
#include "via.h"
#include "log.h"
#include "Arduino.h"

struct {
  uint8_t orb, irb;
  uint8_t ora, ira;
  uint8_t ddrb, ddra;
  uint8_t t1LL, t1HL; // t1 Latches (Low and High) 
  uint8_t t1l, t1h;   // t1 Counter (Low and High)
  uint8_t t2l, t2h;   // t2 Counter (Low and High)
  uint8_t ier, ifr;
  uint8_t acr;
  uint8_t pcr;
  uint8_t sr;

} via;

/*
VIA is clocked at 2MHz
T2 is programmed to divide by 5000
On the analogue board there is a 4052 dual mux with pin 3 going to VCF8
and pin 13 feeding the ADC

+---------------+-----------------------------------+
|   Port B      |               4052 pins           |
|  PB3  PB2     |       pin 3       |    pin 13     |
|---------------+-------------------+---------------+
|   0   0 -> 0  |   mic (sample)    |  compressed   |
|   0   1 -> 1  |   line            |  uncompressed |
|   1   0 -> 2  |   gnd             |  pitch wheel  |
|   1   1 -> 3  |   gnd             |  modwheel     |
+---------------+-----------------------+---------------+
note that bit 4 (PB3)  also drives the top of the volume control, so selecting the
sample input mutes the audio.
When bit 4 is high (which means that the volume control is active) it means that the keyboard is playing.
So the ADC in the DOC can decode the pitch-wheel and modewheel voltage values (bit 3 - PB2).
Which means if "Play" then bit 3 (PB2) select pitchwheel vs modwheel.
 
Register maps
bit   I/O A  Port A              I/O B   Port B                                                    
0      O     LED segment          O      bank 0/1                                            0x01          
1      O     LED segment          O      upper/lower                                         0x02
2      O     LED segment          O      mic/line                                            0x04
3      O     anode 1 active low   O      sample/play                                         0x08
4      O     anode 2 active low   O      drive select/motor on (active low)                  0x10
5      I     keypad row 0         I      DOC CA3 input << THIS IS FOR SOME SYNC w DOC5503    0x20
6      I     keypad row 1         I      disk ready                                          0x40
7      I     keypad row 2         I      (not used, clock to ACIA)                           0x80

The boot rom sets both DDRA and DDRB to 0001_1111 => 0x1F


*/

// CA1 is +5Vcc per DMS-1 schematic. CA2 is floating.
// CB1 is +5Vcc per DMS-1 schematic. CB2 is floating


extern unsigned long get_cpu_cycle_count();
unsigned long via_cycles, via_t2=0;



void via_init() {
  via_t2 = 0;
    
  via.orb = 0x00;
  via.irb = 0x00;
  via.ora = 0xE0; // no keys pressed
  via.ira = 0x00;
  via.ddra = 0x00; 
  via.ddrb = 0x00;
  via.t2l = 0x00;
  via.t2h = 0x00;
  via.ier = 0x00; 
  via.ifr = 0x00;
  via.pcr = 0x00;
  via.sr =  0xff;
}


//***********************************************************
// * Purpose of via_irq is to model the VIA IRQ based on T2 *
//***********************************************************
 uint8_t via_irq() {
  if (via.ier & 0x20) // if T2 Interrupt Enable
  	if (!(via.ifr & 0x20)) 
  		via.ifr &= (0xef); // clear IRQ

  return((via.ifr & 0x80) == 0x80);  // mask 1000_0000 for bit 7, IRQ 
}


 
/*
void via_irq_callback(){
 if (via.ier & 0x20) { // mask is 0010_0000, so we are checking bit 5 of IER, which is Timer 2
     via.ifr |= 0xa0;    // the OR mask is 1010_0000, IRQ and timer 2 interrupt interrupt flag
                        // at this point the IRQ has been set. 
  }
 
 if (via.ier & 0x20) // if T2 Interrupt Enable
    if (!(via.ifr & 0x20)) 
      via.ifr &= (0xef); // clear IRQ

  if((via.ifr & 0x80) == 0x80)  // mask 1000_0000 for bit 7, IRQ 
      fire_via_irq();
}

void fire_via_irq() { //(CPU6809* cpu) {
  //cpu->irq();
  log_debug("FIRED VIA T2 INTERRUPT ");
  return;
}
*/

//************************************************************************
// * Purpose of via_run is to generate the irq when T2 finishes counting *
//************************************************************************
void via_run(CPU6809* cpu) {

  
  if (get_cpu_cycle_count() < via_cycles) {
          //log_debug("*** Waiting for T2 to catch up\n");
          return; // not ready yet
  }

// via_cycles is set by timer2
// The following code is meant to trigger an interrupt IRQ when Timer 2 finishes
// Note that IFR5 is reset when a new write to T2 happens, and via_cycles gets updated
// Which is why we need to reset the irq before the if (clock_cycles < via cycles) above...

  if (via.ier & 0x20) { // mask is 0010_0000, so we are checking bit 5 of IER, which is Timer 2
     via.ifr |= 0xa0;    // the OR mask is 1010_0000, IRQ and timer 2 interrupt interrupt flag
                        // at this point the IRQ has been set. 
  }
  
  via_cycles = get_cpu_cycle_count() + (via_t2)>>1;  // half, because the clock frequency is 2MHz

  // The portion of the model that clears timer 2 will also need to clear IRQ
   if (via_irq()) {
      cpu->irq();
      }  
// AF 120920 TODO: Implement T2 IRQ using TeensyTimerTool
// Timer T2;  // generate a timer from the pool (Pool: 2xGPT, 16xTMR(QUAD), 20xTCK)
// Timer Tsr;  // generate a timer for the shift register
//...
// T2.beginPeriodic( T2_irq(), 2.5);  //2.5 milliseconds (1 / 400Hz = 2.5 millis)
// Tsr.beginPeriodic( Tsr_irq(), 3.125 );  //3.125 milliseconds, 320 characters per second
// ...
//
      
  return;
}


uint8_t via_rreg(uint8_t reg) {

  uint8_t val=0;

  switch(reg) {
    case 0x00:
      //val = via.orb;
#if VIA6522_DEBUG 
    //  log_debug("VIA 6522 READ FROM PORT B: TO DO - MAY NEED TO IMPLEMENT CA3 SYNC with DOC5503 \n");
#endif
      val = (via.orb & 0x1f) | 0x40; // fake disk ready, but I could check whether I have an *.img in the SD Card....  
#if VIA6522_DEBUG 
      if ((val & 0x00100000) == 0x00100000) log_debug("VIA 6522: Received synchronization signal CA3 from DOC"); 
#endif
      break;
    case 0x01:
    val =  via.ora; // 
#if VIA6522_DEBUG
      log_debug("*** VIA6522 >READ<: PoRT A =%0x TODO Add Display emulation \n", val);
#endif
     break;
    case 0x02:
#if VIA6522_DEBUG
      log_debug("VIA read  Direction Register A");
#endif
      val = via.ddra;
     break;
    case 0x03:
#if VIA6522_DEBUG
      log_debug("VIA read  Direction Register B");
#endif
      val = via.ddrb;
     break;
    case 0x04: // T1 Low-Order Counter
#if VIA6522_DEBUG
      log_debug("***** VIA read  T1 Low Order COUNTER - UNHANDLED\n");
#endif
      val = 0;
     break;
    case 0x05: //T1 High-Order COUNTER t1HC
#if VIA6522_DEBUG
      log_debug("***** VIA read  T1 High Order COUNTER - UNHANDLED\n");
#endif
      val = 0;
     break;
    case 0x06:  //T1 Low-Order Latches t1LL  
#if VIA6522_DEBUG
      log_debug("***** VIA read  T1 Low Order Latches - UNHANDLED\n");
#endif
      val = 0;
     break;
    case 0x07:  // T1 High-Order Latches t1HL
#if VIA6522_DEBUG
      log_debug("***** VIA read  T1 High Order Latches - UNHANDLED\n");
#endif
      val = 0;
     break;
    case 0x08: //T2 Low-Order Counter t2LC
#if VIA6522_DEBUG
      log_debug("VIA read  T2 Low Order COUNTER");
#endif
      val = via.t2l;
     break;
    case 0x09: //  T2 High-Order Counter  t2HC  
#if VIA6522_DEBUG
      log_debug("VIA read  T2 High Order COUNTER");
#endif
      val = via.t2h;
     break;
    case 0x0A:
#if VIA6522_DEBUG
      log_debug("VIA read Shift Register %0X -UNHANDLED (MIDI only, Mirage keyboard disconnected)\n", via.sr);
#endif
      val = via.sr;
      break;
    case 0x0B:
#if VIA6522_DEBUG 
//---------------------------------------------------------------------------------------------------------------
//NOTE: CB1 and CB2 are not connected, just like in the Ensoniq DMS-8
//      I think the SW sets it = 0xCC -> 11           0                   011                    0        0 
//                                                                        SHIFT REGISTER CTRL
//                                       Timer1       Timer 2             Shift                  PB       PA
//                                       Continuous   Timed Interrupt     Under control          Latch    Latch
//                                       Interrupt                        of external clock      Disable  Disable
//----------------------------------------------------------------------------------------------------------------
      log_debug("VIA read  ACR = %0X \n", via.acr);
      if( (via.acr & 0x0C) == 0x0C) log_debug("VIA ACR - Shift Register Control CB2 Shift under control of External Clock CB1\n");
          else log_debug("VIA ACR - Shift Register Control for CB1/CB2 is set up in an unexpected way\n");
#endif
     val = via.acr;
     break;
    case 0x0C:
#if VIA6522_DEBUG
//----------------------------------------------------
//NOTE: for CB2 and CB1 we expect (0,0,0) and (0). 
//      Which means for CB2 Control -> Operation Input Negative Active Edge, for CB1 Interrupt control on the Active Edge.
//      These are used together with the shift register to get the serial data from the 6500/11 (note on/off + velocity) from the keyboard. 
//      In this implementation we will not use them.
//----------------------------------------------------- 
      log_debug("VIA Read PCR = %02X\n", via.pcr);
      log_debug("         PCR: CB2 Control{CB2ctr_2, CB2ctr_1, CB2ctr_0} = [%0X%0X%0x]\n", (((via.pcr & 0xE0) >> 7) & 0x01), (((via.pcr & 0xE0) >> 6) & 0x01), (((via.pcr & 0xE0) >> 5) & 0x01) );
      log_debug("         PCR: CB1 Interrupt Control = %0X\n", via.pcr & 0x10 );
//-----------------------------------------------------
//NOTE: The Sync jack on the back panel was intended to sync the Mirage's sequencer to an external clock source; 
//      Since the sequencer is all but useless, the Sync jack and circuitry were dropped on the later DSK-1.      
      log_debug("         PCR: CA2 Control {CA1_2,CA2_1,CA2_0}     = [%0X%0x%0x]\n", (((via.pcr & 0x0E)>>3) & 0x01), (((via.pcr & 0x0E)>>2) &0x01), (((via.pcr & 0x0E)>>1) &0x01) );
      log_debug("         PCR: CA1 Interrupt Control = %0X\n",  via.pcr & 0x01);
//------------------------------------------------------
#endif
      val = via.pcr;
      break;
    case 0x0D:
#if VIA6522_DEBUG
      log_debug("VIA read  IFR = %02x\n", via.ifr);
#endif
      val = via.ifr;
      break;
    case 0x0E: //IF A READ OF THIS REGISTER IS DONE, BIT 7 WILL BE "1" AND ALL OTHER BITS WILL REFLECT THEIR ENABLE/DISABLE STATE.
      via.ier = via.ier | 0x80;
      val = via.ier;
#if VIA6522_DEBUG
      log_debug("VIA read  IER = %02x\n", via.ier);
#endif
      break;  
     case 0x0F:       
     val = via.ora;  
#if VIA6522_DEBUG
     log_debug("VIA read  PORT A (NO HNDSHKE) = %0x TODO Add Keypad/Display emulation=====\n", val);
#endif     
     break;
    default:
#if VIA6522_DEBUG
      log_warning("*** VIA6522 READ: >>> SHOULD NOT GET HERE <<< via_rreg(%02x,val)\n",  reg);
#endif
      break;
    }

  return val;
}

void via_wreg(uint8_t reg, uint8_t val) {
  int bc;
  int old_ier;

  switch(reg) {
    case 0x00: // PORT B
#if VIA6522_DEBUG
    log_debug("VIA write portb <- %02x (Page = %0X)", val, val & 0x03);
 
      bc = val ^ via.orb;
      log_debug("VIA write portb <- %02x Previous value: %02x. bc is %02x. (val & 0x10) = %x", val, via.orb, bc, val & 0x10);
      if (bc == 0) log_debug("      No change");
      if (bc & 0x01) log_debug("      Bank = %s \n",    (val & 0x01) ?   "0"    : "1");
      if (bc & 0x02) log_debug("      Half = %s \n",    (val & 0x02) ?  "Lower" : "Upper");
      if (bc & 0x04) log_debug("      Input = %s \n",   (val & 0x04) ?  "Line"  : "Mic");
      if (bc & 0x08) // if line (meaning playing)
          log_debug("****  %swheel selected\n", (val &0x04) ? "Mod" : "Pitch");
        if (bc & 0x10) log_debug("      FDC = %s \n",     (val & 0x10) ?  "Off"   : "On");          
     // if (bc & 0x20) log_debug("      DOC CA3 synchro = %s \n", (val & 0x20) ? "1" : "0");        // It does not make sense to report writing this value, it is driven by DOC 5503
     // if (bc & 0x40) log_debug("      Disc = %s \n",    (val & 0x40) ? "Loaded" : "Not Loaded");   // It does not make sense to report writing this value, it is written from external devices
#endif

//TODO: clear IFR flags according to documentation
      via.orb = val;
      break;
    case 0x01: // port A only used for keypad and display
#if VIA6522_DEBUG
       log_debug("*** VIA6522 >WRITE<: TODO Add Display emulation===========================\n");
#endif
       via.ora = val | 0xE0; // fake No Kyes are pressed
      break;
    case 0x02:
 #if VIA6522_DEBUG
      log_debug("VIA write ddrb <- %02x", val);  // writes 0x1F 0001_1111 during Boot so PB7, PB6 and PB5 are inputs  (PB7 is SERCLK, PB6 is DSTAT_ from floppy, PB5 is CA3 from DOC5503)
 #endif
      via.ddrb = val;
      break;
    case 0x03:
#if VIA6522_DEBUG  
      log_debug("VIA write ddra <- %02x", val);  // writes 0x1F 0001_1111 during Boot so PA7, PA6 and PA5 are inputs (ROW2, ROW1 and ROW0 for the keypads)
#endif
      via.ddra = val;
      break;
    case 0x04: // T1 Low-Order Latches 
#if VIA6522_DEBUG
      log_debug("***** VIA write T1 Low-Order Latches - UNHANDLED Register %02x <= %02x\n", reg, val);
#endif
    break;
    case 0x05: // T1 High-Order Counter     
#if VIA6522_DEBUG
      log_debug("***** VIA write T1 High-Order Counter - UNHANDLED Register %02x <= %02x\n", reg, val);
#endif
    break;
    case 0x06: //T1 Low-Order Latches    
#if VIA6522_DEBUG   
      log_debug("***** VIA write T1 Low-Order Latches - UNHANDLED Register %02x <= %02x\n", reg, val);
#endif
    break;
    case 0x07: //T1 High-Order Latches    
#if VIA6522_DEBUG   
      log_debug("***** VIA write T1 High-Order Latches  - UNHANDLED Register %02x <= %02x\n", reg, val);
#endif
      break;
    case 0x08: // T2 Low-Order Latches FOLLOWING CODE IS SUSPICIOUS: 0x08 register is for latches, not counter
      via.t2l = val;
      via_t2 = via.t2l | (via.t2h<<8);
      via_cycles = get_cpu_cycle_count() + (via_t2>>1);  // >>1 is half, because the clock frequency is 2MHz. JCP code was >>2, which is divide by 4
#if VIA6522_DEBUG  
      log_debug("VIA write reg8 t2 <- %04x\n", (int) via_t2);
#endif
      break;
    case 0x09: // T2 High-Order Counter
      via.t2h = val;
      via_t2 = via.t2l | (via.t2h<<8);
      via_cycles = get_cpu_cycle_count() + (via_t2>>2);  // half, because the clock frequency is 2MHz
      via.ifr = (via.ifr & 0x11011111); // Clear IFR5, which is Timer2
#if VIA6522_DEBUG
      log_debug("VIA write reg9 t2 <- %04x\n", (int) via_t2);
      log_debug("      WRITING IN T2H CLEARS THE INTERRUPT FLAG ****\n");
#endif
      break;
    case 0x0A:  // 0xE20A
#if VIA6522_DEBUG  
      log_debug("VIA write sr <- %02x ******* NOT EXPECTED\n",  val);
#endif
      via.sr = val;
      break;
    case 0x0B:  // 0xE20B
#if VIA6522_DEBUG  
//---------------------------------------------------------------------------------------------------------------
//NOTE: CB1 and CB2 are not connected, just like in the Ensoniq DMS-8
//      I think the SW sets it = 0xCC -> 11           0                   011                    0        0 
//                                                                        SHIFT REGISTER CTRL
//                                       Timer1       Timer 2             Shift                  PB       PA
//                                       Continuous   Timed Interrupt     Under control          Latch    Latch
//                                       Interrupt                        of external clock      Disable  Disable
//----------------------------------------------------------------------------------------------------------------
     log_debug("VIA write acr <- %02x\n", val);
     if( (val & 0x0C) == 0x0C) log_debug("VIA ACR - Shift Register Control CB2 Shift under control of External Clock CB1\n");
          else log_debug("VIA ACR - THIS MODE IS NOT SUPPORTED IN THiS MIRAGE MODEL (Shift Register control)\n");
     if( (val & 0xC0) == 0xC0) {
                      log_debug("**** VIA ACR - Free Running Mode. Output to PB7 is enabled\n");
                      log_debug("**** T1 Latches are CLR from the BOOT ROM: PB7 is toggling at 1MHz (half the speed of the VIA), this is the CLOCK of the UART\n");
            }  
          else log_debug("VIA ACR - THIS MODE IS NOT SUPPORTED IN THiS MIRAGE MODEL (PB7 Free Running Mode)\n");
#endif
      via.acr = val;
      break;
    case 0x0C:  // 0xE20C
      via.pcr = val;
#if VIA6522_DEBUG  
      log_debug("VIA write pcr <- %02x\n", val);
#endif
      break;
    case 0x0D:  // 0xE20D  IFR
      val &= 0x7f;     // mask is 0111_1111, IFR bit 7 can only be cleared when disabling the interrupt flag that generated it
      via.ifr &= ~val; // bitwise update of IFR with val
#if VIA6522_DEBUG  
      log_debug("VIA write IFR val <- %02x / ifr<=%02x\n", val, via.ifr);
      log_debug("      -> STUDY INTERRUPT FLAG REGISTER");
#endif
      break;
    case 0x0E:  // 0xE20E IER
      // okay, this is a funny one.  If bit 7 is set, the remaining bits
      // that are high are set high in IER.  If it is unset, the remaining
      // bits that are high are cleared.

      old_ier = via.ier;

      if (val & 0x80) via.ier |= (val & 0x7f);
        else via.ier &= (~val);
#if VIA6522_DEBUG  
      log_debug("VIA write IER val <- %02x ier=%02x\n", val, via.ier);
      log_debug("      old IER was %02x", old_ier);
      // one write shows val == a6 and ier = 26
      // val is 1010_0110, so (val & 0x80) => via.ier = via.ier | (val & 0x7f)
      //                                      via.ier = 0000_0000 | 0010_0110
      // which means it will return 26, we are enabling SHIFT REGISTER and CA1 interrupts 
      // in addition to T2 INTERRUPT
#endif
      break;
    case 0x0F:  // 0xE20F
#if VIA6522_DEBUG  
      log_debug("VIA write ORA (NO HNDSHKE) <- %02x\n",  val);
      log_debug("      Check for rotating 1s for segments/columns and anodes\n");
 //     log_debug("      TODO: Here is where to add Display & Keypad emulation\n");
#endif
      via.ora = val | 0xE0; // Fake no keys are pressed
      break;
    default:  
    log_warning("VIA write >>>STILL UNMANAGED<<< via_wreg(%d, 0x%02x)\n", reg, val);
    break;  
  }
}


/*
// port A: front panel
// bits 0-2: column select from 0-7
// bits 3/4 = right and left LED enable
// bits 5/6/7 keypad rows 0/1/2 return
via_keydisp()
{
  UINT8 seg = data & 7;
  static const int segconv[8] =
  {
    16, 8, 32, 2, 1, 64, 128, 4
  };

//  printf("PA: %02x (PC=%x)\n", data, m_maincpu->pc());

  // left LED selected?
  if ((data & 0x10) == 0x10)
  {
    // if the segment number is lower than last time, we've
    // started a new refresh cycle
    if ((seg < m_l_hi) || (seg == 0))
    {
      m_l_segs = segconv[seg];
    }
    else
    {
      m_l_segs |= segconv[seg];
    }

    m_l_hi = seg;
    output_set_digit_value(0, m_l_segs);
//      printf("L LED: seg %d (hi %d conv %02x, %02x)\n", seg, m_l_hi, segconv[seg], m_l_segs);
  }
  // right LED selected?
  if ((data & 0x08) == 0x08)
  {
    // if the segment number is lower than last time, we've
    // started a new refresh cycle
    if ((seg < m_r_hi) || (seg == 0))
    {
      m_r_segs = segconv[seg];
    }
    else
    {
      m_r_segs |= segconv[seg];
    }

    m_r_hi = seg;
    output_set_digit_value(1, m_r_segs);
//      printf("R LED: seg %d (hi %d conv %02x, %02x)\n", seg, m_r_hi, segconv[seg], m_r_segs);
  }
}
*/
