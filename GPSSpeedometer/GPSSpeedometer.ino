/******************************************************************************
                      http://www.soft-collection.com
*******************************************************************************
https://github.com/Bodmer/TFT_eSPI
TFT_eSPI by Bodmer must be installed.
TFT_eSPI config:
1. In User_Setup_Select.h file, select:
  #include <User_Setups/Setup24_ST7789.h>
2. In User_Setups/Setup24_ST7789.h file, must be:
  #define TFT_CS   -1      // Define as not used
  #define TFT_DC   PIN_D2  // Data Command control pin
  #define TFT_RST  PIN_D4  // TFT reset pin (could connect to NodeMCU RST, see next line)
*******************************************************************************
https://github.com/mikalhart/TinyGPS
TinyGPS must be installed.
Tested with TinyGPS 13.
*******************************************************************************
https://github.com/plerup/espsoftwareserial
EspSoftwareSerial must be installed.
*******************************************************************************
Tests string alignment
Normally strings are printed relative to the top left corner but this can be
changed with the setTextDatum() function. The library has #defines for:
TL_DATUM = 0 = Top left
TC_DATUM = 1 = Top centre
TR_DATUM = 2 = Top right
ML_DATUM = 3 = Middle left
MC_DATUM = 4 = Middle centre
MR_DATUM = 5 = Middle right
BL_DATUM = 6 = Bottom left
BC_DATUM = 7 = Bottom centre
BR_DATUM = 8 = Bottom right
L_BASELINE =  9 = Left character baseline (Line the 'A' character would sit on)
C_BASELINE = 10 = Centre character baseline
R_BASELINE = 11 = Right character baseline
So you can use lines to position text like:
  tft.setTextDatum(BC_DATUM); // Set datum to bottom centre
Needs fonts 2, 4, 6, 7 and 8
******************************************************************************/

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

//----------------------------------------
#define GPS_BAUD_RATE                 9600
#define GPS_MAX_ACCEPTABLE_AGE_IN_MS  1500
#define GPS_MIN_ACCEPTABLE_SATELLITES 4
//----------------------------------------
#define SW_SERIAL_UNUSED_PIN  -1
//----------------------------------------
#define SPEED_IS_UNAVAILABLE   0xFFFF
#define COURSE_IS_UNAVAILABLE  0xFFFF
//----------------------------------------
#define SHOW_TRIANGLE
//----------------------------------------
#define SPR_BLACK  0
#define SPR_YELLOW 1
#define SPR_GREEN  2
#define SPR_WHITE  3
//----------------------------------------
#define plus_modulo(value,valueToAdd,modulo) (((value)+(valueToAdd))<(modulo))?((value)+(valueToAdd)):((value)+(valueToAdd)-(modulo))
#define minus_modulo(value,valueToSubtract,modulo) (((value)-(valueToSubtract))>=0)?((value)-(valueToSubtract)):((value)+(modulo)-(valueToSubtract))
//----------------------------------------

//----------------------------------------
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
//----------------------------------------
TinyGPS gps;
//----------------------------------------
SoftwareSerial readerSerial(5, SW_SERIAL_UNUSED_PIN); // RX, TX (Read from GPIO5)
//----------------------------------------
uint16_t speed = SPEED_IS_UNAVAILABLE;
uint8_t sat = 0;
uint16_t course = COURSE_IS_UNAVAILABLE;
//----------------------------------------

void DisplaySpeedOrAzimuth(TFT_eSprite* spr, uint16_t speed_or_azimuth_deg)
{
   spr->setTextWrap(false); 
   spr->setTextSize(2);
   spr->setTextDatum(TC_DATUM);
   spr->setTextPadding(0);   
   spr->setTextColor(SPR_YELLOW, SPR_BLACK);
   int16_t move_left = (((speed_or_azimuth_deg >= 10) && (speed_or_azimuth_deg < 20)) || ((speed_or_azimuth_deg >= 100) && (speed_or_azimuth_deg < 200))) ? 20 : 0;
   String speed_or_azimuth_deg_str = (speed_or_azimuth_deg == SPEED_IS_UNAVAILABLE) ? "---" : String(speed_or_azimuth_deg);
   spr->drawString(speed_or_azimuth_deg_str, 120 - move_left, 50, 7);  
}

void DisplaySat(TFT_eSprite* spr, uint8_t sat)
{
  if (sat > 8) sat = 8;
  for (int8_t i = 0; i < sat; i++)
  {
    spr->fillCircle(230, 10 + ((7 - i) * 20), 4, SPR_GREEN);
  }
}

