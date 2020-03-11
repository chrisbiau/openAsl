/****************************************************************************************/
/* GPS Logger with ESP SCREEN OPENASL                                                   */
/* Version: 1.0                                                                         */
/* Date: 11.03.2020                                                                     */
/*                                                                                      */
/*      Special features:                                                               */
/*                        Key A: Change language between German and Englis              */
/*                        Key B: Changes format of location data                        */
/*                        Key C: Change brightness of display                           */
/*                                                                                      */
/*      Used library:                                                                   */
/*                        Tiny GPS Plus: https://github.com/mikalhart/TinyGPSPlus       */ 
/*                                                                                      */
/*      Base on source code:                                                            */
/* Simple GPS Tracker: https://m5stack.hackster.io/ptschulik/simple-gps-tracker-d3500e  */
/*                                                                                      */
/****************************************************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <TinyGPS++.h>                                        /* Include Tiny GPS++ library */


#define TFT_CS     15 // @param    cs    Chip select pin #
#define TFT_DC     33 // @param    dc    Data/Command pin # 
#define TFT_MOSI   18 // @param    mosi  SPI MOSI pin #
#define TFT_SCLK   5  // @param    sclk  SPI Clock pin #
#define TFT_RST    4  // @param    rst   Reset pin # (optional, pass -1 if unused)
#define TFT_MISO   19 // @param    miso  SPI MISO pin # (optional, pass -1 if unused)*/


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

/* Constants */
const byte DELAY_READ = 10;                                   /* Delay between two reads: 10 = 100ms */
const byte DELAY_CHECK_CONST = 100;                           /* Delay to check if any data received 100 * 10 = 1 sec */
const byte MINUMUM_SAT = 4;                                   /* Minumum number of satellites before fix is accepted */
const byte UTC_CET = 2;                                       /* Time difference from UTC to CET */


TinyGPSPlus gps;                                              /* Reference the TinyGPS++ object */
HardwareSerial GPSRaw(2);                                     /* By default, GPS is connected with M5Core through UART2 */

/* Variables */
boolean UPDATE_FLAG;                                          /* Display update flag: TRUE = Update, FALSE = No update */
boolean LOC_UPDATE_FLAG;                                      /* Location update flag: TRUE = Update, FALSE = No update */
boolean ALT_UPDATE_FLAG;                                      /* Altitude update flag: TRUE = Update, FALSE = No update */
boolean SPEED_UPDATE_FLAG;                                    /* Speed update flag: TRUE = Update, FALSE = No update */
boolean SAT_UPDATE_FLAG;                                      /* Nr. of sattelites update flag: TRUE = Update, FALSE = No update */
boolean HDOP_UPDATE_FLAG;                                     /* HDOP update flag: TRUE = Update, FALSE = No update */
byte LOOP_COUNTER;                                            /* Generic loop counter for delay */
byte DELAY_CHECK = 0;                                         /* Delay to check if any data received */
byte LCD_BRIGHTNESS = 250;                                    /* LCD brightness value (0-250) */
byte MENU_LANGUAGE = 1;                                       /* Menu Language 0 = German, 1 = English */
byte GPS_FORMAT = 0;                                          /* GPS format 0 = Decimal degree, 1= Degree, Minutes, Seconds */   
byte GPS_STATUS;                                              /* GPS status: 0 = No GPS receiver detected, 1 = No valid GPS fix, 2 = Valid GPS data */   
unsigned int NO_OF_SAT;                                       /* Number of satellites */ 
unsigned int CHARS_PROCESSED = 0;                             /* Number of procesed chars */                       
unsigned int OLD_CHARS_PROCESSED = 1;                         /* Number of procesed chars */ 
unsigned int OLD_NO_OF_SAT;                                   /* Old number of satellites */ 
unsigned int OLD_HDOP;                                        /* Old HDOP value */
int OLD_SEC = 0;                                              /* Old second */  
int OLD_ALTITUDE;                                             /* Old altitude value */
int OLD_SPEED;                                                /* Old speed value */                                       
float OLD_LOC_LAT;                                            /* Old lateral location */ 
float OLD_LOC_LNG;                                            /* Old longitudinal location */
/* Table for day of month */
unsigned char DAY_OF_MONTH[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30,31};
char CONVERTED[32];                                           /* Conversion string for display */



 


/****************************************************************************************/
/* Init routine                                                                         */
/****************************************************************************************/
void setup() 
{ 
  GPSRaw.begin(9600);                                         /* Init GPS serial interface */
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);							 /* Set initial LCD brightness */
                    
  delay (2000);                                               /* 2000ms init delay */
  UPDATE_FLAG = true;                                         /* Set update flag true */
  LOC_UPDATE_FLAG = true;                                     /* Set location update flag true */
}


