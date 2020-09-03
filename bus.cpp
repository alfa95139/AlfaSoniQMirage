
void writeRAM8(uint16_t address, uint8_t data) {
  
}

void writeRAM16(uint16_t address, uint16_t data) {
  
}

uint8_t readRAM8(uint16_t address) {
  
}

uint16_t readRAM16(uint16_t address) {
  
}

void writeIO8(uint16_t address, uint8_t data) {
  
}

uint8_t readIO8(uint16_t address) {
  
}


#include "cpu.h"

#include "via.h"
#include "fdc.h"
#include <stdint.h>
#include <Arduino.h>

////////////////////////////////////////////////////////////////////
// 6809 Processor Control
////////////////////////////////////////////////////////////////////
//
// #define GPIO?_PDOR    (*(volatile uint32_t *)0x400FF0C0) // Port Data Output Register
// #define GPIO?_PSOR    (*(volatile uint32_t *)0x400FF0C4) // Port Set Output Register
// #define GPIO?_PCOR    (*(volatile uint32_t *)0x400FF0C8) // Port Clear Output Register
// #define GPIO?_PTOR    (*(volatile uint32_t *)0x400FF0CC) // Port Toggle Output Register
// #define GPIO?_PDIR    (*(volatile uint32_t *)0x400FF0D0) // Port Data Input Register
// #define GPIO?_PDDR    (*(volatile uint32_t *)0x400FF0D4) // Port Data Direction Register

/* Digital Pin Assignments */
#define SET_DATA_OUT(D) (GPIOD_PDOR = (GPIOD_PDOR & 0xFFFFFF00) | (D))

#define xDATA_IN        ((uint8_t) (GPIOD_PDIR & 0xFF))
#define ADDR_H          ((uint16_t) (GPIOA_PDIR & 0b1111000000100000))
#define ADDR_L          ((uint16_t) (GPIOC_PDIR & 0b0000111111011111))
#define ADDR            ((uint16_t) (ADDR_H | ADDR_L))

// Teensy has an LED on its digital pin13 (PTC5). which interferes w/
// level shifters.  So we instead pick-up A5 from PTA5 port and use
// PTC5 for PG0 purposes.
//

#define MEGA_PD7  (24)
#define MEGA_PG0  (13)
#define MEGA_PG1  (16)
#define MEGA_PG2  (17)
#define MEGA_PB0  (28)
#define MEGA_PB1  (39)
#define MEGA_PB2  (29)
#define MEGA_PB3  (30)

#define uP_RESET_N  MEGA_PD7
#define uP_RW_N     MEGA_PG1
#define uP_FIRQ_N   MEGA_PG0
#define uP_GPIO     MEGA_PG2
#define uP_IRQ_N    MEGA_PB3
#define uP_NMI_N    MEGA_PB2
#define uP_E        MEGA_PB1
#define uP_Q        MEGA_PB0

// Fast routines to drive signals high/low; faster than digitalWrite
//
#define CLK_E_HIGH          (GPIOA_PSOR = 0x20000)
#define CLK_E_LOW           (GPIOA_PCOR = 0x20000)
#define CLK_Q_HIGH          (GPIOA_PSOR = 0x10000)
#define CLK_Q_LOW           (GPIOA_PCOR = 0x10000)
#define STATE_RW_N          ((uint8_t) (GPIOB_PDIR & 0x01) )

#define xDATA_DIR_IN()      (GPIOD_PDDR = (GPIOD_PDDR & 0xFFFFFF00))
#define xDATA_DIR_OUT()     (GPIOD_PDDR = (GPIOD_PDDR | 0x000000FF))



uint16_t      uP_ADDR;
uint8_t          uP_DATA;

int page;

#define fdccmd   0x8000
#define fdcrtry  0x8001
#define fdctrk   0x8002
#define fdcsect  0x8003
#define fdcbuff  0x8004
#define fdcstat  0x8006
#define fdcerr   0x8007
#define var1    0xbf70
#define var2    0xbf71
#define var3    0xbf72
#define var4    0xbf73
#define var5    0xbf74
#define var6    0xbf75
#define var7    0xbf76
#define var8    0xbf77
#define var9    0xbf78
#define var10   0xbf79
#define var11   0xbf7a
#define var12   0xbf7b
#define var13   0xbf7c
#define var14   0xbf7d
#define var15   0xbf7e
#define var16   0xbf7f
#define var17   0xbf80
#define var18   0xbf81
#define var19   0xbf82
#define var20   0xbf83
#define var21   0xbf84
#define var22   0xbf85
#define var23   0xbf8c