void DisplayCourse(TFT_eSprite* spr, uint16_t course)
{
   spr->setTextWrap(false);     
   spr->setTextSize(1);
   spr->setTextDatum(TC_DATUM);
   spr->setTextPadding(0);   
   spr->setTextColor(SPR_WHITE, SPR_BLACK);
   if (course == COURSE_IS_UNAVAILABLE) return;
   //--------------------------------------------------------------------------------
   int16_t leftScaleValue = minus_modulo((int16_t)course, TFT_WIDTH / 2, 360);
   int16_t i = 0;
   for (i = 0; (leftScaleValue + i) % 15 != 0; i++);
   for (; i < TFT_WIDTH; i += 15)
   {
      if ((leftScaleValue + i) % 45 == 0)
      {
         switch ((plus_modulo(leftScaleValue , i, 360)) / 45)
         {
            case 0:
               spr->drawString("N", i, TFT_HEIGHT - 50, 4);
               break;
            case 1:
               spr->drawString("ne", i, TFT_HEIGHT - 50, 4);
               break;
            case 2:
               spr->drawString("E", i, TFT_HEIGHT - 50, 4);
               break;
            case 3:
               spr->drawString("se", i, TFT_HEIGHT - 50, 4);
               break;
            case 4:
               spr->drawString("S", i, TFT_HEIGHT - 50, 4);
               break;
            case 5:
               spr->drawString("sw", i, TFT_HEIGHT - 50, 4);
               break;
            case 6:
               spr->drawString("W", i, TFT_HEIGHT - 50, 4);
               break;
            case 7:
               spr->drawString("nw", i, TFT_HEIGHT - 50, 4);
               break;
         }
         spr->fillRect(i - 2, TFT_HEIGHT - 25, 4, 15, SPR_YELLOW);         
      }
      else
      {
         spr->drawLine(i, TFT_HEIGHT - 20, i, TFT_HEIGHT - 10, SPR_WHITE);
      }
#ifdef SHOW_TRIANGLE
      spr->fillTriangle((TFT_WIDTH / 2) - 6, TFT_HEIGHT - 1, (TFT_WIDTH / 2) + 0, TFT_HEIGHT - 10, (TFT_WIDTH / 2) + 6, TFT_HEIGHT - 1, SPR_WHITE);
#endif
   }
}

void setup(void) 
{
  tft.init();
  tft.setRotation(0);
  spr.setColorDepth(4);
  spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
  //--------------------
  uint16_t cmap[4];
  cmap[SPR_BLACK] = TFT_BLACK;
  cmap[SPR_YELLOW] = TFT_YELLOW;
  cmap[SPR_GREEN] = TFT_GREEN;
  cmap[SPR_WHITE] = TFT_WHITE;
  spr.createPalette(cmap);
  //--------------------
  tft.fillScreen(TFT_BLACK);
  //-----------------------------------------
  readerSerial.begin(GPS_BAUD_RATE);
  Serial.begin(115200);
}

void loop()
{
  while(readerSerial.available() > 0) 
  {
    char c = readerSerial.read();
    gps.encode(c);
    //Serial.print(c);
  }    
  //------------------------------------------------------
  unsigned long date;
  unsigned long time;
  unsigned long age;
  gps.get_datetime(&date, &time, &age);
  if (age == TinyGPS::GPS_INVALID_FIX_TIME) //Illegal AGE.
  {
    speed = SPEED_IS_UNAVAILABLE;
    course = COURSE_IS_UNAVAILABLE;
    sat = 0;
  }
  else if (age >= GPS_MAX_ACCEPTABLE_AGE_IN_MS) //AGE too old.
  {
    speed = SPEED_IS_UNAVAILABLE;
    course = COURSE_IS_UNAVAILABLE;
    sat = 0;
  }
  else
  {
    uint8_t tempSATValue = (uint8_t)(gps.satellites());
    sat = (tempSATValue == TinyGPS::GPS_INVALID_SATELLITES) ? 0 : tempSATValue;
    if (tempSATValue >= GPS_MIN_ACCEPTABLE_SATELLITES)
    {
      speed = (uint8_t) (gps.f_speed_kmph());
      course = (uint16_t) (gps.f_course());
    }
    else
    {
      speed = SPEED_IS_UNAVAILABLE;
      course = COURSE_IS_UNAVAILABLE;
    }
  }
  //------------------------------------------------------
  spr.fillSprite(TFT_BLACK);  
  DisplaySpeedOrAzimuth(&spr, speed);
  DisplaySat(&spr, sat);
  DisplayCourse(&spr, course);
  spr.pushSprite(0, 0);
}