/****************************************************************************************/
/* Main routine                                                                         */
/****************************************************************************************/
void loop() 
{
  while (GPSRaw.available() > 0)                              /* Check if new GP data is available */
  {
    gps.encode(GPSRaw.read());                                /* Read until no more data is available */
  }

  CHARS_PROCESSED = gps.charsProcessed();                     /* Read processed chars */

  /* No chars processed or no more data received --> Checked every 10ms ? */
  if ((CHARS_PROCESSED  < 10)|| (CHARS_PROCESSED == OLD_CHARS_PROCESSED))                                   
  {
    if (DELAY_CHECK < 230)                                    /* Prevent variable from overflow */
    {
      DELAY_CHECK = DELAY_CHECK + 10;                         /* Increase delay check if any data received each 10ms by 10 */
    }
  }
  else                                                        /* No chars received ? */
  {
    DELAY_CHECK = 0;                                          /* Reset delay check if any data received */
  }

  /* Case 1: Timeout */
  if (DELAY_CHECK > DELAY_CHECK_CONST)                     
  {
    if (GPS_STATUS > 0)                                       /* Was already in an other GPS status ? */
    {
      UPDATE_FLAG = true;                                     /* Set update flag true */ 
    }
    if (UPDATE_FLAG == true)                                  /* Update flag true ? */
    {
      SUB_DISPLAY_NO_REC();                                   /* Call sub routine to display no receiver error message */ 
      UPDATE_FLAG = false;                                    /* Set update flag false */
    } 
    GPS_STATUS = 0;                                           /* Set GPS status 0 */
  }
  else                                                        /* New data received correctly ? */
  {
    if (GPS_STATUS == 0)                                      /* GPS state 0 ? */
    {
      GPS_STATUS = 1;                                         /* Set GPS status 1 */
      UPDATE_FLAG = true;                                     /* Set update flag true */
    }
  }

  /* Case 2: Receiver found and data are received but no GPS signal --> Status 1 */
  if (GPS_STATUS == 1)                                        /* GPS status is 1 ? */
  {
    /* Check number of satellites */
    if (gps.satellites.isValid() == true)                     /* Valid GPS number of satellites received ? */
    {
      NO_OF_SAT = gps.satellites.value();                     /* Save number of satellites */         
    }
    else                                                      /* Not valid GPS number of satellites received ? */
    {
      NO_OF_SAT = 0;                                          /* Set number of satellites to 0 */
    }
    /* Decide on number of satellites received */
    if (NO_OF_SAT < MINUMUM_SAT)                              /* Too less satelites received ? */
    {
      if (UPDATE_FLAG == true)                                /* Update flag true ? */
      {
        SUB_DISPLAY_NO_GPS();                                 /* Call sub routine to display no GPS message */ 
        UPDATE_FLAG = false;                                  /* Set update flag false */
      } 
    }
    else                                                      /* Correct number of satellites received? */
    {
      GPS_STATUS = 2;                                         /* Set GPS status 2 --> Fix received */
      UPDATE_FLAG = true;                                     /* Set update flag true */
      UPDATE_FLAG = true;                                     /* Set display update flag true */
      LOC_UPDATE_FLAG = true;                                 /* Set location update flag true */ 
      ALT_UPDATE_FLAG = true;                                 /* Set altitude update flag true */
      SPEED_UPDATE_FLAG = true;                               /* Set speed update flag true */
      SAT_UPDATE_FLAG = true;                                 /* Set number of sattelites update flag true */
      HDOP_UPDATE_FLAG = true;                                /* Set HDOP update flag true */
    } 
  }

   /* Case 2: Enough satellites received ? --> Valid fix*/
  if (GPS_STATUS == 2)                                        /* GPS status is 2 ? */ 
  {
    /* Check number of satellites */
    if (gps.satellites.isValid() == true)                     /* Valid GPS number of sattllites received ? */
    {
      NO_OF_SAT = gps.satellites.value();                     /* Save number of satellites */  
    }
    else                                                      /* Not valid GPS number of sattelites received ? */
    {
      NO_OF_SAT = 0;                                          /* Set number of satellites to 0 */
    }
    if (NO_OF_SAT < MINUMUM_SAT)                              /* Too less satelites received ? */
    {   
      GPS_STATUS = 1;                                         /* Set GPS status 1 */
      UPDATE_FLAG = true;                                     /* Set update flag true */
    } 
    else                                                      /* Correct number of satellites received? */
    {
      if (UPDATE_FLAG == true)                                /* Update flag true ? */
      {
        SUB_DISPLAY_MAIN();                                   /* Call sub routine to display main menu */ 
        UPDATE_FLAG = false;                                  /* Set update flag false */
      }       
           
      SUB_DISPLAY_DATE_TIME();                                /* Call sub routine to display time and date info */
      SUB_DISPLAY_LOCATION();                                 /* Call sub routine to display location */
      SUB_DISPLAY_ALTITUDE();                                 /* Call sub routine to display altitude */
      SUB_DISPLAY_SPEED();                                    /* Call sub routine to display speed */
      SUB_DISPLAY_SAT();                                      /* Call sub routine to display number of sattelites */
      SUB_DISPLAY_HDOP();                                     /* Call sub routine to display HDOP value */
    }
  } 

  OLD_CHARS_PROCESSED = CHARS_PROCESSED;


  /* Delay as defined in delay read const and check on key pressed every 10ms */
//  for (LOOP_COUNTER = 0; LOOP_COUNTER < DELAY_READ; LOOP_COUNTER++)
//  {
//    delay (10);                                               /* 10 ms delay */
//    M5.update();                                              /* Update on reading keys */

//    if (M5.BtnA.wasPressed() == true)                         /* Button A pressed ? --> Change language */
//    {
 //      M5.Speaker.tone(NOTE_DH2, 1);                         
//      if (MENU_LANGUAGE == 0)                                 /* German selected ? */
//      {
//        MENU_LANGUAGE = 1;                                    /* Set menu language to English */
//        UPDATE_FLAG = true;                                   /* Set display update flag true */
//        LOC_UPDATE_FLAG = true;                               /* Set location update flag true */ 
//        ALT_UPDATE_FLAG = true;                               /* Set altitude update flag true */
//        SPEED_UPDATE_FLAG = true;                             /* Set speed update flag true */
//        SAT_UPDATE_FLAG = true;                               /* Set number of sattelites update flag true */
//        HDOP_UPDATE_FLAG = true;                              /* Set HDOP update flag true */
//      }
//      else                                                    /* English selected ? */
//      {
//        MENU_LANGUAGE = 0;                                    /* Set menu language to German */
//        UPDATE_FLAG = true;                                   /* Set display update flag true */
//        LOC_UPDATE_FLAG = true;                               /* Set location update flag true */ 
//        ALT_UPDATE_FLAG = true;                               /* Set altitude update flag true */
//        SPEED_UPDATE_FLAG = true;                             /* Set speed update flag true */
//        SAT_UPDATE_FLAG = true;                               /* Set number of sattelites update flag true */
//        HDOP_UPDATE_FLAG = true;                              /* Set HDOP update flag true */
//      }
//    }

//    if (M5.BtnB.wasPressed() == true)                         /* Button B pressed ? --> Change format */
//    {
 //      M5.Speaker.tone(NOTE_DH2, 1);                         
//      if (GPS_FORMAT == 0)                                    /* Format 0 selected ? */
//      {
//        GPS_FORMAT = 1;                                       /* Set Format to 1 */
//        LOC_UPDATE_FLAG = true;                               /* Set location update flag true */
//      }
//      else                                                    /* Format 1 selected ? */
//      {
//        GPS_FORMAT = 0;                                       /* Set Format to 0 */
//        LOC_UPDATE_FLAG = true;                               /* Set location update flag true */
//      }
//    }
    
//    if (M5.BtnC.wasPressed() == true)                         /* Button C pressed ? --> Change brightness */
//    {
//      M5.Speaker.tone(NOTE_DH2, 1);                         
//      if (LCD_BRIGHTNESS < 250)                               /* Maximum brightness not reached ? */
//      {
//        LCD_BRIGHTNESS = LCD_BRIGHTNESS + 10;                 /* Increase brightness */
//      }
//      else                                                    /* Maximum brightness reached ? */
//      {
//        LCD_BRIGHTNESS = 10;                                  /* Set brightness to lowest value */
//      }
//      tft.setBrightness(LCD_BRIGHTNESS);                   /* Change brightness value */
//   }
//  }
}


