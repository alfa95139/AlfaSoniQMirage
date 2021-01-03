
#include "bus.h"

#include "via.h"
#include "fdc.h"
#include "doc5503.h" //AF: Added: 11/1/2020
#include "acia.h"    //AF: Added: 11/28/2020
#include "log.h"
#include "os.h"
#include <stdint.h>
#include <Arduino.h>

#define RomMonEnable  0 //AF: Added 11/28/2020. Set to 0 to disable Ensoniq Monitor ROM
// Format Examples
// Q              : Quit
// DC100-C1FF     : Display from C100 to C1FF
// Mxxxx          : Display content of location xxxx. "dd" will change the data, "space" will step forward, "return" will exit back to the monitor
// Jxxxx          : Jump to location xxxx and run program. NOTE: SOMETHING WRONG HAPPEN WHEN I TRY JC100, which is the Jump to ROM Monitor start
// L              : Load memory data in Motorola S-record format
// Pxxxxyyyy      : Dump memory from xxxx to yyyy as S-records
// Bxxxxyyyy      : Dump memory from xxxx to yyyy as raw binary data preceded by a "$" marker


// Mirage Hardware Memory Map
//0000-7FFF Bank of Wavetable RAM. There are four banks of 32K selected by lines on the Port 8 output of the VIA. This RAM appears to the Q-chip's DMA as two 64K banks.
//8000-BFFF Operating system RAM. Where OS is loaded.
//C000-DFFF Expansion slot space. Where Input Sampling Filter or Sequencer Expansion RAM appear.
//B800-BFFF Where short sequences are loaded.
//B800-DFFF Where long sequences are loaded.
//E100  MIDI interface UART control/status
//E101  MIDI interface UART data
//E200-E20F Versatile Interface Adapter (VIA)
//E408-E41F VCF parameters for Osc. 1-8
//E408-E40F Filter cut-off frequency
//E410-E417 Filter Resonance
//E418-E41F Multiplexer address pre-set (no output)
//E800-E803 Floppy disk controller
//EC00-ECEF Ensoniq Q-chip registers
//F000-FFFF Bootstrap ROM (contains disk I/O, pitch and controller parameter tables and Q-chip drivers)


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

#define firqvec         0x800B
#define osvec           0x800E // location of osentry
#define irqentry        0x893C // IRQ service routine in OS 3.2
#define manageKeys      0x8989 // I assume this is managing the request from the 6511
#define entersDOCirq    0x8962 // called within IRQentry, part of the interrupts polling
#define DOCirqService   0x8871 // this is the routine that services the DOC IRQ
#define firqentry       0xA151 // FIRQ Service Routine
#define osentry         0xB920 // OS 3.2 OS Entry point
#define AF_1            0xA758 // after first call for FDC from OS - case 1
#define AF_2            0xA78A // after first call for FDC from OS - case 2
#define AF_3            0xA7D0 // Another readsector
#define AF_4            0xA7D5 // After Readsector - case 1
#define AF_5            0xA7D7 // After Readsector - case 2
#define firstOSjmp      0x875E // this is the first jump from OS 3.2 Entry Point. It contains the loop to reset the filters and other houskeeping
#define AF_6            0xA7EB // Sector Counter - check for A (up to 5)
#define Dec162020N2     0x9858
#define Dec162020       0x9A4D
#define DisplayParam    0x9CC1
#define DisplayNKeys4  0x9DD7
#define DisplayNKeys3  0x9DE2
#define B4DispNKeys     0x9E00
#define DisplayNKeys1   0x9E14
#define DisplayNKeys2   0x9812 //
#define AF_7            0xA809 // JSR   fdcseekin
#define AF_8            0xA80C // After fdcseekin 
#define AF_9            0x91F4 // 122820 NMI crash
#define AF_10           0x9F6F // before NMI crash
#define AF_11           0x9F74 // /---
#define AF_12           0x9F77 // 
#define AF_13           0x9F7A  // 
#define AF_14           0x9F7D // 
#define AF_15           0x9F80 // ---/
#define AF_16           0xA4D7 //122920
#define AF_17           0xAADE // loop for gototrack2
#define AF_18           0x90E9 // Keyval Interpreter
#define AF_19           0xAAFC // 122920
#define AF_20           0xAAFF // 122920
#define DualUARTrst     0xA07E // before hang
#define UART_OUT        0xA1BC //
#define AnotherUART     0xA13D // We analyzed this well...
#define tunefiltpitchw  0xB96E // this is the second routine from OS 3.2 Entry Point
#define keypadscan      0x896B // seems to be keypad scanner
#define somehousekeep   0xB900 // don't know yet 
#define docctrlregs     0x8844 // Write ctrl registers a0, 1a, ext w value 03
#define writeaciasr     0xA13D // subroutine that ends with loading $B5 (1011_0101) in the ACIA Status Register
#define readfdinOS      0xA736 // trying to determine where the OS is loading the WAV data
#define fdcreadsector   0xF000
#define fdcskipsector   0xF013
#define fdcwritesector  0xF024
#define fdcfillsector   0xF037
#define fdcreadtrack    0xF04A
#define fdcwritetrack   0xF058
#define fdcrestore      0xF066
#define fdcseektrack    0xF06F
#define fdcseekin     0xF07D
#define fdcseekout    0xF086
#define fdcforceinterrupt  0xF08F
#define countdown     0xF0A7
#define nmivec        0xF0B0
#define coldstart     0xF0F0
#define runopsys      0xF146
#define hwsetup       0xF15D
#define qchipsetup    0xF1BB
#define clearram      0xF1E5
#define loadopsys     0xF20D // load OS in PROGRAM RAM
#define readsysparams   0xF2AF
#define checkos         0xF306
#define showerrcode     0xF33C
#define preparefd       0xF38C
#define loadossector    0xF3AC
#define gototrack       0xF3F1
#define seterrcode      0xF413
#define saveparams      0xF425
#define restoreparams   0xF437
#define readsector    0xF448
#define writesector   0xF476
#define gototrack2    0xF4A4
#define enablefd      0xF4C6
#define disablefd     0xF4D6

