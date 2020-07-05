/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   fdc.cpp -- emulation of 1772 FDC based on (c) 2012 implementation by Gordon JC Pearce

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  

Original : fdc.c -- emulation of 1772 FDC, Copyright (C) 2012 Gordon JC Pearce
   
June 2020:  fdc.cpp -- ported emulation to work with SD card in Teensy 3.5/3.6
            Added implementation of DRQ and INTRQ, BUSY.
            by Alessandro Fasan
   
   */

#include "Arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "fdc.h"

#include <SD.h>
#include <SPI.h>

// Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;


/*
 Quick technical rundown on the Ensoniq Mirage Disk Format 

The disk only contains data on one side of the disk with 80 tracks numbered 0 - 79. 
Each track has five 1024 byte sectors numbered consecutively from zero to four followed 
by one sector of 512 bytes with a sector ID of five. 
The following examples should clarify this. 

TK  SC SIZE 
0   0-4  1024 bytes of data is first stored on Track 0, Sectors 0-4 
0     5   512 bytes of data is next stored on Track 0, Sector 5 
1   0-4  1024 bytes of data is next stored on Track 1, Sectors 0-4 
1     5   512 bytes of data is next stored on Track 1, Sector 5 this process continues until... 
.
.
.
79  0-4 1024 bytes of data is next stored on Track 79, Sectors 0-4 
79    5  512 bytes of data are stored in the last track - Track 79, Sector 5 

Teensy 3.5/3.6 do not have enough memory to store the whole 440Kbytes.
1024 x 5 x 80 + 512 x 80 = 450560 bytes (440 Kbytes)
So this implementation uses the SD.seek and SD.read(buf, bufsize).

We can read a sector of 1024 bytes, or a sector of 512 bytes.
We will only use a buffer of 1024 bytes for both cases and discern whether we need 1024
or 512 depending on the value in the register fdc.sec_r (possible values: 0, 1, 2, 3, 4 and 5)

*/

/*
This is how the OS is stored in each disk

         1024 bytes  512 bytes   Total mem   
OS       Sectors     Sector      bytes   
Track 0  0,1,2,3,4   5          5632    
Track 1  0,1,2,3,4,  5          5632    
Track 2  /           5           512   
Track 3  /           5           512   
Track 4  /           5           512   
Track 5  /           5           512   
Track 6  /           5           512   
Track 7  /           5           512   
Track 8  /           5           512   
Track 9  /           5           512   
Track 10 /           5           512   
                               15872 15.50 Kbytes
        
System Parameters           
Track 11  /          5           512 0.50  Kbytes
                               16.00 Kbytes
            
The OS loads from 8000 to  BFFF, which is 16Kbytes          
*/