/****************************************************************************************/
/* SUBROUTINE Display Date and Time Information                                         */
/* This subroutine displays the time and date information and corrects the time zone    */
/****************************************************************************************/
void SUB_DISPLAY_DATE_TIME ()
{
  boolean TIME_VALID;                                         /* True = Valid time, False = not valid */
  boolean DATE_VALID;                                         /* True = Valid date, False = not valid */
  boolean TIME_DATE_UPDATE;                                   /* True = Needs update, False = Needs no update */
  byte NTP_TIME_ZONE;                                         /* Variable for time zone: 0 = UTC time, 1 = CET winter time, 2 = CET summer time */
  int BEGIN_DST_DATE;                                         /* Begin date summer time */
  int BEGIN_DST_MONTH;                                        /* Begin month summer time */
  int END_DST_DATE;                                           /* End date summer time */
  int END_DST_MONTH;                                          /* End month summer time */
  int ACT_YEAR;                                               /* Actual year */                               
  int ACT_MONTH;                                              /* Actual month */ 
  int ACT_DAY;                                                /* Actual day */ 
  int ACT_HOUR;                                               /* Actual hour */ 
  int ACT_MIN;                                                /* Actual minute */ 
  int ACT_SEC;                                                /* Actual second */

  TIME_DATE_UPDATE = false;                                   /* No update needed */
 
  if (gps.time.isValid())                                     /* Valid time available ? */
  {
    ACT_HOUR = gps.time.hour();                               /* Get actual hour */
    ACT_MIN = gps.time.minute();                              /* Get actual minute */
    ACT_SEC = gps.time.second();                              /* Get actual second */
    TIME_VALID = true;                                        /* Set valid time flag true */
    TIME_DATE_UPDATE = true;                                  /* Update needed */
  }                                                           
  else                                                        /* Valid time not available ? */
  {
    TIME_VALID = false;                                       /* Set valid time flag false */
  }

  if (gps.date.isValid())                                     /* Valid date available ? */
  {
    ACT_YEAR = gps.date.year();                               /* Get actual year */
    ACT_MONTH = gps.date.month();                             /* Get actual month */    
    ACT_DAY = gps.date.day();                                 /* Get actual day */
    DATE_VALID = true;                                        /* Set valid time flag true */
    TIME_DATE_UPDATE = true;                                  /* Update needed */
  }                                                           
  else                                                        /* Valid time not available ? */
  {
    DATE_VALID = false;                                       /* Set valid date flag false */
  }

  /* Changed second and valid date/time --> Update display */
  if ((OLD_SEC != ACT_SEC) && (TIME_VALID == true) && (DATE_VALID == true))
  {
    OLD_SEC = ACT_SEC;                                        /* Save old second */

    /* Step 1: Leap year? --> Adjust February */
    if (ACT_YEAR%400 == 0 || (ACT_YEAR%4 == 0 && ACT_YEAR%100 !=0))
    {
      DAY_OF_MONTH[1] = 29;                                   /* Set day of month to 29 */
    }
    else   
    {                                                    
      DAY_OF_MONTH[1] = 28;                                   /* Set day of month to 28 */
    }

    /* Step 2: Determine start and end date of summer time */
    BEGIN_DST_DATE=  (31 - (5* ACT_YEAR /4 + 4) % 7);         /* Determine last Sunday of March */
    BEGIN_DST_MONTH = 3;                                      /* Begin month is March */
    END_DST_DATE= (31 - (5 * ACT_YEAR /4 + 1) % 7);           /* Determine last Sunday of October */
    END_DST_MONTH = 10;                                       /* Begin month is October */
  
    /* Step 3: Check for summer time */
    if (((ACT_MONTH > BEGIN_DST_MONTH) && (ACT_MONTH < END_DST_MONTH))
       || ((ACT_MONTH == BEGIN_DST_MONTH) && (ACT_DAY >= BEGIN_DST_DATE))
       || ((ACT_MONTH == END_DST_MONTH) && (ACT_DAY <= END_DST_DATE)))
    {
      NTP_TIME_ZONE = UTC_CET;                                /* Set time zone for CET = UTC + 2 hour */
    }
    else                                                      /* Winter time ? */
    {
      NTP_TIME_ZONE = UTC_CET-1;                              /* Set time zone for CET = UTC +1 hour */
    }

    /* Step 4: Generate correct adjusted time */
    ACT_HOUR = ACT_HOUR + NTP_TIME_ZONE;            
    if (ACT_HOUR > 23)                                        /* Next day ? */
    {
      ACT_HOUR = ACT_HOUR - 23;                               /* Correct hour */
      if (ACT_DAY = DAY_OF_MONTH [ACT_MONTH-1])               /* Last day of month ? */
      {
        ACT_DAY = 1;                                          /* Set day to 1st of month */
        ACT_MONTH = ACT_MONTH +1;                             /* Increase month */
        if (ACT_MONTH > 12)                                   /* Beyond December ? */
        {
          ACT_MONTH = 1;                                      /* Set actual month to January */
          ACT_YEAR = ACT_YEAR + 1;                            /* Increase year */
        }
      }
    }
    
    /* Step 4: Display actual time and date according chosen language */
    tft.fillRect(0, 215, 320, 25, 0x439);                  /* Clear old time and date */
    tft.setTextSize(2);                                    /* Set text size to 2 */
    tft.setCursor(10, 219);                                /* Position cursor to display time */ 
    /* Convert into dispaly string */
    sprintf(CONVERTED, "%02d:%02d:%02d ", ACT_HOUR, ACT_MIN, ACT_SEC);
    tft.print(CONVERTED);                                  /* Display converted time string */
    tft.setCursor(190, 219);                               /* Position cursor to display date */
    if (MENU_LANGUAGE == 0)                                   /* German language chosen ? */
    {    
      /* Convert into dispaly string */
      sprintf(CONVERTED, "%02d.%02d.%02d ", ACT_DAY, ACT_MONTH, ACT_YEAR);
    }
    else                                                      /*Englishn language chosen ? */         
    {
      /* Convert into dispaly string */
      sprintf(CONVERTED, "%02d/%02d/%02d ", ACT_YEAR, ACT_MONTH, ACT_DAY);
    }
    tft.print(CONVERTED);                                  /* Display converted date string */
  }

  /* Time or date not correctly received */
  if ((TIME_VALID == false) || (DATE_VALID == false))
  {
    if (TIME_DATE_UPDATE == true)                             /* Update needed ? */
    {
      tft.fillRect(0, 215, 320, 25, 0x439);                /* Clear old time and date */
      tft.setTextSize(2);                                  /* Set text size to 2 */
      tft.print(F("--:--:--"));                            /* Dispaly time placeholder */
      tft.setCursor(190, 219);                             /* Position cursor to display date */
      if (MENU_LANGUAGE == 0)                                 /* German language chosen ? */
      {
        tft.print(F("--.--.----"));                        /* Display date placeholder */
      }  
      else                                                    /*Englishn language chosen ? */         
      {
        tft.print(F("----/--/--"));                        /* Display date placeholder */
      }
      TIME_DATE_UPDATE = false;                               /* Set date update false */
    }
  }
}