#define cmpwA2D_A0E2    0x90B0 // compare with A2D before jumpint to A0E2
#define call2_A0E2      0x90B9 //
#define another_A0E2    0xAD82 // another call to A0E2 (where we get stuck) - not invoked prior to stuck
#define monitorROM      0xC113     // after completing hwsetup
#define monitorMainLoop 0xC12A // This is where it launches printstring, etc
#define monitorPrintStr 0xC09D // This is the print of "Monitor Rom"
#define monitorsendch1  0xC0AA // This is the send char of "Monitor Rom"
#define somediskstuff   0xA4BA //


uint8_t WAV_RAM[4][WAV_END - WAV_START+1]; // AF: 11/2/2020 -> Very nice, thank you Dylan!

uint8_t PRG_RAM[RAM_END - RAM_START+1];     //

uint8_t FILTERS[0xE41F - 0xE400 + 1];           // filters

int page = 0;

const char* address_name(uint16_t address) {
  if (address == monitorROM)          return "MONITOR ROM";
  if (address == monitorMainLoop)     return "MONITOR ROM Main Loop";
  if (address == monitorPrintStr)     return "MONITOR ROM printstring Routine";
  if (address == monitorsendch1)      return "MONITOR ROM sendch1 Routine";
  if (address == loadopsys)           return "LOAD OS IN PRG RAM";
  if (address == osentry)             return "*OS ENTRY";
  if (address == irqentry)            return "IRQ INTERRUPT ROUTINE ENTRY POINT";
  if (address == manageKeys)          return "*Manage Keys - assuming 6511 or UART????"; 
  if (address == entersDOCirq)        return "Check whether the DOC has generated the interrupt";
  if (address == DOCirqService)       return "*DOC IRQ Service Routine";
  if (address ==  DualUARTrst)        return "Dual UART RST";
  if (address ==  UART_OUT)           return "UART OUT";
  if (address ==  AnotherUART)        return "ANOTHER UART";
  
  if (address == firqentry)           return "FIRQ INTERRUPT ROUTINE ENTRY POINT";
  if (address == firqvec)             return "firqvec";
  //if (address == irqvec)            return "irqvec";
  if (address == osvec)               return "osvec"; 
  if (address ==readfdinOS)           return "readfdinOS -> This is one of the floppy disk reads from within the OS";
  if (address == fdcreadsector)       return "fdcreadsector"; 
  if (address == fdcskipsector)       return "fdcskipsector"; 
  if (address == fdcwritesector)      return "fdcwritesector"; 
  if (address == fdcfillsector)       return "fdcfillsector"; 
  if (address == fdcreadtrack)        return "fdcreadtrack"; 
  if (address == fdcwritetrack)       return "fdcwritetrack"; 
  if (address == fdcrestore)          return "fdcrestore"; 
  if (address == fdcseektrack)        return "fdcseektrack"; 
  if (address == fdcseekin)           return "fdcseekin"; 
  if (address == fdcseekout)          return "fdcseekout"; 
  if (address == fdcforceinterrupt)   return "fdcforceinterrupt"; 
  if (address == countdown)           return "countdown"; 
  if (address == nmivec)              return "nmivec"; 
  if (address == coldstart)           return "coldstart"; 
  if (address == runopsys)            return "runopsys"; 
   if (address == hwsetup)             return "HW SETUP ROUTINE: IN MONITOR ROM";
  //if (address == hwsetup)             return "hwsetup"; 
  if (address == qchipsetup)          return "qchipsetup"; 
  if (address == clearram)            return "clearram"; 
  if (address == readsysparams)       return "readysysparams"; 
  if (address == checkos)             return "checkos"; 
  if (address == showerrcode)         return "showerrorcode"; 
  if (address == preparefd)           return "preparefd"; 
  if (address == loadossector)        return "loadossector"; 

  if (address == 0xF23C)              return "Set Track & Sector # before loadossector"; 
  if (address == 0xF253)              return "AFTER LOADOSSECTOR - Check $8006 flag";
  if (address == 0xF3B1)              return " BEFORE FDCREADSECTOR: check for X";
  if (address == 0xF3B7)              return " AFTER FDCREADSECTOR:  check for X";
  if (address == readsector)          return " Entering Readsector";
  if (address == 0xF450)              return " In readector, just before JSR FDCREADESCTOR";
  if (address == 0xF453)              return " AFTER JSR FDCREADSECTOR FROM READSECTOR ROUTINE";
  //if (address == 0xAADE)              return "* Only once in MAME";
  if (address == 0xA74E)              return " AFTER JSR READSECTOR";
  if (address == 0xA77A)              return " here starts copying data from WAV to B31D and onward";
  if (address == 0xA787)              return " AFTER looping from X=B31D to X=B58E: did it copy a bunch of WAV values here?";
  if (address == 0xA7D0)              return " This is the Readsector from OS";
  if (address == 0xA7D3)              return " This is after the Readsector from OS - CHECK 0x8000 values";
  if (address == 0xA7A4)              return " STX fdcbuff: what is X? Who is changing it?";
  if (address == 0xA809)              return " Breakpoint where 0x80D7 is read";
  if (address == 0x80D5)              return " Breakpoint where loops through 5 READSECTORS - check values of $80D5, $80D6, $80D7, $80D8";
  if (address == 0xA80C)              return " After FDCSEEKIN ****** START LOOOKING AT WHO FUCKS UP fdcsect ($800 ";
  if (address == 0xA748)              return " just started the loop - check for X";
  if (address == gototrack)           return "gototrack";
  if (address == seterrcode)          return "seterrcode"; 
  if (address == saveparams)          return "saveparams"; 
  if (address == restoreparams)       return "restoreparams"; 
  if (address == writesector)         return "writesector"; 
  if (address == gototrack2)          return "gototrack2"; 
  if (address == enablefd)            return "enablefd";
  if (address == disablefd)           return "disablefd";
  if (address == docctrlregs)         return "docctrlregs";

  if (address == 0x910D)              return " only this";
  if (address == 0x8F10)              return " CHECK - $8F10 RTS - CHECK";
  if (address == 0x8EEC)              return " Part where it writes in PRGM memory starting at 0x8e71 and onward";
  if (address == 0x8EF1)              return " starting point of the loop";
  if (address == 0x8EEB)              return " CHECK - $8EEB RTS - CHECK";
  
  if (address == 0x9234)              return " Before JSR $8ECF";
  if (address == 0x9237)              return " After JSR $8ECF";
  if (address == 0x9239)              return " After BSR $9249";
  if (address == 0x9F91)              return " 1/1/2021 BEFORE last NMI crash";
/*
    if (address == AF_1)                return "After reading sector: case 1";
    if (address == AF_2)                return "After reading sector: case 2";
  
    if (address == AF_3)                return " Readsector";
    if (address == AF_4)                return " After Readsector - case 1";
    if (address == AF_5)                return " After Readsector - case 2";
 
  if (address == AF_6)                return " Sector Counter - check for A (up to 5)";
  if (address == AF_7)                return " AT: JSR   fdcseekin";
  if (address == AF_8)                return " AFTER     JSR fdcseekin: How Many NMIs????";
  
  if (address == AF_9)                return " 122820 NMI crash";
  
  if (address == AF_10)               return "* before NMI crash";
  
  if (address == AF_11)               return "* **** 1 after JD=S unknown3 1";
  if (address == AF_12)               return "* **** 2 after JDS unknown2 2";
  
  if (address == AF_13)               return "* **** 3 after JSR $875E";
  if (address == AF_14)               return "* **** 4 after JSR $896B";
  if (address == 0xA07E)              return "* Entering 0xA07E";
  if (address == 0x91F9)              return "* Before JSR $9249 ";
  if (address == 0x91FC)              return "* Before JSR $9431 ";
  if (address == 0x91FF)              return "* RTS (after JSR $941) ";
  if (address == AF_15)               return "* **** 5 after JSR $A07E";
  if (address == 0x9200)              return " Getting closer";

//  if (address == 0x8EEC)              return " In a Few Lines";
//  if (address == 0x8EF5)              return " STB A,U <<< CAUSING PROBLEMS";
  if(address == 0x9239)               return "* After BSR $9249";
 */ 
  if (address == AF_16)               return " **** 16";
  if (address == AF_19)               return " @ JSR   gototrack2";
  if (address == AF_20)               return " @ RTI";
  
  if (address == AF_17)               return " LOOP FOR GOTOTRACK 2. Check values of $80D5, $80D6, $80D7";
  /*
  if (address == AF_18)               return "*Keyval Interpreter<<<<";

  if (address == DisplayNKeys1)       return "*DISPLAY AND KEYPAD 1";
  if (address == DisplayNKeys2)       return "*DISPLAY AND KEYPAD 2";
  if (address ==Dec162020N2 )         return "*Dec162020N2";
  if (address == Dec162020)           return "*Dec162020";
  if (address == DisplayParam)        return "*DisplayParam <<<< DisplayParam >>> DisplayParam";
  if (address == DisplayNKeys4)       return "* BEFORE DisplayNKeys 4";
  if (address == DisplayNKeys3)        return "* BEFORE DisplayNKeys 3";
  if (address == B4DispNKeys)          return "* Before DisplayNKeys";
  */

  if (address == firstOSjmp)          return "firstOSjmp";
  if (address == tunefiltpitchw )     return "tunefiltpitchw";
  if (address == keypadscan)          return "keypadscan";
  if (address == somehousekeep)       return "somehousekeep";
  if (address == writeaciasr)         return "writeaciasr";

  //if (address == )         return "*c";

  if ((address & 0xFF00) == 0x7f00)     return "cpucrash";

  if ((address & 0xFF00) < 0x8000)     return "cpucrash";

  if ((WAV_START <= address) && (address <= WAV_END)) return "wav data section";

  if ((address & 0xFF00) == VIA6522) return "VIA6522";
  if ((address & 0xFF00) == FDC1770) return "FDC1770";
  if ((address & 0xFF00) == DOC5503) return "DOC5503";
  if ((address & 0xFF00) == UART6850)  return "ACIA";

  return "?";
}

