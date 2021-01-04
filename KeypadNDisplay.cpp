/*
   KeypadNDisplay.c - emulation of the Mirage Keypad and Display
   using an ILI9341 or ILI9488 TFT Display with Touch
   (C) 2021 - Alessandro Fasan
   */

/*   
 *   ANODE 1 is PA4 -> input to this model
 *    when LOW Drives LEFT Display
 *   ANODE 2 is PA3 -> input to this model
 *    when LOW Drives RIGHT Display
 *   CAD0, CAD1, CAD2 are (PA0, PA1, PA2) ->  inputs to this model
 *   ROW0, ROW1, ROW2 are (PA5, PA6, PA7) -> outputs to this model
 *    
 */
#define KD_DEBUG 0
#define KD_DEBUGWReg 0
#define KD_DEBUGRReg 0

#include "KeypadNDisplay.h"
#include "SPI.h"
#include "Arduino.h"
#include "stdint.h"
#include "via.h"

//#include "bus.h"
//#include "log.h"

#ifdef ILI9341
#include "ILI9341_t3.h"
 #else
  #include "ILI9488_t3.h"
#endif

#include <XPT2046_Touchscreen.h>  // NOTE: If Touch does not work, disconnect MISO (poor Display design)
#define CS_PIN      8
#define TIRQ_PIN    4 
XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts

//STANDARD SETTINGS - Remember to Modify for use with Teensy Audio Board
#define TFT_CS     10
#define TFT_DC      9
#define TFT_RST   255 // Unused - connect to 3.3V  

#ifdef ILI9341
ILI9341_t3 tft=ILI9341_t3(TFT_CS, TFT_DC);
 #else
  ILI9488_t3 tft=ILI9488_t3(TFT_CS, TFT_DC, TFT_RST);
#endif




#ifdef ILI9341
#define BCKGND_COLOR      CL(80,80,80) //Dark Background, but not Black
#define SEG_ON_COLOR      ILI9341_YELLOW
#define SEG_OFF_COLOR     ILI9341_BLACK
#define BUTTON_COLOR      ILI9341_ORANGE
#define BUTTON_DARKGREY   ILI9341_DARKGREY 
#define ILI_WHITE         ILI9341_WHITE
#define ILI_RED           ILI9341_RED
#define ILI_DARKGREEN     ILI9341_DARKGREEN
#define ILI_GREY          ILI9341_LIGHTGREY
#define ILI_BLACK         ILI9341_BLACK 
 #else
  #define BCKGND_COLOR      CL(80,80,80) //Dark Background, but not Black
  #define SEG_ON_COLOR      ILI9488_YELLOW
  #define SEG_OFF_COLOR     ILI9488_BLACK
  #define BUTTON_COLOR      ILI9488_ORANGE
  #define BUTTON_DARKGREY   ILI9488_DARKGREY 
  #define ILI_WHITE         ILI9488_WHITE
  #define ILI_RED           ILI9488_RED
  #define ILI_DARKGREEN     ILI9488_DARKGREEN
  #define ILI_GREY          ILI9488_LIGHTGREY
  #define ILI_BLACK         ILI9488_BLACK 
   
#endif

#define minXdim 3
#define minYdim 9
#define scaleFactor 2 // experiment with digit size

// define segment truth table for each hex symbol
static const int digit_array[17][8] = {
  {1, 1, 1, 1, 1, 1, 0, 0},  // 0
  {0, 1, 1, 0, 0, 0, 0, 0},  // 1
  {1, 1, 0, 1, 1, 0, 1, 0},  // 2
  {1, 1, 1, 1, 0, 0, 1, 0},  // 3
  {0, 1, 1, 0, 0, 1, 1, 0},  // 4
  {1, 0, 1, 1, 0, 1, 1, 0},  // 5
  {1, 0, 1, 1, 1, 1, 1, 0},  // 6
  {1, 1, 1, 0, 0, 0, 0, 0},  // 7
  {1, 1, 1, 1, 1, 1, 1, 0},  // 8
  {1, 1, 1, 0, 0, 1, 1, 0},  // 9
  {1, 1, 1, 0, 1, 1, 1, 0},  // A
  {0, 0, 1, 1, 1, 1, 1, 0},  // b
  {1, 0, 0, 1, 1, 1, 0, 0},  // C
  {0, 1, 1, 1, 1, 0, 1, 0},  // d
  {1, 0, 0, 1, 1, 1, 1, 0},  // E
  {1, 0, 0, 0, 1, 1, 1, 0},  // F
  {0, 0, 0, 0, 0, 0, 0, 1}   // . (DP)
};


 uint8_t button_array[8] = {0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7};