/* WD1772

| A1 | A0 |  RW_=1 |  RW_=0             |  Register  | Also Available in 16Kbytes RAM Program Memory as
|  0 |  0 | STATUS | COMMAND FDC_CR     |   0xE800   |
|  0 |  1 |       TRACK      FDC_TRACK  |   0xE801   |   fdctrk  equ $8002
|  1 |  0 |      SECTOR      FDC_SECTOR |   0xE802   |   fdcsect equ $8003
|  1 |  1 |       DATA       FDC_DATA   |   0xE803   |   fdccmd  equ $8000
 
 fdccmd  equ $8000
 fdcrtry equ $8001
 fdctrk  equ $8002
 fdcsect equ $8003
 fdcbuff equ $8004
 fdcstat equ $8006
 fdcerr  equ $8007
 
 DRQ   is inverted (see DSK-1 schematics) and drives IRQ_

 DRQ: On DISK READ, DRQ is asserted (IRQ_ goes low) when a byte is available
      THE BIT IS CLEARED WHEN DATA IS READ BY THE PROCESSOR
 
 INTRQ is inverted (see DSK-1 schematics) and drives NMI_
 
 vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
 INTRQ (NMI_ goes low) is asserted at the completion of EVERY COMMAND.
 INTRQ is reset by:
  - Reading the Status Register
  - Loading Command Register with new command
  - Force Interrupt Command (which I don't see happening in the Boot ROM)
 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 Sector Length: hex 02 -> 512 bytes, hex 03 -> 1024 bytes (this is in the ID, not sure if/where/when/how to manage this)
 
 COMMANDS [TOP 4 BITS => which means (cmd & $f0) >> 4]
 
 After receiving a command, the STATUS REGISTER BUSY bit (bit 0) is set to 1

 0x0 0b0000 TYPE 1 - RESTORE                REQUIRED
 0x1 0b0001 TYPE 1 - SEEK                   REQUIRED
 0x2 0b0010 TYPE 1 - STEP (NO UPDATE)       -> NOT RELEVANT for BOOT ROM ?
 0x3 0b0011 TYPE 1 - STEP ( UPDATE)         -> NOT RELEVANT for BOOT ROM ?
 0x4 0b0100 TYPE 1 - STEP IN (NO UPDATE)    -> NOT RELEVANT for BOOT ROM ?
 0x5 0b0101 TYPE 1 - STEP IN ( UPDATE )     REQUIRED
 0x6 0b0110 TYPE 1 - STEP OUT (NO UPDATE)   REQUIRED
 0x7 0b0111 TYPE 1 - STEP OUT ( UPDATE )    -> CALLED FROM OS 3.2
 0x8 0b1000 TYPE 2 - READ SINGLE SECTOR     REQUIRED
 0x9 0b1001 TYPE 2 - READ MULTIPLE SECTORS  -> NOT RELEVANT for BOOT ROM ?
 0xA 0b1010 TYPE 2 - WRITE SINGLE SECTOR    -> CALLED FROM OS 3.2
 0xB 0b1011 TYPE 2 - WRITE MULTIPLE SECTORS -> CALLED FROM OS 3.2
 0xC 0b1100 TYPE 3 - READ ADDRESS           -> NOT RELEVANT for BOOT ROM ?
 0xD 0b1101 TYPE 3 - READ TRACK             -> NOT RELEVANT for BOOT ROM ?  ATTN: FORCE INTERRUPT LOOKS VERY SIMILAR: CHECK FOR 0x03 also!!!
 0xF 0b1111 TYPE 3 - WRITE TRACK            -> CALLED FROM FROM OS 3.2


 STATUS REGISTER
  ________________________________________________
 |   |   |   |   |   |   |   S2  |  S1   |   S0   |
 |   |   |   |   |   |   |  TR00 |  DRQ  |  BUSY  |
 |   |   |   |   |   |   |(bit 2)|(bit 1)| (bit 0)|
 |___|___|___|___|___|___|_______|_______|________|

 S0: BUSY is asserted when a Command is received. It is set to Zero by writing zero to it
 S1: DRQ
 S2: On Type I commands, this bit reflects the status of the TR00 signal.
     On Type 2 and 3 commands, it is a LOST BYTE bit. I will not model it here.
 
 The STATUS REGISTER shows how we will be modeling IRQ.
 I will need to model INTRQ: this is a register (1 bit only) that needs to be reset after being asserted.
 
 
*/

static File disk;
static uint8_t diskTrackdata[1024];
static unsigned long fdc_cycles;
static int s_byte;  // byte within sector
static bool ReadSectorDone;

int a;

extern unsigned long clock_cycle_count;

uint8_t fdc_intrq() {
 // if(fdc.intrq == 1) {
 //             Serial.printf("FDC WD1772: INTRQ Fired");
 //             Serial.printf(" clock_cycle_count = %ld\n", clock_cycle_count);
 //             }
  return(fdc.intrq);
}

uint8_t fdc_drq() {
//  if ( (fdc.sr & 0x02) == 0x02 ) {
//               Serial.printf("FDC WD1172: DRQ Fired");
//               Serial.printf(" clock_cycle_count = %ld\n", clock_cycle_count);
//                }
  return( (fdc.sr & 0x02) == 0x02); //DRQ bit
}

