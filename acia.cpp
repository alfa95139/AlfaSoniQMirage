/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   acia.c -- emulation of 6850 ACIA
   Copyright (C) 2012 Gordon JC Pearce
   Adapted to TeensyDuino and Retroshield by Alessandro Fasan, Novemnber 2020

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


#include "arduino.h"
#include "acia.h"



unsigned long acia_cycles = ACIA_CLK;
extern unsigned long get_cpu_cycle_count();

/*
   The 6850 ACIA in the Mirage is mapped at $E100
   registers are:
   * $E100 control register / status register
   * $E101 transmit register / receive register
   GJCP implemented only CR7 RX interrupt and CR5 TX interrupt in the control
   and SR0 RD full, SR1 TX empty, and SR7 IRQ.
   (GJCP: "maybe for robustness testing I will implement a way to signal errors").
   RTS, CTS and DCD are not used, with the latter two held grounded - see schematics.
   The 6850 is clocked at 2Mhz.
   IRQ is directly connect to the 6809 FIRQ.
   IRQ is not excercised while booting the system ROM.
   It is excercised by OS 3.2. As of 11/29/20 the IRQ functionality is not tested.
*/



//INIT
void acia_init() {

	// tx register empty     // IRQ PE OVRN FE CTS DCD TDRE RDRF
  
	acia.sr = 0x02;          //  0   0   0   0  0    0   1   0
	
	return;
}

// *****************************************
// * IRQ - which will be connected to FIRQ
// *****************************************
uint8_t acia_irq() {
  if ((acia.sr & 0x80) == 0x80 ) Serial.printf("DONE FIRING A FIRQ!!!\n");
  return ((acia.sr & 0x80) == 0x80 );
}


//RUN
//*********************************************************
// * Purpose of acia_run is to generate the irq when needed
//*********************************************************
void acia_run(CPU6809* cpu) { 
        // call this every time around the loop
        int i;
        uint8_t buf;

       if (get_cpu_cycle_count() < acia_cycles) 
                return;                       // nothing to do yet
       
       acia_cycles = get_cpu_cycle_count() + ACIA_CLK;        // nudge timer

       // read a character?
       if (Serial.available()){
              buf = Serial.read();
              if (buf != 255) {         // when there is nothing this would read ff, or -1, or "@"
                acia.rdr = buf;           //SR  IRQ PE OVRN FE CTS DCD TDRE RDRF
                acia.sr |= 0x01;          //SR    0   0   0   0  0   0   0    1
                if (acia.cr & 0x80) {     //SR  IRQ PE OVRN FE CTS DCD TDRE RDRF
                        acia.sr |= 0x80;    //SR  1   0   0   0   0  0   0    0
                        if (acia_irq()) {
                                 cpu->firq();     // Irrelevant for Boot ROM, 
                                 }                // it is required for OS3.2 ( firq interrupt routine)
                        }    
                }
            }
        // got a character to send?
        
        if (!(acia.sr & 0x02)) { 
                          //Serial.printf("Do I GET HERE?  char= >%c< \n", acia.tdr);
                          Serial.write(acia.tdr);
                          acia.sr |= 0x02; // since I just txmitted, now TDRE is empty
                          if ((acia.cr & 0x60) == 0x20) { // checking for CR5 active, will set IRQ
                                                           // IRQ PE OVRN FE CTS DCD TDRE RDRF
                                   acia.sr |= 0x80;        //  1   0   0   0   0  0   0    0
                                   if (acia_irq()) {
                                        cpu->firq();       // Irrelevant for Boot ROM, 
                                        }                  // it is required for OS3.2 ( firq interrupt routine)
                                   }
                        }
}


uint8_t acia_rreg(uint8_t reg) {
	switch (reg & 0x01) {   // not fully mapped
		case ACIA_SR:
//  		Serial.printf(" ACIA_SR: %0x\n ", acia.sr);
			  return acia.sr;
        break;
		case ACIA_RDR:
//  		Serial.printf(" ACIA_RDR: %x\n", acia.rdr);     //     IRQ PE OVRN FE CTS DCD TDRE RDRF
			  acia.sr &= 0x7e;	           // clear IRQ, RDRF       0  1   1   1   1   1   1   0
			  return acia.rdr;
        break;
		default:
 			 Serial.printf("******************** ACIA: Unmapped Register ******************************\n");
			 return 0xff;
      break;
	}
	return 0xff;	// maybe the bus floats
}

void acia_wreg(uint8_t reg, uint8_t val) {
//Serial.printf(" ACIA_Wreg \n");  
	switch (reg & 0x01) {   // not fully mapped
		case ACIA_CR:
			acia.cr = val;
      if (acia.cr & 0x11) {
          Serial.printf("ACIA Master Reset\n");
          //acia.cr = 0;
          acia.sr = 0;
          acia.tdr = 0;
          acia.rdr = 0;
      }
//   	 Serial.printf("CR acia.cr = %02x\n", val); // 15 is: 0 Rx IRQ disabled 00 (Tx IRQ disabled) 101 (8 bit, no parity, 1 stop bit) 01 ( 16 division ratio)
			break;
		case ACIA_TDR:
			acia.tdr = val;
//    Serial.printf(" TDR acia.tdr = %c\n", val);   // IRQ PE OVRN FE CTS DCD TDRE RDRF
			acia.sr &= 0x7d;	       // clear IRQ, TDRE       0   1   1   1   1   1   0    1
			break;
     default:
       Serial.printf("******************** ACIA: Unmapped Register ******************************\n");
       return 0xff;
       break;
	}
}