#define firqvec    0x800B
#define osvec     0x800E // location of osentry
#define irqentry    0x893C // IRQ service routine in OS 3.2
#define firqentry   0xA151 // FIRQ Service Routine
#define osentry   0xB920 // OS 3.2 OS Entry point
#define fdcreadsector   0xF000
#define fdcskipsector   0xF013
#define fdcwritesector  0xF024
#define fdcfillsector   0xF037
#define fdcreadtrack    0xF04A
#define fdcwritetrack   0xF058
#define fdcrestore    0xF066
#define fdcseektrack    0xF06F
#define fdcseekin   0xF07D
#define fdcseekout    0xF086
#define fdcforceinterrupt  0xF08F
#define countdown   0xF0A7
#define nmivec    0xF0B0
#define coldstart   0xF0F0
#define runopsys    0xF146
#define hwsetup   0xF15D
#define qchipsetup    0xF1BB
#define clearram    0xF1E5
#define loadopsys   0xF20D // load OS in PROGRAM RAM
#define readsysparams   0xF2AF
#define checkos   0xF306
#define showerrcode   0xF33C
#define preparefd   0xF38C
#define loadossector    0xF3AC
#define gototrack   0xF3F1
#define seterrcode    0xF413
#define saveparams    0xF425
#define restoreparams   0xF437
#define readsector    0xF448
#define writesector   0xF476
#define gototrack2    0xF4A4
#define enablefd    0xF4C6
#define disablefd   0xF4D6

CPU6809::CPU6809()
{
  reset();

  clock_cycle_count = 0;
}
  
void CPU6809::reset()
{
  // Not sure what to do here quite yet
}

////////////////////////////////////////////////////////////////////
// Processor Control Loop
////////////////////////////////////////////////////////////////////

volatile uint8_t DATA_OUT;
volatile uint8_t DATA_IN;

#if outputDEBUG
#define DELAY_FACTOR() delayMicroseconds(1000)
#else
#define DELAY_FACTOR_H() asm volatile("nop\nnop\nnop\nnop\n");
#define DELAY_FACTOR_L() asm volatile("nop\nnop\nnop\nnop\n");
#endif