int fdc_init() {
//char mybuffer[100000];
//int z, k;

ReadSectorDone = true;

 Serial.print("Initializing SD card...");

 if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return(-2);
  }
  Serial.println("SD CARD Initialization: Completed.");

 // open the file.
  disk = SD.open("A1.img"); // TO DO: CHOOSE FROM AVAIL IMGs IN SD ROOT DIRECTORY
    if (disk) 
      Serial.println("Found Mirage image disk: disk.img");
      else 
       {
        Serial.println("Error: Mirage image disk not found!!!");
        return(-1);
        }

	return(0);
}


void fdc_run() {
 // int i, z;

	// called every cycle
	if ((fdc.sr & 0x01) == 0) return;   // nothing to do if the FDC is NOT BUSY

 //Serial.printf("Entering I: FDC_RUN\n");
 // Serial.printf("fdc.sr & 0x01 = %02x\n",fdc.sr & 0x01);
 // Serial.printf("fdc.cr & 0xf0 = %02x\n", fdc.cr & 0xf0);
 // Serial.printf(" %ld < %ld? \n", clock_cycle_count, fdc_cycles);

	if (clock_cycle_count < fdc_cycles) return; // not ready yet
 // if (fdc_drq() | fdc_intrq()) return; // nothing to do while interrupts are active

 // Serial.printf("Entering II: FDC_RUN\n");
  //Serial.printf("fdc.cr & 0xf0 = %02x\n", fdc.cr & 0xf0);
  
	switch (fdc.cr & 0xf0) {
		case 0x00:  // Restore
			Serial.printf("***** fdc_run(): restore\n");
			fdc.trk_r = 0;  // Track 0
			fdc.sr = 0x04;  // We are emulating TR00 HIGH from the FDC (track at 0), clear BUSY and DRQ INTERRUPT
      ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
      //fdc_cycles = clock_cycle_count + 32; // new
      fdc.intrq = 1; // Assert INTRQ interrupt at the end of command
			return;
      break;
		case 0x10:  // Seek
			Serial.printf("***** fdc_run(): Seek\n");
			fdc.trk_r = fdc.data_r;
			fdc.sr = 0; // Clear BUSY and DRQ INTERRUPT
      ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
			if (fdc.trk_r == 0) fdc.sr |= 0x04;  // track at 0, emulates TR00 HIGH for Type 1 command
      //fdc_cycles = clock_cycle_count + 32; // new
		  fdc.intrq = 1; // Assert INTRQ interrupt at the end of command
			return;
      break;
    case 0x50:  // Step In
      Serial.printf("***** fdc_run(): Step In\n");
      //fdc_cycles = clock_cycle_count + 32; // new
      fdc.intrq = 1; // Assert INTRQ interrupt at the end of command
      return;
      break;
    case 0x60:  // Step Out
      Serial.printf("***** fdc_run(): Step Out\n");
      //fdc_cycles = clock_cycle_count + 32; // new
      fdc.intrq = 1; // Assert INTRQ interrupt at the end of command
      return;
      break;
		case 0x80:  // Read Sector
      // TODO : If this is a NEW read sector request (ReadSectorDone == TRUE)
      // TODO :   Calculate the memory location: (1024*fdc.sec_r)+(5632*fdc.trk_r)
      // TODO :   do a seek there
      // TODO :   do a disk.read(diskTrackdata, 1024)
      // TODO :   now we can do fdc.data_r = diskTrackdata[a];
      // TODO : If this is NOT a NEW read sector request
      // TODO :   continue with fdc.data_r = diskTrackdata[a];
      // TODO : What are the other situations when ReadSectorDone is TRUE? 
      // TODO:  ANSWER: when trk_r and sec_r are written (wreg)
      // TODO:          THIS INCLUDES "Restore", when trk_r is modified again
      // TODO:  Can a Restore or any other operations changing trk_r and sec_r occur
      // TODO:  while a Read Sector is being performed? No, because the FDC has the Busy flag asserted.
      // TOOD:
			//Serial.printf("**** fdc_run(): read sector %d\n", fdc.sec_r);
     
      if (ReadSectorDone) {
                Serial.printf("FDC RUN: Performing Seek @ %d;  ", (1024*fdc.sec_r)+(5632*fdc.trk_r));
                Serial.printf("s_byte = %d\n", s_byte);
                disk.seek((1024*fdc.sec_r)+(5632*fdc.trk_r));
                disk.read(diskTrackdata, 1024);
                }
      a = s_byte;
      if(s_byte > 1024) Serial.printf("**************SOMETHING IS WRONG*************"); // remove this after debug
			fdc.data_r = diskTrackdata[a];
      //Serial.printf("*** fdc_run(): s_byte=%04x trk=%d sec=%d disk addr = %04x data=%02x\n",s_byte, fdc.trk_r, fdc.sec_r, a, fdc.data_r);
			fdc.sr |= 0x02;  // Assert IRQ_, Data is ready!
			s_byte++;
			fdc_cycles = clock_cycle_count + 32; // original 32
      
			if (s_byte > (fdc.sec_r == 5 ? 512 : 1024) ) {
				        fdc.sr &= 0xfe; // Clear the Busy Bit, we will disregard the last value read anyway, this is needed to keep irq and nmi in sync
                ReadSectorDone = true; // NEXT TIME: This will force reading a new sector of 1024 bytes from the SD card
				        Serial.printf("SDFDC: Done reading sector %x\n", fdc.sec_r);
				        fdc.intrq = 1; // NMI_ Interrupt at the end of READ SECTOR command
			          }
                else 
                  ReadSectorDone = false;
			return;
      break;
		default: // Others
			Serial.printf("***** FDC EMULATION RUN: NOT SUPPORTED(%02x)\n", fdc.cr);
			fdc.sr = 0;
      return;
			break;
	}
    // Should we ever get here?
	fdc.sr &= 0xfe;	// stop, clear BUSY bit
    Serial.printf("***** FDC EMULATION RUN: <fdc.sr &= 0xfe;  // stop> where fdc.sr =%02x ", fdc.sr);
}

