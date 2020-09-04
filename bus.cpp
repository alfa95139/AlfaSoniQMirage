
#include "bus.h"

#include "via.h"
#include "fdc.h"
#include <stdint.h>
#include <Arduino.h>

////////////////////////////////////////////////////////////////////
// Monitor Code
////////////////////////////////////////////////////////////////////
#include "EnsoniqRom.h"
#include "CartridgeROM.h"

////////////////////////////////////////////////////////////////////
// Debug Vectors
////////////////////////////////////////////////////////////////////
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

uint8_t WAV_RAM_0[WAV_END - WAV_START+1];
uint8_t PRG_RAM[RAM_END - RAM_START+1];
int page = 0;


void CPU6809::write(uint16_t address, uint8_t data) {
  // RAM?
  if ((RAM_START <= address) && (address <= RAM_END)) {
    PRG_RAM[address - RAM_START] = data;
    //Serial.printf("Writing to PRG_RAM, address = %04x : DATA = %02x\n", address, data);
    if(address == 0x800F) Serial.printf("************* WRITING OS ENTRY JMP 0x800F = %02X *********************\n", data);
    if(address == 0x8010) Serial.printf("*************                      0x8010 = %02X *********************\n", data);
    if(address == 0xBDEB) Serial.printf("************* WRITING TO 0xBDEB = %02X *********************\n", data);

  } else if ( (WAV_START <= address) && (address <= WAV_END) ) {
    // WAV RAM
    page = via_rreg(0) & 0b0011;
    WAV_RAM_0[address - WAV_START] = data;
    //Serial.printf("Writing to WAV RAM: PAGE %x address = %X, DATA = %0X\n", page, address, data);

  } else if ( (address & 0xFF00) == VIA6522) {
    Serial.printf("Writing to VIA 6522 %04x %02x\n", address, data);
    via_wreg(address & 0xFF, data);

  } else if ( (address & 0xFF00) == FDC1770) {
    Serial.printf("Writing to FDC 1770 %04x %02x\n", address, data);
    fdc_wreg(address & 0xFF, data);

  } else if ((address & 0xFF00) == DOC5503) {
    Serial.printf("Writing to DOC: Register %02X\n", address & 0x00FF);

  } else if ((address & 0xFF00) == 0xE100) {
    // FTDI?
    Serial.printf("Writing to ACIA (not implemented) %c\n", data);
    //Serial.write(data);
  }
}