/****************************************************************************************/
/* SUBROUTINE Display Location                                                          */
/* This subroutine displays the location information                                    */
/****************************************************************************************/
void SUB_DISPLAY_LOCATION ()
{
  boolean LOC_FAIL_UPDATE;                                    /* True = Needs update, False = Needs no update */
  float ACT_LOC_LAT;                                          /* Actual lateral location */ 
  float ACT_LOC_LNG;                                          /* Actual longitudinal location */
  float ROUNDED_VAL;                                          /* Rounded floating value */
  uint16_t CONVERT_DATA;                                      /* Converted data value */

  LOC_FAIL_UPDATE = false;                                    /* No update needed */
  
  if (gps.location.isValid() == true)                         /* Valid GPS location received ? */
  {
    LOC_FAIL_UPDATE = true;                                   /* Update needed */
    tft.setTextSize(3);                                    /* Set text size to 3 */
    ACT_LOC_LAT = gps.location.lat();                         /* Get lattitude value */
    ACT_LOC_LNG = gps.location.lng();                         /* Get longitude value */

    /* Display lattitude value */
    ROUNDED_VAL = SUB_ROUND_FLOAT_VALUE (ACT_LOC_LAT, 5);     /* Call subroutine to round float to 5 digits */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_LOC_LAT != ROUNDED_VAL) || (LOC_UPDATE_FLAG == true))                          
    {
      OLD_LOC_LAT = ROUNDED_VAL;                              /* Save value */
      tft.fillRect(114, 40, 169, 44, 0x1E9F);              /* Clear old degree of lattitude value */
      tft.setCursor(115, 50);                              /* Position cursor */
      if (GPS_FORMAT == 0)                                    /* Format 0 selected */
      {     
        tft.print(ACT_LOC_LAT, 6);                         /* Display latitude information */
      }
      else                                                    /* Format 1 selected */
      {
        CONVERT_DATA = (int)(ACT_LOC_LAT);                    /* Extract degree value */
        tft.print(CONVERT_DATA);                           /* Display degree value */
        tft.print(" ");
        /* Extract Minute value */
        ACT_LOC_LAT= ACT_LOC_LAT - CONVERT_DATA;
        ACT_LOC_LAT = ACT_LOC_LAT * 60; 
        CONVERT_DATA = (int)(ACT_LOC_LAT);
        tft.print(CONVERT_DATA);                           /* Display minute value */
        tft.print(" ");
        /*Extract Second value */
        ACT_LOC_LAT= ACT_LOC_LAT - CONVERT_DATA;
        ACT_LOC_LAT = ACT_LOC_LAT * 60;  
        tft.print (ACT_LOC_LAT, 0);                        /* Dispaly second value */
      }
    }
    
    /* Display lngitude value */
    ROUNDED_VAL = SUB_ROUND_FLOAT_VALUE (ACT_LOC_LNG, 5);     /* Call subroutine to round float to 5 digits */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_LOC_LNG != ROUNDED_VAL) || (LOC_UPDATE_FLAG == true))
    {
      OLD_LOC_LNG = ROUNDED_VAL;                              /* Save value */
      tft.fillRect(114, 94, 169, 44, 0x1E9F);              /* Clear old degree of longitude value */
      tft.setCursor(115, 104);                             /* Position cursor */
      if (GPS_FORMAT == 0)                                    /* Format 0 selected */
      {     
        tft.print(ACT_LOC_LNG, 6);                         /* Display longitudinal information */
      }
      else                                                    /* Format 1 selected */
      {
        CONVERT_DATA = (int)(ACT_LOC_LNG);                    /* Extract degree value */
        tft.print(CONVERT_DATA);                           /* Display degree value */
        tft.print(" ");
        /* Extract Minute value */
        ACT_LOC_LNG= ACT_LOC_LNG - CONVERT_DATA;
        ACT_LOC_LNG = ACT_LOC_LNG * 60; 
        CONVERT_DATA = (int)(ACT_LOC_LNG);
        tft.print(CONVERT_DATA);                           /* Display minute value */
        tft.print(" ");
        /*Extract Second value */
        ACT_LOC_LNG= ACT_LOC_LNG - CONVERT_DATA;
        ACT_LOC_LNG = ACT_LOC_LNG * 60;  
        tft.print (ACT_LOC_LNG, 0);                        /* Dispaly second value */
      }
    }
    LOC_UPDATE_FLAG = false;                                  /* Set location update flag false */
  }
  else                                                        /* Not valid GPS location received ? */    
  {
    if (LOC_FAIL_UPDATE == true)                              /* Update needed ? */
    { 
      tft.setTextSize(3);
      tft.fillRect(114, 40, 169, 44, 0x1E9F);              /* Clear old degree of lattitude value */
      tft.fillRect(114, 94, 169, 44, 0x1E9F);              /* Clear old degree of longitude value */
      if (GPS_FORMAT == 0)                                    /* Format 0 selected */
      {
        tft.setCursor(115, 50);                            /* Display location placeholder */
        tft.print("--.------");
        tft.setCursor(115, 104);
        tft.print("--.------");
      }
      else                                                    /* Format 1 selected */
      {
        tft.setCursor(115, 50);                            /* Display location placeholder */
        tft.print("----'---");
        tft.setCursor(115, 104);
        tft.print("----'---");
      }      
    }
    LOC_FAIL_UPDATE = false;                                  /* Set date update false */
  }
}