extern bool debug_mode;
extern bool do_continue;

void CPU6809::write(uint16_t address, uint8_t data) {
  set_log("bus");
  /*if (data == 0x7f && address > WAV_END && address < DEVICES_START) {
    log_debug("Wrote 0x%02x to %04x.\n", data, address);
    do_continue = false;
    debug_mode = true;
    set_debug(true);
  }*/
//if(address == 0x80D5) log_debug(">>>>> 80D5 written with value %X <<<<<", data);
//if(address == 0x80D6) log_debug(">>>>> 80D6 written with value %X <<<<<", data);
//if(address == 0x80D7) log_debug(">>>>> 80D7 written with value %X <<<<<", data);
//if(address == 0x80D8) log_debug(">>>>> 80D8 written with value %X <<<<<", data);

//if(address == 0x8000) log_debug(">>>>> 8000 FCD CMD    written with value %X <<<<<", data);
//if(address == 0x8001) log_debug(">>>>> 8001 FCD RETRY  written with value %X <<<<<", data);
//if(address == 0x8002) log_debug(">>>>> 8002 FCD TRK    written with value %X <<<<<", data);
//if(address == 0x8003) log_debug(">>>>> 8003 FCD SECT   written with value %X <<<<<", data);
//if(address == 0x8004) log_debug(">>>>> 8006 FCD BUFF   written with value %X", data);
//if(address == 0x8005) log_debug(" %X\n", data);
//if(address == 0x8006) log_debug(">>>>> 8006 FCD STATUS written with value %X <<<<<", data);

  // RAM?
  if ((RAM_START <= address) && (address <= RAM_END)) {                     //  0x8000 to 0xBFFF
    PRG_RAM[address - RAM_START] = data;  

  // AF 1/1/2020 We will need to remove this: it is not accurate (the OS RAM has areas for writing data: see for instance 8E71-8ECE) and it is very OS Dependent.
  //  if  (address < 0x9000 && address >= 0x828C) {  
  //    if (os_img_start[address - RAM_START] != data) {
  //      log_emergency("Attempted to write OS RAM at 0x%04x with data 0x%02x, which does not match expected 0x%02x", address, data, os_img_start[address - RAM_START]);
  //    }
  //  }
    
    //log_debug("********************* Writing to PRG_RAM, address = %04x : DATA = %02x\n", address, data);
    /*if(address == 0x800F) log_debug("************* WRITING OS ENTRY JMP 0x800F = %02X *********************\n", data);
    if(address == 0x8010) log_debug("*************                      0x8010 = %02X *********************\n", data);
    if(address == 0xBDEB) log_debug("************* WRITING TO 0xBDEB = %02X *********************\n", data);
*/
  } else if ( (WAV_START <= address) && (address <= WAV_END) ) {              // 0x0000 to 0xFFFF + Page
    // WAV RAM
    set_log("waveram");
    page = via_rreg(0) & 0b0011;
    WAV_RAM[page][address - WAV_START] = data;
//    log_debug("Writing to WAV RAM: PAGE %hhu address = %04x, DATA = %02x\n", page, address, data);
  } else if ( (address & 0xFF00) == VIA6522) {
    set_log("via");
    via_wreg(address & 0xFF, data);
  } else if ( (address & 0xFF00) == FDC1770) {
    set_log("fdc");
    fdc_wreg(address & 0xFF, data);
  } else if ((address & 0xFF00) ==  DOC5503) {
    set_log("doc5503");                       // AF: Added 11.1.2020
    doc_wreg(address & 0xFF, data);           // AF: Added 11.1.2020
  } else if ((address & 0xFF00) == UART6850) { // AF: Added 11.28.20 
    set_log("acia6850");
    //Serial.printf("UART6850: Writing Register %04x %04x\n", address & 0xFF, data);
    acia_wreg(address & 0xFF, data); 
  } else if ((address & 0xFF00) == 0xE400) {
    FILTERS[address - 0xE400] = data; 
#ifdef BUS_DEBUG
    log_debug("* Write Filters    [%04x] <- %02x\n", address, data);  
    log_debug("*            Filter[E4%0x]  <- %02x\n", address-0xE400, FILTERS[address - 0xE400]);
#endif 
  }
  set_log("cpu");
}

