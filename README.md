# EnsoniqMirageRetroshield
Bringing the Ensoniq Mirage back to life with TeensyDuino + Retroshield (8BitForce, by Erturk Kocalar).
Using Teensyduino 3.5 with 8BitForce's Retroshield and 6809 CPU
Modeling VIA6522 and FDC WD1772 based on prior work by GordonJCPearce (*thank you*).
Many, many, many *thank you* to Erturk for his passion for CPUs and for getting me going!

This implenentation boot from the ROM and start executing MASOS 3.2 from the image (any Mirage *.img based on OS3.2 should work) stored in the SD card
There are several things left to do:
 * the VIA 6522 is not fully implemented
 * the FDC reads successfully from the SD card in the Teensy!!! This was a major achievement. However, no "write" instruction is yet implemented - as it is not currently a priority
 * the DOC is not implemented. There is a model available for MAME, but the idea is to use 2nd order digital filters and to sum up the signal digitally within the Teensyduino
 * my "dream" is to implement the original interface using a touch TFT screen, but also allow an alternate more modern user interface.
 
While the Retroshield does indeed work at 1MHz for some SBC implementation, I am not sure that this will be fast enough to make the Mirage sing.
HOWEVER, TeensyDuino 4.1 is now available, maybe Erturk from 8BitForce will update his Retroshield to support it. 

Contact me alex at fasan dot US if you want to contribute to this project.