uint8_t CPU6809::read(uint16_t address) {
  uint8_t out;

  // ROM?
  if ((ROM_START <= address) && (address <= ROM_END)) {
    out = ROM [ (address - ROM_START) ];

  } else if ((RAM_START <= address) && (address <= RAM_END)) {
    // RAM?
    if (address == loadopsys) Serial.printf("***   LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM *** LOAD OS IN PRG RAM   *** \n");
    if (address == osentry)   Serial.printf("***  OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY *** OS ENTRY  ***\n");
    if (address == irqentry)  Serial.printf("***  IRQ INTERRUPT ROUTINE ENTRY POINT ***  IRQ INTERRUPT ROUTINE ENTRY POINT ***   IRQ INTERRUPT ROUTINE ENTRY POINT  ***\n");
    if (address == firqentry) Serial.printf("*** FIRQ INTERRUPT ROUTINE ENTRY POINT *** FIRQ INTERRUPT ROUTINE ENTRY POINT ***  FIRQ INTERRUPT ROUTINE ENTRY POINT  ***\n");
    
    
    if (address == firqvec)      Serial.printf("**** firqvec     ****\n");
    if (address == osvec)       Serial.printf("**** osvec       ****\n"); 
    if (address == fdcreadsector)     Serial.printf("**** fdcreadsector   ****\n"); 
    if (address == fdcskipsector)     Serial.printf("**** fdcskipsector   ****\n"); 
    if (address == fdcwritesector)    Serial.printf("**** fdcwritesector  ****\n"); 
    if (address == fdcfillsector)     Serial.printf("**** fdcfillsector   ****\n"); 
    if (address == fdcreadtrack)    Serial.printf("**** fdcreadtrack  ****\n"); 
    if (address == fdcwritetrack)     Serial.printf("**** fdcwritetrack   ****\n"); 
    if (address == fdcrestore)    Serial.printf("**** fdcrestore    ****\n"); 
    if (address == fdcseektrack)    Serial.printf("**** fdcseektrack  ****\n"); 
    if (address == fdcseekin)     Serial.printf("**** fdcseekin     ****\n"); 
    if (address == fdcseekout)    Serial.printf("**** fdcseekout    ****\n"); 
    if (address == fdcforceinterrupt)   Serial.printf("**** fdcforceinterrupt   ****\n"); 
    if (address == countdown)     Serial.printf("**** countdown     ****\n"); 
    if (address == nmivec)      Serial.printf("**** nmivec    ****\n"); 
    if (address == coldstart)     Serial.printf("**** coldstart     ****\n"); 
    if (address == runopsys)    Serial.printf("**** runopsys    ****\n"); 
    if (address == hwsetup)     Serial.printf("**** hwsetup     ****\n"); 
    if (address == qchipsetup)    Serial.printf("**** qchipsetup    ****\n"); 
    if (address == clearram)    Serial.printf("**** clearram    ****\n"); 
    if (address == readsysparams)     Serial.printf("**** readysysparams  ****\n"); 
    if (address == checkos)     Serial.printf("**** checkos     ****\n"); 
    if (address == showerrcode)     Serial.printf("**** showerrorcode   ****\n"); 
    if (address == preparefd)     Serial.printf("**** preparefd     ****\n"); 
    if (address == loadossector)    Serial.printf("**** loadossector  ****\n"); 
    if (address == gototrack)     Serial.printf("**** gototrack     ****\n");
    if (address == seterrcode)    Serial.printf("**** seterrcode    ****\n"); 
    if (address == saveparams)    Serial.printf("**** saveparams    ****\n"); 
    if (address == restoreparams)     Serial.printf("**** restoreparams   ****\n"); 
    if (address == readsector)    Serial.printf("**** readsector    ****\n"); 
    if (address == writesector)     Serial.printf("**** writesector     ****\n"); 
    if (address == gototrack2)    Serial.printf("**** gototrack2    ****\n"); 
    if (address == enablefd)    Serial.printf("**** enablefd    ****\n");
    if (address == disablefd)     Serial.printf("**** disablefd     ****\n"); 
    if (address == osvec)  Serial.printf("****  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC %04x = %02x %02x\n", address, PRG_RAM[address - RAM_START+1], PRG_RAM[address - RAM_START +2] );
    //if (address == irqvec) Serial.printf("**** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC %04x = %04x\n", address, PRG_RAM[address - RAM_START]);
    out = PRG_RAM[address - RAM_START];
  } else if ((WAV_START <= address) && (address <= WAV_END)) {
    // WAVE RAM? NOTE: VIA 6522 PORT B contains info about the page, when we will be ready to manage 4 banks
    out = WAV_RAM_0[address - WAV_START];
    //Serial.printf("Reading from WAV RAM: address = %X, DATA = FF\n", address, out);

  } else if ((CART_START <= address) && (address <= CART_END)) {
    //out = CartROM[ (address - CART_START) ];
    out = 0xFF; // we will enable when everything (but DOC5503) is working, we will need working UART
    Serial.printf("Reading from Expansion Port: address = %X, DATA = FF\n", address, out);

  } else if ( (address & 0xFF00) == VIA6522) {
    // FTDI?
    //if ( (address & 0xFF00) == 0xE100) 
    //      out = FTDI_Read();
    //else
    Serial.printf("Reading from VIA 6522\n");
    out = via_rreg(address & 0xFF);

  } else if ( (address & 0xFF00) == FDC1770) {
    out = fdc_rreg(address & 0xFF); 
    //Serial.printf("**** Reading from FDC 1770. Value ====> out = %02x\n", out);

  } else if (( address & 0xFF00) == DOC5503 ) {
    Serial.printf("Reading from DOC 5503: Register %02X\n", address & 0x00FF);
    out = 0xFF;

  } else
    out = 0xFF;

  return out;
}

CPU6809::CPU6809()
{
  reset();
  clock_cycle_count = 0;
}

void CPU6809::tick()
{
  step();
  clock_cycle_count++;
}

void CPU6809::invalid(const char* message) {
  Serial.print("CPU error detected: ");
  if (message)
    Serial.println(message);
  else
    Serial.println("No message specified");
}

////////////////////////////////////////////////////////////////////
// Processor Control Loop
////////////////////////////////////////////////////////////////////

#if outputDEBUG
#define DELAY_FACTOR() delayMicroseconds(1000)
#else
#define DELAY_FACTOR_H() asm volatile("nop\nnop\nnop\nnop\n");
#define DELAY_FACTOR_L() asm volatile("nop\nnop\nnop\nnop\n");
#endif