// Every combination of (PA7, PA6, PA5) identifies a COLUMN in the keypad (after being decoded by a 74LS145)
// A key pressed will read as a 1'b0 in its corresponding row  
// PA7 PA6 PA5    A B C D E F G Dp                                                  
//  0   0   0 => 0 1 1 1 1 1 1 1   { 1, 1, 1}, //  "1",       "3",     "LOAD UPPER"
//  0   0   1 => 1 0 1 1 1 1 1 1   { 1, 1, 1}, //  "4",       "6",     "LOAD LOWER"
//  0   1   0 => 1 1 0 1 1 1 1 1   { 1, 1, 1}, //  "7",       "9",     "SAMPLE UPPER"
//  0   1   1 => 1 1 1 0 1 1 1 1   { 1, 1, 1}, //  "^",       "5",     "PLAY SEQ"
//  1   0   0 => 1 1 1 1 0 1 1 1   { 1, 1, 1}, //  "PARAM",   "8",     "LOAD SEQ"
//  1   0   1 => 1 1 1 1 1 0 1 1   { 1, 1, 1}, //  "v",       "0",     "SAVE SEQ"
//  1   1   0 => 1 1 1 1 1 1 0 1   { 1, 1, 1}, //  "VALUE",   "2",     "REQ SEQ"
//  1   1   1 => 1 1 1 1 1 1 1 0   { 1, 1, 1}  //  "CANCEL",  "ENTER", "SAMPLE LOWER"

/*
void init_buttons() {
  int i,j;
  
  for (i=0; i++; i<8) 
      button_array[i] = 0x7;
  
    
}

*/

uint8_t ORA_register_;
uint8_t KeypadNDisplay_clk;


void print_buttons() {
  int i;
   
   for (i=0; i++; i<8) {
    tft.setCursor(200, 200+i*4);
        tft.print( (button_array[i]&0x4>>2));
        tft.print( (button_array[i]&0x2>>1)); 
        tft.print( (button_array[i]&0x1));
   }
}