inline __attribute__((always_inline))
void cpu_tick()
{    

  //////////////////////////////////////////////////////////////
  CLK_Q_HIGH;           // digitalWrite(uP_Q, HIGH);   // CLK_Q_HIGH;  // PORTB = PORTB | 0x01;     // Set Q = high
  DELAY_FACTOR_H();
  CLK_E_HIGH;           // digitalWrite(uP_E, HIGH);    // PORTB = PORTB | 0x02;     // Set E = high
  // DELAY_FACTOR_H();
  
  uP_ADDR = ADDR;

 

  CLK_Q_LOW;            // digitalWrite(uP_Q, LOW);    // CLK_Q_LOW; // PORTB = PORTB & 0xFE;     // Set Q = low
  // DELAY_FACTOR_L();

  if (STATE_RW_N)      // Check R/W
  //////////////////////////////////////////////////////////////
  // R/W = high = READ?
  {
    // DATA_DIR = DIR_OUT;
    xDATA_DIR_OUT();

    
    // ROM?
    if ( (ROM_START <= uP_ADDR) && (uP_ADDR <= ROM_END) )
      DATA_OUT = ROM [ (uP_ADDR - ROM_START) ];
    else 
    // RAM?
    if ( (RAM_START <= uP_ADDR) && (uP_ADDR <= RAM_END) ) {
           if (uP_ADDR == loadopsys) Serial.printf("***   LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM   *** \n");
  if (uP_ADDR == osentry)   Serial.printf("***  OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY  ***\n");
  if (uP_ADDR == irqentry)  Serial.printf("***  IRQ INTERRUPT ROUTINE ENTRY POINT ***  IRQ INTERRUPT ROUTINE ENTRY POINT ***   IRQ INTERRUPT ROUTINE ENTRY POINT  ***\n");
  if (uP_ADDR == firqentry) Serial.printf("*** FIRQ INTERRUPT ROUTINE ENTRY POINT *** FIRQ INTERRUPT ROUTINE ENTRY POINT ***  FIRQ INTERRUPT ROUTINE ENTRY POINT  ***\n");


if (uP_ADDR == firqvec)      Serial.printf("**** firqvec     ****\n");
if (uP_ADDR == osvec)       Serial.printf("**** osvec       ****\n"); 
if (uP_ADDR == fdcreadsector)     Serial.printf("**** fdcreadsector   ****\n"); 
if (uP_ADDR == fdcskipsector)     Serial.printf("**** fdcskipsector   ****\n"); 
if (uP_ADDR == fdcwritesector)    Serial.printf("**** fdcwritesector  ****\n"); 
if (uP_ADDR == fdcfillsector)     Serial.printf("**** fdcfillsector   ****\n"); 
if (uP_ADDR == fdcreadtrack)    Serial.printf("**** fdcreadtrack  ****\n"); 
if (uP_ADDR == fdcwritetrack)     Serial.printf("**** fdcwritetrack   ****\n"); 
if (uP_ADDR == fdcrestore)    Serial.printf("**** fdcrestore    ****\n"); 
if (uP_ADDR == fdcseektrack)    Serial.printf("**** fdcseektrack  ****\n"); 
if (uP_ADDR == fdcseekin)     Serial.printf("**** fdcseekin     ****\n"); 
if (uP_ADDR == fdcseekout)    Serial.printf("**** fdcseekout    ****\n"); 
if (uP_ADDR == fdcforceinterrupt)   Serial.printf("**** fdcforceinterrupt   ****\n"); 
if (uP_ADDR == countdown)     Serial.printf("**** countdown     ****\n"); 
if (uP_ADDR == nmivec)      Serial.printf("**** nmivec    ****\n"); 
if (uP_ADDR == coldstart)     Serial.printf("**** coldstart     ****\n"); 
if (uP_ADDR == runopsys)    Serial.printf("**** runopsys    ****\n"); 
if (uP_ADDR == hwsetup)     Serial.printf("**** hwsetup     ****\n"); 
if (uP_ADDR == qchipsetup)    Serial.printf("**** qchipsetup    ****\n"); 
if (uP_ADDR == clearram)    Serial.printf("**** clearram    ****\n"); 
if (uP_ADDR == readsysparams)     Serial.printf("**** readysysparams  ****\n"); 
if (uP_ADDR == checkos)     Serial.printf("**** checkos     ****\n"); 
if (uP_ADDR == showerrcode)     Serial.printf("**** showerrorcode   ****\n"); 
if (uP_ADDR == preparefd)     Serial.printf("**** preparefd     ****\n"); 
if (uP_ADDR == loadossector)    Serial.printf("**** loadossector  ****\n"); 
if (uP_ADDR == gototrack)     Serial.printf("**** gototrack     ****\n");
if (uP_ADDR == seterrcode)    Serial.printf("**** seterrcode    ****\n"); 
if (uP_ADDR == saveparams)    Serial.printf("**** saveparams    ****\n"); 
if (uP_ADDR == restoreparams)     Serial.printf("**** restoreparams   ****\n"); 
if (uP_ADDR == readsector)    Serial.printf("**** readsector    ****\n"); 
if (uP_ADDR == writesector)     Serial.printf("**** writesector     ****\n"); 
if (uP_ADDR == gototrack2)    Serial.printf("**** gototrack2    ****\n"); 
if (uP_ADDR == enablefd)    Serial.printf("**** enablefd    ****\n");
if (uP_ADDR == disablefd)     Serial.printf("**** disablefd     ****\n"); 
          if (uP_ADDR == osvec)  Serial.printf("****  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC %04x = %02x %02x\n", uP_ADDR, PRG_RAM[uP_ADDR - RAM_START+1], PRG_RAM[uP_ADDR - RAM_START +2] );
          //if (uP_ADDR == irqvec) Serial.printf("**** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC %04x = %04x\n", uP_ADDR, PRG_RAM[uP_ADDR - RAM_START]);
          DATA_OUT = PRG_RAM[uP_ADDR - RAM_START];
          }
    else  // WAVE RAM? NOTE: VIA 6522 PORT B contains info about the page, when we will be ready to manage 4 banks
    if ( (WAV_START <= uP_ADDR) && (uP_ADDR <= WAV_END) ) {
      DATA_OUT = WAV_RAM_0[uP_ADDR - WAV_START];
      //Serial.printf("Reading from WAV RAM: uP_ADDR = %X, DATA = FF\n", uP_ADDR, DATA_OUT);
      }
    else
    if (( CART_START <= uP_ADDR) && (uP_ADDR <= CART_END) ) {
       //DATA_OUT = CartROM[ (uP_ADDR - CART_START) ];
        DATA_OUT = 0xFF; // we will enable when everything (but DOC5503) is working, we will need working UART
        Serial.printf("Reading from Expansion Port: uP_ADDR = %X, DATA = FF\n", uP_ADDR, DATA_OUT);
    }
    else
    // FTDI?
    //if ( (uP_ADDR & 0xFF00) == 0xE100) 
    //      DATA_OUT = FTDI_Read();
    //else
    if ( (uP_ADDR & 0xFF00) == VIA6522) {
                Serial.printf("Reading from VIA 6522\n");
                DATA_OUT = via_rreg(uP_ADDR & 0xFF);  
            }                   
   
    else
    if ( (uP_ADDR & 0xFF00) == FDC1770) {
           DATA_OUT = fdc_rreg(uP_ADDR & 0xFF); 
           //Serial.printf("**** Reading from FDC 1770. Value ====> DATA_OUT = %02x\n", DATA_OUT);
        }
   
    else 
    if (( uP_ADDR & 0xFF00) == DOC5503 ) {
        Serial.printf("Reading from DOC 5503: Register %02X\n", uP_ADDR & 0x00FF);
        DATA_OUT = 0xFF;
        }
    
    else
      DATA_OUT = 0xFF;

    // Start driving the databus out
    SET_DATA_OUT( DATA_OUT );
    
#if outputDEBUG
    char tmp[40];
    sprintf(tmp, "-- A=%0.4X D=%0.2X\n", uP_ADDR, DATA_OUT);
    Serial.write(tmp);
#endif

  } 
  else 
  //////////////////////////////////////////////////////////////
  // R/W = low = WRITE?
  {
    DATA_IN = xDATA_IN;
    
    // RAM?
    if ( (RAM_START <= uP_ADDR) && (uP_ADDR <= RAM_END) ) {
          PRG_RAM[uP_ADDR - RAM_START] = DATA_IN;
          //Serial.printf("Writing to PRG_RAM, uP_ADDR = %04x : DATA = %02x\n", uP_ADDR, DATA_IN);
          if(uP_ADDR == 0x800F) Serial.printf("************* WRITING OS ENTRY JMP 0x800F = %02X *********************\n", DATA_IN);
          if(uP_ADDR == 0x8010) Serial.printf("*************                      0x8010 = %02X *********************\n", DATA_IN);
          if(uP_ADDR == 0xBDEB) Serial.printf("************* WRITING TO 0xBDEB = %02X *********************\n", DATA_IN);

          }
    else // WAV RAM
    if ( (WAV_START <= uP_ADDR) && (uP_ADDR <= WAV_END) ) {
       page = via_rreg(0) & 0b0011;
       WAV_RAM_0[uP_ADDR - WAV_START] = DATA_IN;
       //Serial.printf("Writing to WAV RAM: PAGE %x uP_ADDR = %X, DATA = %0X\n", page, uP_ADDR, DATA_IN);
    }
     else
    if ( (uP_ADDR & 0xFF00) == VIA6522) {
          Serial.printf("Writing to VIA 6522 %04x %02x\n", uP_ADDR, DATA_IN);
          via_wreg(uP_ADDR & 0xFF, DATA_IN);     
          }
    else
    if ( (uP_ADDR & 0xFF00) == FDC1770) {
          Serial.printf("Writing to FDC 1770 %04x %02x\n", uP_ADDR, DATA_IN);
          fdc_wreg(uP_ADDR & 0xFF, DATA_IN);
         }
    else 
      if ( (uP_ADDR & 0xFF00) == DOC5503 ) Serial.printf("Writing to DOC: Register %02X\n", uP_ADDR & 0x00FF);
  else
    // FTDI?
    if ( (uP_ADDR & 0xFF00) == 0xE100) {
          Serial.printf("Writing to ACIA (not implemented) %c\n", DATA_IN);
          //Serial.write(DATA_IN);
          }

#if outputDEBUG
    char tmp[40];
    sprintf(tmp, "WR A=%0.4X D=%0.2X\n", uP_ADDR, DATA_IN);
    Serial.write(tmp);
#endif
  }

  //////////////////////////////////////////////////////////////
  // cycle complete
  
  CLK_E_LOW;    // digitalWrite(uP_E, LOW);      // CLK_E_LOW; // PORTB = PORTB & 0xFC;     // Set E = low
  
  // natural delay for data hold
  xDATA_DIR_IN();

  // DELAY_FACTOR_L();

//#if (outputDEBUG)  
  clock_cycle_count ++;
//#endif
}