uint8_t fdc_rreg(uint8_t reg) {
	
	// handle reads from FDC registers
	uint8_t val;

	switch (reg & 0x03) {
		case FDC_SR:
			      val = fdc.sr;
            fdc.intrq = 0; //INTRQ is de-asserted (NMI_ goes HIGH) after reading the STATUS REGISTER
            Serial.printf("FDC_RREG FDC: SR ");
            Serial.printf(" val => %02x (FDC)\n",val);
			      break;
		case FDC_TRACK:
			      val = fdc.trk_r;
            Serial.printf("FDC_RREG FDC_TRACK: ");
            Serial.printf(" val => %02x (FDC)\n",val);
			      break;
		case FDC_SECTOR: 
			      val =  fdc.sec_r;
            Serial.printf("FDC_RREG FDC_SECTOR: ");
            Serial.printf(" val => %02x (FDC)\n",val);
			      break;
		case FDC_DATA:
           // Serial.printf("FDC_RREG DATA, I clear the IRQ bit. DATA "); Serial.printf(" val => %02x (FDC)\n",val);
			      fdc.sr &= 0xfd; // mask is 1111_1101, we are clearing the DRQ bit (IRQ_ goes HIGH) as Data is being read
			      val =  fdc.data_r;
			      break;
      default: 
            Serial.printf("*** FDC READING REGISTER EMULATION reg %02x: CURRENTLY NOT SUPPORTED\n", reg);
            break;
	}
  
	return(val);
}