void KeypadNDisplay_init()   {
 // Serial.begin(9600);
 // while (!Serial);
 // Serial.printf("I am in Setup\n");
 // init_buttons();
 // print_buttons();

  
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(BCKGND_COLOR);


  ts.begin();
  ts.setRotation(3); // This will match the TFT Rotation



#ifdef ILIDebug
tft.drawRect(1, 240, 479, 80, ILI_WHITE); 
tft.setTextColor(ILI_WHITE);  tft.setTextSize(1); 
tft.setCursor(1, 245); 
tft.println("Teensyduino-based Mirage Emulator");
tft.setCursor(1, 255); 
tft.println("This is area is left for debugging");
tft.setCursor(1, 265); 
tft.println("I will use this to detect whether the screen has been touched...");
#endif

 tft.setCursor(30, 20);
 tft.setTextColor(ILI_RED);  tft.setTextSize(2);
 tft.println("ALFASoniQ");
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.setCursor(310, 24);
 tft.println(" Digital Multisampler");
 tft.drawLine(22, 37, 450, 37, ILI_RED);

// Draw Dark Green Display Background
 tft.fillRect(36, 54, 72, 54, ILI_DARKGREEN); // original 42, 54, 66, 54, ILI_DARKGREEN);
 //Serigraphy - Left  Digit
 tft.drawLine(45, 110, 45, 135, ILI_WHITE); // vertical white line
 tft.drawLine(35, 135, 60, 135, ILI_WHITE); // horizitontal
 //Serigraphy - Right Digit
 tft.drawLine(85, 110, 85, 135, ILI_WHITE); // vertical white line
 tft.drawLine(75, 135, 115, 135, ILI_WHITE); // horizontal

// 4 buttons under the display
 tft.fillRect(40, 160,  20, 10 , ILI_GREY); //  "Param" 480 < x < 800     1900 < y < 2400
 tft.setCursor(35, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Param");
 
 tft.fillRect(90, 160,  20, 10 , ILI_GREY);  // "Value" 800 < x < 1120    1900 < y < 2400 
 tft.setCursor(85, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Value");
 
 tft.fillRect(40, 210,  20, 10 , ILI_GREY);  // "Off"   480 < x < 800     2400 < y < 2900
  tft.setCursor(34, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Off/v");
 
 tft.fillRect(90, 210,  20, 10 , ILI_GREY);  // "On"    800 < x < 1120    2400 < y < 2900   
 tft.setCursor(85, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("On/^");


// Keypad
 tft.fillRect(160,  60,  20, 10 , BUTTON_COLOR);  // "1" 1450 < x < 1700   750 < y < 1150
 tft.setCursor(166, 50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("1");
 
 tft.fillRect(210,  60,  20, 10 , BUTTON_COLOR);  // "2"  1700 < x < 2100  750 < y < 1150
 tft.setCursor(216, 50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("2");
 
 tft.fillRect(260,  60,  20, 10 , BUTTON_COLOR);  // "3" 2100 < x < 2500   750 < y < 1150
 tft.setCursor(266, 50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("3");
 
 tft.fillRect(160, 110,  20, 10 , BUTTON_COLOR);   //  "4" 1450 < x < 1700  1150 < y < 1550    
 tft.setCursor(166, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("4");
 
 tft.fillRect(210, 110,  20, 10 , ILI_GREY);      // "5" 1700 < x < 2100  1150 < y < 1550 
 tft.setCursor(216, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("5");
 
 tft.fillRect(260, 110,  20, 10 , ILI_GREY);     // "6" 2100 < x < 2500   1150 < y < 1550 
  tft.setCursor(266, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("6");
 
 tft.fillRect(160, 160,  20, 10 , ILI_GREY);      // "7"  1450 < x < 1700    1900 < y < 2400 
 tft.setCursor(166, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("7");
 
 tft.fillRect(210, 160,  20, 10 , ILI_GREY);      // "8"  1700 < x < 2100     1900 < y < 2400 
 tft.setCursor(216, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("8");
 
 tft.fillRect(260, 160,  20, 10 , ILI_GREY);      // "9"  2100 < x < 2500    1900 < y < 2400 
 tft.setCursor(266, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("9");
 
 tft.fillRect(160, 210,  20, 10 , ILI_GREY);      // "Cancel"  1450 < x < 1700   2400 < y < 2900 
 tft.setCursor(152, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Cancel");
 
 tft.fillRect(210, 210,  20, 10 , BUTTON_COLOR);  // "0/Prog"  1700 < x < 2100  2400 < y < 2900 
 tft.setCursor(202, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("0/Prog");
 
 tft.fillRect(260, 210,  20, 10 , ILI_WHITE);     // "Enter"    2100 < x < 2500   2400 < y < 2900 
 tft.setCursor(255, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Enter");

// 4 SEQ buttons
 tft.fillRect(320,  60,  20, 10 , ILI_GREY);    // "Rec" 2600 < x < 3000    750 < y < 1150
 tft.setCursor(320, 50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Rec");
 
 tft.fillRect(320, 110,  20, 10 , ILI_GREY);    // "Play" 2600 < x < 3000   1150 < y < 1550 
 tft.setCursor(318, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Play");
 
 tft.fillRect(320, 160,  20, 10 , ILI_GREY);  // "Load"   2600 < x < 3000   1900 < y < 2400  
 tft.setCursor(318, 150);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Load");
 
 tft.fillRect(320, 210,  20, 10 , ILI_GREY); // "Save"    2600 < x < 3000  2400 < y < 2900 
 tft.setCursor(318, 200);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Save");


// SAMPLE Upper and Lower buttons
 tft.fillRect(370,  60,  20, 10 , ILI_BLACK);// "Sample Upper"  3000 < x < 3400    750 < y < 1150
 tft.setCursor(365, 50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Upper");
 tft.fillRect(370, 110,  20, 10 , ILI_BLACK); // "Sample Lower" 3000 < x < 3400    1150 < y < 1550 
  tft.setCursor(365, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Lower");

 // LOAD Upper and Lower Buttons
 tft.fillRect(430,  60,  20, 10 , ILI_WHITE); // "Load Upper"   3400 < x < 3800    750 < y < 1150
 tft.setCursor(425,50);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Upper");
 tft.fillRect(430, 110,  20, 10 , ILI_WHITE); // "Load Lower"   3400 < x < 3800    1150 < y < 1550 
 tft.setCursor(425, 100);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("Lower");
 
// Lines and Bottom Serigraphy
 tft.drawLine(160,  75,  450, 75, ILI_WHITE);
 tft.drawLine(160,  125, 450, 125, ILI_WHITE);
 tft.drawLine(40,  175, 450, 175, ILI_WHITE);
 tft.drawLine(40,  225, 450, 225, ILI_WHITE);

 tft.drawLine(60,  226, 100, 226, ILI_WHITE);
 tft.drawLine(60,  227, 100, 227, ILI_WHITE);
 tft.setCursor(60,230);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("CONTROL");

 tft.drawLine(204,  226, 240, 226, ILI_WHITE);
 tft.drawLine(204,  227, 240, 227, ILI_WHITE);
 tft.setCursor(205,230);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("SELECT");

 tft.drawLine(318,  226, 340, 226, ILI_WHITE);
 tft.drawLine(318,  227, 340, 227, ILI_WHITE);
 tft.setCursor(322,230);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("SEQ");

 tft.drawLine(358,  226, 395, 226, ILI_WHITE);
 tft.drawLine(358,  227, 395, 227, ILI_WHITE);
 tft.setCursor(360,230);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("SAMPLE");

 tft.drawLine(420,  226, 448, 226, ILI_WHITE);
 tft.drawLine(420,  227, 448, 227, ILI_WHITE);
 tft.setCursor(422,230);
 tft.setTextColor(ILI_WHITE);  tft.setTextSize(1);
 tft.println("LOAD");

//render_digit(45, 60, 16,  SEG_ON_COLOR);
//render_digit(79, 60, 16,  SEG_ON_COLOR);


}

void KeypadNDisplay_run() {
int i,j;
int k;
int bc;
bool RightDigit_en;
bool LeftDigit_en;
uint8_t ORA_register;
uint8_t leftSegment, rightSegment;


ORA_register = via_rreg(0xF) & 0x1F; 
bc = ORA_register_ ^ ORA_register;  // see if ORA has changed
if (bc == 0)
          return;                   // otherswise there is nothing to do for now

ORA_register_ = ORA_register;

LeftDigit_en =  !(ORA_register & ANODE1);
RightDigit_en = !(ORA_register & ANODE2);

leftSegment = ORA_register & 0x07;
rightSegment = ORA_register & 0x07;

render_segment( 45, 60, rightSegment, RightDigit_en ? SEG_ON_COLOR : SEG_OFF_COLOR);
render_segment( 79, 60, leftSegment,  LeftDigit_en  ? SEG_ON_COLOR : SEG_OFF_COLOR);


 
/*
render_digit(45, 60, 2, SEG_ON_COLOR);
render_digit(8, 60, 16, SEG_ON_COLOR);
render_digit(79, 60, 1, SEG_ON_COLOR);
render_digit(47, 60, 16, SEG_ON_COLOR);
*/

if (ts.touched()) {
  TS_Point p = ts.getPoint(); // p.x and p.y are the coordinates
  tft.fillRect(1, 240, 479, 80, CL(80,80,80)); 
  tft.setCursor(20,300);
  tft.print(p.x);
  tft.setCursor(200,300);
  tft.print(p.y);
  tft.setCursor(20, 275); 
  tft.print("Button Pressed: ");
  tft.setCursor(230, 275);

 
  // Reset previous value
  for (i=0; i++; i<8) {
      button_array[i] = 0x7;
      Serial.print(button_array[i]); 
  }

//  "1"            1450 < x < 1700     750 < y < 1150
//  "2"            1700 < x < 2100     750 < y < 1150   COL 1 / G
//  "3"            2100 < x < 2500     750 < y < 1150
// "Rec"           2600 < x < 3000     750 < y < 1150   COL 0 / G
// "Sample Upper"  3000 < x < 3400     750 < y < 1150   COL 0 / C
// "Load Upper"    3400 < x < 3800     750 < y < 1150   COL 0 / A

if ( (p.y> 650) && (p.y < 1150) ) { // "1", "2", "3", "REC", "Sample Upper", "Load Upper"
    if( (p.x>1450) && (p.x < 1700)) { 
        button_array[1] = 0x3; //(0,1,1);
        tft.print("1"); 
    }
      else if (p.x<2100) { 
        button_array[7] = 0x5; //(1,0,1);
        tft.print("2");
      }
      else if (p.x<2500) { 
        button_array[1] = 0x5; //(1,0,1);
        tft.print("3");
      }  
      else if (p.x<3000) { 
        button_array[7] = 0x6; //(1,1,0);
        tft.print("REC");
      }
      else if (p.x<3400) { 
        button_array[3] = 0x6; //(1,1,0);
        tft.print("SAMPLE UPPER");
      }
     else if (p.x<3800) { 
        button_array[1] = 0x6; // (1,1,0);
        tft.print("LOAD UPPER");
     }

//  "4"           1450 < x < 1700    1150 < y < 1550 
//  "5"           1700 < x < 2100    1150 < y < 1550 
//  "6"           2100 < x < 2500    1150 < y < 1550 
// "Play"         2600 < x < 3000    1150 < y < 1550  COL 0 / D
// "Sample Lower" 3000 < x < 3400    1150 < y < 1550  COL 0 / DP
// "Load Lower"   3400 < x < 3800    1150 < y < 1550  COL 0 / B
    
} else if ( (p.y>= 1150) && (p.y < 1750) ) { // "4", "5", "6", "PLAY", "Sample Lower", "Load Lower"
    if( (p.x > 1450) && (p.x < 1700)) { 
        button_array[2] = 0x3; //(0,1,1);
        tft.print("4"); 
    }
      else if (p.x<2100) { 
        button_array[4] = 0x5; //(1,0,1);
        tft.print("5");
      }
      else if (p.x<2500) { 
        button_array[2] = 0x5; //(1,0,1);
        tft.print("6");
      }  
      else if (p.x<3000) { 
        button_array[4] = 0x6; //(1,1,0);
        tft.print("PLAY");
      }
      else if (p.x<3400) { 
        button_array[7] = 0x6; //(1,1,0);
        tft.print("SAMPLE LOWER");
      }
     else if (p.x<3800) { 
        button_array[2] = 0x6; //(1,1,0);
        tft.print("LOAD LOWER");
      }

//  "Param" 480 < x < 800     1900 < y < 2400           COL 2 /  E
//  "Value" 800 < x < 1120    1900 < y < 2400           COL 2 /  G
//  "7"    1450 < x < 1700    1900 < y < 2400           COL 2 /  C      
//  "8"    1700 < x < 2100    1900 < y < 2400           COL 1 /  E 
//  "9"    2100 < x < 2500    1900 < y < 2400           COL 1 /  C
// "Load"  2600 < x < 3000    1900 < y < 2400           COL 0 /  E
} else if ( (p.y>= 1750) && (p.y < 2400) ) { // "PARAM", "VALUE", "7", "8", "9",  "LOAD SEQ"
        if(p.x < 900) { 
        button_array[5] = 0x3; //(0,1,1);
        tft.print("Param"); 
    }
      else if (p.x<1200) { 
        button_array[6] = 0x3; //(0,1,1);
        tft.print("Value");
      }
      else if (p.x<1700) { 
        button_array[3] = 0x3; //(0,1,1);
        tft.print("7");
      }  
      else if (p.x<2100) { 
        button_array[5] = 0x5; //(1,0,1);
        tft.print("8");
      }
      else if (p.x<2500) { 
        button_array[3] = 0x5; //(1,0,1);
        tft.print("9");
      }
     else if (p.x<3000) { 
        button_array[5] = 0x5; //(1,1,0);
        tft.print("LOAD SEQ");
      }

//   "Off"   480 < x <  800   2400 < y < 2900
//   "On"    800 < x < 1120   2400 < y < 2900
//"Cancel"  1450 < x < 1700   2400 < y < 2900           COL 2 / DP
//"0/Prog"  1700 < x < 2100   2400 < y < 2900 
//"Enter"   2100 < x < 2500   2400 < y < 2900           COL 1 / DP
// "Save"    2600 < x < 3000  2400 < y < 2900           COL 0 / F
} else if ( (p.y> 2400) && (p.y < 2900) ) { // "OFF", "ON", "CANCEL", "0/PROG", "ENTER",  "SAVE SEQ"
    if(p.x < 900) { 
        button_array[6] = 0x3; //(0,1,1);
        tft.print("OFF/v"); 

    }
      else if (p.x<1200) { 
        button_array[4] = 0x3; //(0,1,1);
        tft.print("ON/^");
      }
      else if (p.x<1700) { 
        button_array[8] = 0x3; //(0,1,1);
        tft.print("CANCEL");
      }  
      else if (p.x<2100) { 
        button_array[6] = 0x5; //(1,0,1);
        tft.print("0/PROG");
      }
      else if (p.x<2500) { 
        button_array[7] = 0x5; //(1,0,1);
        tft.print("ENTER");
      }
     else if (p.x<3000) { 
        button_array[6] = 0x6; //(1,1,0);
        tft.print("SAVE SEQ");
      }
           
  } // else if

 //print_buttons();
}  // if ts.touched()
/*
for (i=0; i<16; i++) {
  render_digit(45, 60, i,  SEG_ON_COLOR);
   for (j=0; j<16; j++) {
     render_digit(79, 60, j,  SEG_ON_COLOR);
     delay(500);
     render_digit(79, 60, j,  SEG_OFF_COLOR);
   }
  delay(500);
  render_digit(45, 60, i,  SEG_OFF_COLOR);
  }
*/
   
} // end KeypadNDisplay_run


// TO EMULATE THE DISPLAY WE WILL NEED
// ANODE 1 and ANODE 2 (PA4 and PA3 respectively)
// EACH DISPLAY WILL BE ACTIVE WHEN AN is ZERO
// ANODE 1 (PA4)
// 0    Select the Segment chosen by PA7, PA6, PA5
// 1    Display not selected - all segments are OFF
// ANODE 2 (PA3)
// 0    Select the Segment chosen by PA7, PA6, PA5
// 1    Display not selected - all segments are off


void render_segment(uint8_t pos_x, uint8_t pos_y, 
                    uint8_t segment, uint16_t color) {

  switch (segment) {
        case 0:
        tft.fillRect(2 * scaleFactor + pos_x, 0 * scaleFactor + pos_y,  minYdim * scaleFactor, minXdim * scaleFactor, color); // SEG a
         break;
        case 1:
        tft.fillRect(10 * scaleFactor + pos_x, 2 * scaleFactor + pos_y, minXdim * scaleFactor, minYdim * scaleFactor, color); // SEG b
         break;
        case 2:
        tft.fillRect(10 * scaleFactor + pos_x, 12 * scaleFactor + pos_y, minXdim * scaleFactor, minYdim * scaleFactor, color); // SEG c
          break;
        case 3:
        tft.fillRect(2 * scaleFactor + pos_x, 20 * scaleFactor + pos_y, minYdim * scaleFactor, minXdim * scaleFactor,  color); // SEG d
          break;
        case 4:
        tft.fillRect(0 * scaleFactor + pos_x, 12 * scaleFactor + pos_y, minXdim * scaleFactor, minYdim * scaleFactor,  color); // SEG e
          break;
        case 5:
        tft.fillRect(0 * scaleFactor + pos_x, 2 * scaleFactor + pos_y, minXdim * scaleFactor, minYdim * scaleFactor, color); // SEG f
          break;
        case 6:
        tft.fillRect(2 * scaleFactor + pos_x, 10 * scaleFactor + pos_y, minYdim * scaleFactor, minXdim * scaleFactor, color); // SEG g
          break;
        case 7: 
        tft.fillRect(-5 * scaleFactor + pos_x,  20 * scaleFactor + pos_y, minXdim * scaleFactor,  minXdim * scaleFactor, color); // . (DP)  // ori 14* scaleFactor + posx_x
         break;
      }
}
 