uint8_t CPU6809::read(uint16_t address) {
  uint8_t out;

  set_log("bus");

#ifdef BUS_DEBUG
if ((DEVICES_START <= address) &  (address <= DEVICES_END)) {
    log_debug("READING FROM DEVICE %04X", address);
    if ((address & 0xFF00) == 0xE400) 
      log_debug("REDING FROM FILTERS!!!!!!!!!!!!!!!!**************");
}
#endif 

  // ROM?
  if ((ROM_START <= address) && (address <= ROM_END)) {
    out = ROM [ (address - ROM_START) ];

  } else if ((RAM_START <= address) && (address <= RAM_END)) {
    // RAM?

    out = PRG_RAM[address - RAM_START];
  } else if ((WAV_START <= address) && (address <= WAV_END)) {
    // WAVE RAM? NOTE: VIA 6522 PORT B contains info about the page, when we will be ready to manage 4 banks
    set_log("wavram");
    page = via_rreg(0) & 0b0011;
    out = WAV_RAM[page][address - WAV_START];
    //log_debug("READING FROM WAV RAM: PAGE %hhu address = %04x, DATA = %02x\n", page, address, out);
 } else if ((CART_START <= address) && (address <= CART_END)) { // Will load the Monitor first. The command "Q" will Quit the monitor and continue the regular boot
    set_log("rom_cart");
#if RomMonEnable
    out = CartROM[ (address - CART_START) ];
#else
   out = 0xFF;
#endif
   // log_debug("READING FROM ROM CART:  address  = %04x, DATA = %02x\n", address, out);
  } else if ((address & 0xFF00) == VIA6522) {
    set_log("via");
    out = via_rreg(address & 0xFF);
  } else if ((address & 0xFF00) == FDC1770) {
    set_log("fdc");
    if (address  == 0xE800) log_debug(" READING from FDC REGISTER %0x", address & 0xFF);
    out = fdc_rreg(address & 0xFF);
  } else if ((address & 0xFF00) == DOC5503 ) {
    set_log("doc5503");                       // AF: Added 11.1.2020
    out = doc_rreg(address & 0xFF);           // AF: Added 11.1.2020
    //out = 0xFF;
  } else if ((address & 0xFF00) == UART6850) { //AF: Added 11.28.20 
    set_log("acia6850");  
    out = acia_rreg(address & 0xFF);
#if RomMonEnable
    log_debug("UART6850: Reading Register %04x %04x\n", address, out);
#endif
  } else if ((address & 0xFF00) == 0xE400) {
    set_log("filters"); 
    out = FILTERS[address - 0xE400]; 
#ifdef BUS_DEBUG    
    log_debug("read  filters    [E4%02x] -> %02x\n", address-0xE400, out);
#endif 
    //out = 0xFF;
  } else
    out = 0xFF;

  set_log("cpu");
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

extern bool emergency;

void CPU6809::invalid(const char* message) {
  log_emergency("CPU error detected: %s", message);
  printRegs();

  // stack trace
  Serial.printf("Stack:\n%04x:", s);
  for (uint16_t i = 0; i < 16; i++)
    Serial.printf(" %02x", read(s + i));
  Serial.println();

  // look for addresses in the stack
  for (uint16_t i = 0; i < 16; i++) {
    uint16_t entry = read_word(s + i);
    const char* entry_name = address_name(entry);
    if (entry_name[0] != '?')
      Serial.printf("[%04x] = %04x ~ %s\n", s + i, entry, entry_name);
  }
  
  Serial.flush();

  emergency = true;
}

void CPU6809::printRegs() { // AF 1/1/21 same order as MAME debugger to speed up debugging
  Serial.println("Register dump:"); 
 // Serial.printf("IR %04x (%s)\n", ir, address_name(ir));
  Serial.printf("PC %04x (%s)\n", pc, address_name(pc));
  Serial.printf("S  %04x (%s)\n", s, address_name(s));
  Serial.printf("CC %02x\n", cc.all);
  Serial.printf("DP %02x\n", dp);
  Serial.printf("A  %02x\n", a);
  Serial.printf("B  %02x\n", b);
  Serial.printf("D  %04x\n", (a<<8) + b);
  Serial.printf("X  %04x (%s)\n", x, address_name(x));
  Serial.printf("Y  %04x (%s)\n", y, address_name(y));
  Serial.printf("U  %04x (%s)\n", u, address_name(u));
  Serial.printf("MEMORY BANK (0, 1, 2, 3)= %d\n",via_rreg(0) & 0b0011);
  Serial.printf("Condition Code Register (Breakdown)\n");
  Serial.printf("   E F H I N Z V C\n");
  //Serial.printf("  | + + + + + + + |\n");
  Serial.printf("   %x %x %x %x %x %x %x %x\n", (cc.all>>7) &0x01, (cc.all>>6) &0x01,(cc.all>>5) &0x01, (cc.all>>4) &0x01, (cc.all>>3) &0x01, (cc.all>>2) &0x01, (cc.all>>1) &0x01, (cc.all>>0) &0x01);
 // Serial.printf("  | + + + + + + + |\n");
  Serial.printf("   E F H I N Z V C\n");
  //Serial.printf("     I a R e e   a\n");
 // Serial.printf("     R l Q g r O r\n");
 // Serial.printf("     Q f     o v r\n");
 // Serial.printf("       C       f y\n");
 // Serial.printf("       r       l  \n");
 // Serial.printf("       y       w  \n");
   Serial.println();
}

void CPU6809::printLastInstructions() {
  char buffer[64];

  for (int i = 0; i < INSTRUCTION_RECORD_SIZE; i++) {
    Instruction instruction = last_instructions[(instruction_loc + i) % INSTRUCTION_RECORD_SIZE];

    // Print instruction location and s
    const char* adrname = address_name(instruction.address);
    if (adrname[0] != '?' && strcmp(adrname, "wav data section"))
      Serial.printf("%s:\n", adrname);
    Serial.printf("%04x (S = %04x): ", instruction.address, instruction.s);

    // Print bytes
    int j;
    for (j = 0; j < instruction.byte_length; j++) {
      Serial.printf("%02x ", instruction.bytes[j]);
    }

    // Add spacing so the text all lines up
    for (; j < 4; j++) {
      Serial.printf("   ");
    }

    instruction.decode(buffer);
    Serial.println(buffer);
  }
}

void CPU6809::on_branch(const char* opcode, uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (branch)");
  /*if (debug) {
    const char* name = address_name(dst);
    Serial.printf("branch with opcode %s from %04x to %04x (%s)\n", opcode, src, dst, name);
  }*/
}

void CPU6809::on_branch_subroutine(const char* opcode, uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (branch subroutine)");
  /*if (debug) {
    const char* name = address_name(dst);
    Serial.printf("call with opcode %s from %04x to %04x (%s)\n", opcode, src, dst, name);
  }*/
}

void CPU6809::on_ret(const char* opcode, uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (return)");
  /*if (debug) {
    const char* name = address_name(dst);
    Serial.printf("return with opcode %s from %04x to %04x (%s)\n", opcode, src, dst, name);
  }*/
}

void CPU6809::on_nmi(uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (nmi)");
  if (debug) {
    const char* name = address_name(dst);
    log_debug("NMI from %04x to %04x (%s)\n", src, dst, name);
  }
}

void CPU6809::on_irq(uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (irq)");
  if (debug) {
    const char* name = address_name(dst);
      //  log_debug("IRQ from %04x to %04x (%s)\n", src, dst, name);
  }
}
void CPU6809::on_firq(uint16_t src, uint16_t dst) {
  if (dst < 0x8000UL)
    invalid("Not allowed to branch below 0x8000 (firq)");
  if (debug) {
    const char* name = address_name(dst);
    log_debug("FIRQ from %04x to %04x (%s)\n", src, dst, name);
  }
}

void CPU6809::print_memory(int address, int lines) {
  char s[17];

  s[16] = 0;

  address &= 0xFFF0;
  if (address == 0x8000) {
  Serial.println("ADDR |  f  f  f  f  f     f  f");
  Serial.println("ADDR |  d  d  d  d  d     d  d");
  Serial.println("ADDR |  c  c  c  c  c     c  c");
  Serial.println("ADDR |  c  r  t  s  b     s  e");
  Serial.println("ADDR |  m  t  r  e  u     t  r");
  Serial.println("ADDR |  d  r  k  c  f     a  r");
  Serial.println("ADDR |  d  y     t  f     t   ");
  }
  Serial.println("ADDR | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | 0123456789ABCDEF |");
  Serial.println("-----|------------------------------------------------ | ---------------- |");
  for (int line = 0; line < lines; line++) {
    Serial.printf("%04x |", address);
    for (int i = 0; i < 16; i++) {
      char c = read(address | i);
      Serial.printf(" %02x", c);

      if (c > 126 || c < 32)
        c = '.';
      s[i] = c;
    }
    Serial.print(" | ");
    Serial.print(s);
    Serial.println(" |");
    address += 16;
  }
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
