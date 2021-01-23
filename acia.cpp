/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   acia.c -- emulation of 6850 ACIA
   Copyright (C) 2012 Gordon JC Pearce
   Copyright (C) 2020 Alessandro Fasan
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
   The 6850 ACIA in the Mirage is mapped at $E100
   registers are:
   * $E100 control register / status register
   * $E101 transmit register / receive register
   GJCP implemented only CR7 RX interrupt and CR5 TX interrupt in the control
   and SR0 RD full, SR1 TX empty, and SR7 IRQ.
   (GJCP: "maybe for robustness testing I will implement a way to signal errors").
   RTS, CTS and DCD are not used, with the latter two held to ground - see schematics.
   
   The 6850 is clocked at 2Mhz by PB7 of the VIA 6522.
   IRQ is directly connect to the 6809 FIRQ.
   IRQ is not excercised while booting the system ROM.
   It is excercised by Mirage OS 3.2. As of 11/29/20 the IRQ functionality is not tested.
   
   NOTE: Without the correct connection (IRQ->FIRQ) and modeling of the clock, the system hangs between A10E and A113
         ------------------------------------------------------------------------------------------------------------   
 bit:     7     | 6 |    5     |   4  | 3  |    2    |    1     |    0     |
 CR:   Receive  |   |          |      |    |         |          |          |  
      Interrupt |   |          |      |    |         |          |          |
       Enable   |   |          |      |    |         |          |          |
        (rie)   |   |          |      |    |         |          |          |
 ---------------+---+----------+------+----+---------+----------+----------+
 SR:  Interrupt |   | Receiver |      |    |  Data   | Transmit |  Receive |
       Request  |   | Overrun  |      |    | Carrier | data reg | data reg |
                |   |          |      |    | Detect  |  Empty   |   Full   |
       (irq)    |   |  (ovr)   |      |    | (ndcd)  |  (tdre)  |  (rdrf)  |
  -------------------------------------------------------------------------+
   IRQ_ bit 7: 1'b1 -> IRQ_ LOW (INTERRUPT FIRED)
   IRQ_ bit 7: 1'b0 -> IRQ_ HIGH 
   The IRQ_ bit is cleared by a READ OPERATION to the Receive Data Register or a WRITE OPERATION to the Transmit Data Register
   MIDI: One start bit, eight data bits, and one stop bit result in a maximum transmission rate of 3125 bytes (characters) per second
*/

#include "Arduino.h"
#include "acia.h"
#include "log.h"

#define MIDISERIAL Serial1

unsigned long acia_cycles;
extern unsigned long get_cpu_cycle_count();

#define ACIA6850_DEBUG 1

struct {
  uint8_t cr;
  uint8_t sr;
  uint8_t tdr;
  uint8_t rdr;
} acia;

uint16_t acia_clk;
int m_tx_irq_enable, m_rx_irq_enable;


// *******************************************
// *************** ACIA_INIT *****************
// *******************************************
void acia_init() {

  // tx register empty        // IRQ PE OVRN FE CTS DCD TDRE RDRF
  acia.sr = SR_TDRE;          //  0   0   0   0  0    0   1   0
  
  acia.cr = 0x00;
  
  acia.tdr  = 0;
  acia.rdr  = 0;

  MIDISERIAL.begin(31250); // MIDI BAUD RATE
  
  return;
}

//*********************************************************
//******************** ACIA_RUN ***************************
//*********************************************************
void acia_run(CPU6809* cpu) { 

if ( (m_tx_irq_enable && ( acia.sr & SR_TDRE)) || (m_rx_irq_enable && (acia.sr & SR_RDRF)) )   {
        acia.sr |= SR_IRQ;
        cpu->firq();
        return;
}

if (get_cpu_cycle_count() < acia_cycles) return;  // nothing to do yet
acia_cycles = get_cpu_cycle_count() + ACIA_CLK;  // nudge timer

if (MIDISERIAL.available() > 0) {    
                acia.rdr = MIDISERIAL.read();
                acia.sr |= SR_RDRF;          // flag that data is ready
#if ACIA6850_DEBUG 
                log_debug("UART data received, but not read yet        %x\n", acia.rdr);
#endif
            }


if( !(acia.sr & SR_TDRE)) {
#if ACIA6850_DEBUG 
log_debug("***** UART: ACIA6850 - TX DATA  char= >%0x< (%c) \n", acia.tdr, acia.tdr);
#endif  
                MIDISERIAL.write(acia.tdr);  
                Serial.printf("MIDI OUT %0x\n", acia.tdr);
                acia.sr |= SR_TDRE;             // we just sent a data, so the queue is now empty
            }    

}
//***************** END acia_run(CPU6809* cpu) ******************

// **************************************************************
// ************ ACIA_RREG : READ SR (at $E100) and RDR (at $E101)
// **************************************************************
uint8_t acia_rreg(uint8_t reg){

switch (reg & 0x01) {   
  
    case ACIA_SR:   // $E100
#if ACIA6850_DEBUG 
log_debug("********************** READING ACIA_SR: %0x\n ", acia.sr);
#endif
    return acia.sr;
    break;
    
    case ACIA_RDR: // $E101
#if ACIA6850_DEBUG 
log_debug("UART data received and read                              %x\n", acia.rdr);     //  
#endif
        acia.sr &= 0x7e;  // clear IRQ, RDRF
        return acia.rdr;
        break;

  }
return -1;
}

// ****************************************************************
// ************ ACIA_WREG : WRITES CR (at $E100) and TDR (at $E101)
// ****************************************************************
void acia_wreg(uint8_t reg, uint8_t val) {

switch(reg & 0x01) {
    
case ACIA_CR:       // E100
      acia.cr = val;
      m_rx_irq_enable = (acia.cr >> 7) & 1;         // check for RX IRQ Enable Active
               //   CR5 & CR6  (tx2, tx1) == (0,1)
      m_tx_irq_enable = ((acia.cr & 0x60) == 0x20); // check for TX IRQ Enable Active
               //   CR0 & CR1 == (1,1) 
      if ( ( acia.cr & 0x03 ) == 0x03)              // Check for Master Reset
              acia_init();                          // Master Reset
#if ACIA6850_DEBUG
log_debug("*                  m_rx_irq_enable will %s trigger a RX request\n", (m_rx_irq_enable == 1) ? "" : "not");
log_debug("*                  m_tx_irq_enable will %s trigger a TX request\n", (m_tx_irq_enable == 1) ? "" : "not");
log_debug("****************** ACIA CR = %0x\n", acia.cr);
#endif
      break;
    
case ACIA_TDR:      // E101
      acia.tdr = val;
      acia.sr &= 0x7d;  // clear IRQ, TDRE
      break;
       

  }
}