/****************************************************************************************/
/* SUBROUTINE Display Altitude                                                          */
/* This subroutine displays the altitude information                                    */
/****************************************************************************************/
void SUB_DISPLAY_ALTITUDE ()
{
  boolean ALT_FAIL_UPDATE;                                    /* True = Needs update, False = Needs no update */
  float ACT_ALTITUDE;                                         /* Actual laltitude value */ 
  int INTEGER_VAL;                                            /* Integer value */
  int STRING_LENGTH;                                          /* Length of string */

  ALT_FAIL_UPDATE = false;                                    /* No update needed */
  
  if (gps.altitude.isValid() == true)                         /* Valid GPS altitude received ? */
  {
    ALT_FAIL_UPDATE = true;                                   /* Update needed */
    tft.setTextSize(3);                                    /* Set text size to 3 */
    ACT_ALTITUDE = gps.altitude.meters();                     /* Get altitude value */

    /* Display altitude value */
    INTEGER_VAL = (int)(ACT_ALTITUDE);                        /* Extract integer value of altitude variable */
    sprintf(CONVERTED, "%.0f", ACT_ALTITUDE);                 /* Convert float to a string to determine length */
    STRING_LENGTH = strlen(CONVERTED);                        /* Determine length of string */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_ALTITUDE != INTEGER_VAL) || (ALT_UPDATE_FLAG == true))                          
    {
      OLD_ALTITUDE = INTEGER_VAL;                             /* Save value */
      tft.fillRect(4, 145, 100, 40, 0x1E9F);               /* Clear old altitude value */
      if (STRING_LENGTH >= 4)                                 /* String length >= 4 ? */
        tft.setCursor(8, 155);                             /* Position cursor */
      if (STRING_LENGTH == 3)                                 /* String length = 3 ? */
        tft.setCursor(18, 155);                            /* Position cursor */
      if (STRING_LENGTH == 2)                                 /* String length = 2 ? */
        tft.setCursor(25, 155);                            /* Position cursor */
      if (STRING_LENGTH<= 1)                                  /* String length <= 1 ? */ 
        tft.setCursor(35, 155);                            /* Position cursor */      
      tft.print(CONVERTED);                                /* Display value */
      tft.print("m");
    } 
    ALT_UPDATE_FLAG = false;                                  /* Set altitrude update flag false */     
  }
  else                                                        /* Not valid GPS altitude received ? */
  {
    if (ALT_FAIL_UPDATE == true)                              /* Update needed ? */
    { 
      tft.setTextSize(3);                                  /* Set text size to 3 */
      tft.fillRect(4, 145, 100, 40, 0x1E9F);               /* Clear old altitude value */
      tft.setCursor(18, 155);                              /* Position cursor */ 
      tft.print("---m");                                   /* Display altitude placeholder */
    }
    ALT_FAIL_UPDATE = false;                                  /* Set altitude update false */
  }
}