void fdc_wreg(uint8_t reg, uint8_t val) {
	
	// handle writes to FDC registers
	int cmd = (val & 0xf0)>>4;

  //Serial.printf("Entering: FDC_WREG\n");
  //Serial.printf("FDC_WREG reg => %02x; val =>  %02x\n", reg, val);
  //Serial.printf("FDC_WREG cmd => %02x\n", cmd);
  //Serial.printf("FDC_WREG reg & 0x03 = > %02x\n", reg & 0x03);

	switch (reg & 0x03) {
		case FDC_CR: // 0
            fdc.intrq = 0; //INTRQ is de-asserted (NMI_ goes HIGH) after loading the Command Register with new command
            Serial.printf("FDC_WREG COMMAND fdc.cr = %02x\n", val);
			if ((val & 0xf0) == 0xd0) { // mask is 1111_0000, comparig with 1101_0000 force interrupt
				      Serial.printf("FDC_WREG cmd %02x: force interrupt\n", val);
				      fdc.sr &= 0xfe; // mask is 1111_1110,  we are clearing the busy bit
				      return;
			        }
            Serial.printf("FDC_WREG FDRC fdc.sr & 0x01 shows %s BUSY\n", fdc.sr & 0x01 ? "" : "NOT" );
			if (fdc.sr & 0x01) return; // Just return if BUSY
            Serial.printf("FDC_WREG FDRC: cmd %02x val = %02x\n", cmd, val);
			fdc.cr = val;
			switch(cmd) {
				     case 0x0: // Restore
					            Serial.printf("FDC_WREG cmd %02x: restore\n",  val);
					            fdc_cycles = clock_cycle_count + 1000; //1000000;   // remove (or minimize) tbis
					            fdc.sr = 0x01; // busy
					            break;
				     case 0x1: // Seek
					            Serial.printf("FDC_WREG cmd %02x: seek to %d\n", val, fdc.data_r);
					            fdc_cycles = clock_cycle_count + 1000; //1000000;   // remove (or minimize) tbis
					            fdc.sr = 0x01;  // busy
					            break;
				     case 0x5: // Step In
                      Serial.printf("FDC_WREG cmd %02x: Step in %d\n", val, fdc.trk_r++);
					            fdc_cycles = clock_cycle_count + 100; //100000;    // remove (or minimize) tbis
					            fdc.trk_r++;
                      fdc.sr = 0x01;  // busy
					            ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
				              break;
				     case 0x6: // Step Out
					            Serial.printf("FDC_WREG cmd %02x: Step out (w/o Update) %d\n", val, fdc.trk_r--);
					            fdc_cycles = clock_cycle_count + 100; //100000;    // remove (or minimize) tbis
					            fdc.trk_r--;
                      fdc.sr = 0x01;  // busy
					            ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
				              break;
             case 0x7:
                      Serial.printf("*** FDC WRITING COMMAND: Step Out (w Update (%02x) CURRENTLY NOT SUPPORTED (val = %02x)\n", cmd, val);
                      break;
				     case 0x8: // Read Single Sector
					            Serial.printf("FDC_WREG cmd %02x: read sector\n", val);
					            fdc_cycles = clock_cycle_count + 10; //1000;    // remove (or minimize) tbis
					            s_byte = 0;
					            fdc.sr = 0x01; // busy
                      ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
					            break;
             case 0xa: // Write Single Sector
                      Serial.printf("*** FDC WRITING COMMAND: Write Single Sector (%02x) CURRENTLY NOT SUPPORTED (val = %02x)\n", cmd, val);
                      break;
             case 0xb: // Write Multiple Sectors
                      Serial.printf("*** FDC WRITING COMMAND: Write Multiple Sectors (%02x) CURRENTLY NOT SUPPORTED (val = %02x)\n", cmd, val);
                      break;
             case 0xf: // Write Track
                      Serial.printf("*** FDC WRITING COMMAND: Write Track (%02x) CURRENTLY NOT SUPPORTED (val = %02x)\n", cmd, val);
                      break;
				      default: //0f
					            Serial.printf("*** FDC WRITING REGISTER EMULATION: COMMAND cmd %02x: CURRENTLY NOT SUPPORTED (val = %02x)\n", cmd, val);
					            fdc.sr = 0;
					            fdc_cycles = 0;
					            break;
			        }
			break;
		case FDC_TRACK:
			Serial.printf("FDC_WREG *TRACK* = %d\n",  val);
			fdc.trk_r = val; 
			ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
			break;
		case FDC_SECTOR:
			Serial.printf("FDC_WREG *SECTOR* = %d\n", val);
			fdc.sec_r = val;
			ReadSectorDone = true; // This will force reading a new sector of 1024 bytes from the SD card
			break;
		case FDC_DATA:
    	Serial.printf("FDC_WREG *DATA* = %d\n", val);
			fdc.data_r = val;
			break;
    default:
      Serial.printf("*** FDC WRITING REGISTER EMULATION reg %02x: CURRENTLY NOT SUPPORTED\n", reg);
      fdc.sr = 0;
      fdc_cycles = 0;
      break;
	}
}