/****************************************************************************************/
/* SUBROUTINE Display Speed                                                             */
/* This subroutine displays the speed information                                       */
/****************************************************************************************/
void SUB_DISPLAY_SPEED ()
{
  boolean SPEED_FAIL_UPDATE;                                  /* True = Needs update, False = Needs no update */
  float ACT_SPEED;                                            /* Actual speed value */ 
  int INTEGER_VAL;                                            /* Integer value */
  int STRING_LENGTH;                                          /* Length of string */

  SPEED_FAIL_UPDATE = false;                                  /* No update needed */
  
  if (gps.speed.isValid() == true)                            /* Valid GPS speed received ? */
  {
    SPEED_FAIL_UPDATE = true;                                 /* Update needed */
    tft.setTextSize(3);                                    /* Set text size to 3 */
    ACT_SPEED = gps.speed.kmph();                             /* Get speed value */

    /* Display speed value */
    INTEGER_VAL = (int)(ACT_SPEED);                           /* Extract integer value of speed variable */
    sprintf(CONVERTED, "%.0f", ACT_SPEED);                    /* Convert float to a string to determine length */
    STRING_LENGTH = strlen(CONVERTED);                        /* Determine length of string */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_SPEED != INTEGER_VAL) || (SPEED_UPDATE_FLAG == true))                          
    {
      OLD_SPEED = INTEGER_VAL;                                /* Save value */
      tft.fillRect(114, 145, 94, 40, 0x1E9F);              /* Clear old speed value */
      if (STRING_LENGTH >= 4)                                 /* String length >= 4 ? */
        tft.setCursor(125, 155);                           /* Position cursor */
      if (STRING_LENGTH == 3)                                 /* String length = 3 ? */
        tft.setCursor(135, 155);                           /* Position cursor */
      if (STRING_LENGTH == 2)                                 /* String length = 2 ? */
        tft.setCursor(142, 155);                           /* Position cursor */
      if (STRING_LENGTH<= 1)                                  /* String length <= 1 ? */ 
        tft.setCursor(152, 155);                           /* Position cursor */      
      tft.print(CONVERTED);                                /* Display value */
    }  
    SPEED_UPDATE_FLAG = false;                                /* Set speed update flag false */     
  }
  else                                                        /* Not valid GPS altitude received ? */
  {
    if (SPEED_FAIL_UPDATE == true)                            /* Update needed ? */
    { 
      tft.setTextSize(3);                                  /* Set text size to 3 */
      tft.fillRect(114, 145, 94, 40, 0x1E9F);              /* Clear old speed value */
      tft.setCursor(135, 155);                             /* Position cursor */ 
      tft.print("---");                                    /* Display speed placeholder */
    }
    SPEED_FAIL_UPDATE = false;                                /* Set speed update false */
  }
}


/****************************************************************************************/
/* SUBROUTINE Display Sattelites                                                         */
/* This subroutine displays the number of sattelites                                    */
/****************************************************************************************/
void SUB_DISPLAY_SAT ()
{
  boolean SAT_FAIL_UPDATE;                                    /* True = Needs update, False = Needs no update */

  SAT_FAIL_UPDATE = false;                                    /* No update needed */

  if (gps.satellites.isValid() == true)                       /* Valid GPS number of sattllites received ? */
  {
    SAT_FAIL_UPDATE = true;                                   /* Update needed */
    tft.setTextSize(2);                                    /* Set text size to 2 */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_NO_OF_SAT != NO_OF_SAT) || (SAT_UPDATE_FLAG == true))                          
    {
      OLD_NO_OF_SAT = NO_OF_SAT;                              /* Save value */
      tft.fillRect(275, 145, 45, 35, 0x1E9F);              /* Clear satelite field */
      tft.setCursor(280, 155);                             /* Display header info */
      tft.print(NO_OF_SAT);                                /* Display number of sattelites */      
    }
    SAT_UPDATE_FLAG = false;                                  /* Set speed update flag false */ 
  }
  else                                                        /* Not valid GPS altitude received ? */
  {
    if (SAT_FAIL_UPDATE == true)                              /* Update needed ? */
    { 
      tft.fillRect(275, 145, 45, 35, 0x1E9F);              /* Clear satelite field */
      tft.setCursor(280, 155);                             /* Display header info */
      tft.print("-");                                      /* Display sattelite placeholder */
    }
    SAT_FAIL_UPDATE = false;                                  /* Set sattelite update false */
  }
}


/****************************************************************************************/
/* SUBROUTINE Display HDOP                                                              */
/* This subroutine displays the HDOP value                                              */
/****************************************************************************************/
void SUB_DISPLAY_HDOP ()
{
  boolean HDOP_FAIL_UPDATE;                                    /* True = Needs update, False = Needs no update */
  float ACT_HDOP;                                              /* Actual HDOP value */
  unsigned int INTEGER_VAL;                                    /* Integer value */

  HDOP_FAIL_UPDATE = false;                                    /* No update needed */

  if (gps.hdop.isValid() == true)                             /* Valid GPS HDOP value received ? */
  {
    HDOP_FAIL_UPDATE = true;                                  /* Update needed */
    tft.setTextSize(2);                                    /* Set text size to 2 */
    ACT_HDOP = gps.hdop.hdop();                               /* Get HDOP value */
    INTEGER_VAL = (int)(ACT_HDOP);                            /* Extract integer value of HDOP variable */
    /* Value has changed for updating display or location update flag true? */
    if ((OLD_HDOP != INTEGER_VAL) || (HDOP_UPDATE_FLAG == true))                          
    {
      OLD_HDOP = INTEGER_VAL;                                 /* Save value */
      tft.fillRect(275, 180, 45, 28, 0x1E9F);              /* Clear satelite field */
      tft.setCursor(280, 185);                             /* Display header info */
      tft.print(INTEGER_VAL);                              /* Display HDOP value */      
    }
    HDOP_UPDATE_FLAG = false;                                 /* Set HDOP update flag false */ 
  }
  else                                                        /* Not valid GPS altitude received ? */
  {
    if (HDOP_FAIL_UPDATE == true)                             /* Update needed ? */
    { 
      tft.fillRect(275, 180, 45, 28, 0x1E9F);              /* Clear satelite field */
      tft.setCursor(280, 185);                             /* Display header info */
      tft.print("---");                                    /* Display sattelite placeholder */
    }
    HDOP_FAIL_UPDATE = false;                                 /* Set sattelite update false */
  }
}  


/****************************************************************************************/
/* SUBROUTINE Display Main                                                              */
/* This subroutine displays the main menu based on the selected kanguage                */
/****************************************************************************************/
void SUB_DISPLAY_MAIN (void)
{    
  tft.fillRect(0, 0, 320, 30, 0x439);                      /* Upper dark blue area */
  tft.fillRect(0, 31, 320, 178, 0x1E9F);                   /* Main light blue area */
  tft.fillRect(0, 210, 320, 30, 0x439);                    /* Lower dark blue area */
  tft.fillRect(0, 30, 320, 4, 0xffff);                     /* Upper white line */
  tft.fillRect(0, 208, 320, 4, 0xffff);                    /* Lower white line */
  tft.fillRect(0, 84, 320, 4, 0xffff);                     /* First vertical white line */
  tft.fillRect(0, 140, 320, 4, 0xffff);                    /* Second vertical white line */
  tft.fillRect(106, 140, 4, 72, 0xffff);                   /* First horizontal white line */
  tft.fillRect(210, 140, 4, 72, 0xffff);                   /* First horizontal white line */
  tft.setTextSize(2);                                      /* Set text size to 2 */
  tft.setTextColor(ILI9341_WHITE);                                 /* Set text color to white */
  tft.setCursor(10, 7);                                    /* Display header info */
  tft.print("GPS Logger");
  tft.setCursor(210, 7);
  tft.print("@PT 2019");

  if (MENU_LANGUAGE == 0)                                     /* German language chosen ? */
  {
    tft.setCursor(5, 40);                                  /* Display degree of lattitude */
    tft.print("Breiten-");
    tft.setCursor(5, 65);  
    tft.print("grad");
    tft.setCursor(5, 94);                                  /* Display degree of longitude */
    tft.print("Laengen-");
    tft.setCursor(5, 119);  
    tft.print("grad");
    tft.setCursor(25, 185);                                /* Display elevation info */
    tft.print("Hoehe");
    tft.setCursor(125, 185);                               /* Display hspeed info */
    tft.print("Gesch.");
  }
  else                                                        /*Englishn language chosen ? */         
  {
    tft.setCursor(5, 40);                                  /* Display degree of lattitude */
    tft.print("Degree");
    tft.setCursor(5, 65);  
    tft.print("of Lat.");
    tft.setCursor(5, 94);                                  /* Display degree of longitude */
    tft.print("Degree");
    tft.setCursor(5, 119);  
    tft.print("of Long.");
    tft.setCursor(25, 185);                                /* Display altitude info */
    tft.print("Alti.");
    tft.setCursor(125, 185);                               /* Display hspeed info */
    tft.print("Speed");
  }
  
  tft.setCursor(215, 155);                                 /* Display number of sattelites */
  tft.println(" Sat.");
  tft.setCursor(215, 185);                                 /* Display HDOP info */
  tft.print(" HDOP");
  tft.setTextSize(3);                                      /* Set text size to 3 */
  tft.setCursor(295, 50);                                  /* Display North info */
  tft.print("N");
  if (MENU_LANGUAGE == 0)                                     /* German language chosen ? */
  {  
    tft.setCursor(295, 104);                               /* Display East info */
    tft.print("O");
  }
  else                                                        /* Englishn language chosen ? */         
  {
    tft.setCursor(295, 104);                               /* Display East info */
    tft.print("E");
  }
}

/****************************************************************************************/
/* SUBROUTINE No receiver                                                               */
/* This subroutine displays the screen when no GPS receiver is found                    */
/****************************************************************************************/
void SUB_DISPLAY_NO_REC (void)
{  
  tft.fillRect(0, 0, 320, 30, 0x439);                      /* Upper dark blue area */
  tft.fillRect(0, 31, 320, 178, 0x1E9F);                   /* Main light blue area */
  tft.fillRect(0, 210, 320, 30, 0x439);                    /* Lower dark blue area */  
  tft.setTextSize(3);                                      /* Set text size to 3 */
  tft.setTextColor(ILI9341_WHITE);                                 /* Set text color to white */
  if (MENU_LANGUAGE == 0)                                     /* German language chosen ? */
  {  
    tft.setCursor(80, 80);                                 /* Display message */
    tft.print(F("Kein GPS!"));
    tft.setCursor(40, 120);
    tft.print(F("Pruefe Modul!"));
  }
  else                                                        /* Englishn language chosen ? */         
  { 
    tft.setCursor(100, 80);                                /* Display message */
    tft.print(F("No GPS!"));
    tft.setCursor(40, 120);
    tft.print(F("Check module!"));     
  }
}


/****************************************************************************************/
/* SUBROUTINE No GPS signal                                                             */
/* This subroutine displays the screen when no or a weak GPS signal is available        */
/****************************************************************************************/
void SUB_DISPLAY_NO_GPS (void)
{  
  tft.fillRect(0, 0, 320, 30, 0x439);                      /* Upper dark blue area */
  tft.fillRect(0, 31, 320, 178, 0x1E9F);                   /* Main light blue area */
  tft.fillRect(0, 210, 320, 30, 0x439);                    /* Lower dark blue area */  
  tft.setTextSize(3);                                      /* Set text size to 3 */
  tft.setTextColor(ILI9341_WHITE);                                 /* Set text color to white */
  if (MENU_LANGUAGE == 0)                                     /* German language chosen ? */
  {  
    tft.setCursor(40, 80);                                 /* Display message */
    tft.print(F("Kein valides"));
    tft.setCursor(50, 120);
    tft.print(F("GPS Signal!"));
  }
  else                                                        /* Englishn language chosen ? */         
  { 
    tft.setCursor(70, 80);                                 /* Display message */
    tft.print(F("No valid"));
    tft.setCursor(50, 120);
    tft.print(F("GPS signal!"));     
  }
}

/****************************************************************************************/
/* SUBROUTINE to round floating value                                                   */
/* Input to this subroutine is float value amd number of digits after decimal point     */
/****************************************************************************************/
float SUB_ROUND_FLOAT_VALUE (float FLOAT_IN, byte DIGITS) 
{
  float SHIFTS = 1;                                           /* Define shift value */
  while (DIGITS--)                                            /* Process until digit value is zero */
  {
    SHIFTS *= 10;                                             /* Increase shifts value */
  }
  return floor(FLOAT_IN * SHIFTS + .5) / SHIFTS;              /* Calculated rounded value */
}
