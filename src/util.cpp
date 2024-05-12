//
// File:  util.cpp
//

// Ref: how to add application OTA update
// https://lastminuteengineers.com/esp32-ota-updates-arduino-ide/

#include  <WiFi.h>
// hard reset by triggering watchdog
#include  <esp_int_wdt.h>
#include  <esp_task_wdt.h>
#include  <Update.h>
#include  "LittleFS.h"
#include  "util.h"
#include  "measure.h"          // Object(s):  MEASURE

void dmesg (char *message1);

// External object references 
extern  MEASURE  Measure;

// Publish object
UTIL  Utils;

int   isFirmwareUploaded = 0;  // set false for debug security

const char *doFWupdate ( void );

////////////////////////////////////////////////////////
//
//  Constructor
//
/*!
 *  @brief  Instantiates a new INA219 class
 *  @param addr the I2C address the device can be found on. Default is 0x40
 */
UTIL::UTIL()
{
}


/*!
 *  @brief INA219 class destructor
 */
UTIL::~UTIL()
{
}


bool  UTIL::begin( void )
{
    return true;
}


// Firmware update do not support  "*filename"
// Currently use constant filename "/firmware.bin"
//
const char * UTIL::fwupdate( int args, const char *filename )
{
    static char s[64];

    // Debug print
    if ( args ) {
        snprintf(s, sizeof(s), "fwupdate function called with arg1: <%s>", filename);
        Serial.printf("%s", s);
        Serial.println("");
    }

    isFirmwareUploaded = 1;   // Fake to enable update process
    return doFWupdate();
}


// currently "arg1" is not used here
const char * UTIL::charge( int args, const char *arg1 )
{
    static char s[128];

    if ( args ) {
        if ( strstr(arg1, "sta") )
        {
            int stat[22];

            for ( int i = 0; i <= 21; i++ ) {
                stat[i] = Measure.getStat( i );
            }
            snprintf( s, sizeof(s), "stat: %i %i %i %i %i %i %i %i %i %i - %i %i %i %i %i %i %i %i %i %i - %i %i",
                stat[0],  stat[1],  stat[2],  stat[3],  stat[4],  stat[5],  stat[6],  stat[7],  stat[8],  stat[9],
                stat[10], stat[11], stat[12], stat[13], stat[14], stat[15], stat[16], stat[17], stat[18], stat[19],
                stat[20], stat[22] );
                    
            return s;
        }
        return "[charge] invalid argument";
    }

    int  uV    = Measure.uV();     // Shunt
    int  mV    = Measure.mV();     // Bus
    int  mA1s  = Measure.mA1s();   // Charging current without efficiency
    int  mAs   = Measure.mAs();    // Charging sum with efficiency
    int  mAh   = mAs / 3600;
    char sign1 = mA1s >= 0 ? '+' : '-';
    char sign  = mAs  >= 0 ? '+' : '-';

    mA1s = abs( mA1s );
    mAs  = abs( mAs);
    mAh  = abs( mAh );
    snprintf( s, sizeof(s), "charge = %i.%03i  %c%i.%03i  %c%i.%03i  %c%i.%03i  %i", mV / 1000, mV % 1000,
                sign1, mA1s / 1000, mA1s % 1000, sign, mAs / 1000, mAs % 1000, sign, mAh / 1000, mAh % 1000, uV );

    return s;
}


#include <time.h>
static time_t getUptime( void )
{
    uint64_t uptime = esp_timer_get_time();   // [us]

    return (time_t) (uptime / 1000000);
}
static String UpTime( void )
{
    String  s;
    char    c[16];
    time_t  uptime   = getUptime ();
    int     seconds  = uptime % 60;   uptime /= 60;
    int     minutes  = uptime % 60;   uptime /= 60;
    int     hours    = uptime % 24;   uptime /= 24; // uptime now holds days
    int     days     = uptime;

    if (days) {
        s += (char *) itoa (days, c, 10) ;
        s += (char *) " days, ";
    }
    sprintf (c, "%02i:%02i:%02i", hours, minutes, seconds);
    s += String (c);

    return s;
}



static String jsonBeginS( char *name, String value )
{
    String s = "{\" + name + \":\"" + value + "\"";
    return s;
}


static String jsonString( char *name, String value )
{
    char    text[64];

    snprintf( text, sizeof(text),  ",\"%s\":\"%s\"", name, value );
    return String (text);
}


static String jsonValue3( char *name, int value, int decimal )
{
    char    text[64];
    char    sign       =  (value >= 0) ? '+' : '-';
    int     integer    =  abs( value / 1000 );
    int     fractional =  abs( value % 1000 );

    if ( decimal ) {  sprintf( text, ",\"%s\":\"%c%i.%03i\"", name, sign, integer, fractional );  }
    else           {  sprintf( text, ",\"%s\":\"%c%i\"",      name, sign, abs(value)          );  }
    return String (text);
}


static String jsonEnd( void )
{
    return  "}\r\n";
}


String UTIL::httpCharge( int args, const char *arg1 )
{
    #define HOSTNAME  "ESP32_SRV"
    #define SERVICE   "charge"
    #define IDSTRING  HOSTNAME     // HOSTNAME or SERVICE

    String  s;

    int  uV    = Measure.uV();     // Shunt
    int  mV    = Measure.mV();     // Bus
    int  mA1s  = Measure.mA1s();   // Charging current without efficiency
    int  mAs   = Measure.mAs();    // Charging sum with efficiency
    int  mAh   = mAs / 3600;

    if ( args == 0 )
    {
        s = String ( UTIL::charge(0,0) );
        s += " # Up " + UpTime();
        return s;
    }
    if ( (args == 1) && strstr(arg1,"values") )
    {
        s  = jsonBeginS( "id",     IDSTRING   );
        s += jsonValue3( "uV",     uV,      0 );
        s += jsonValue3( "mV",     mV,      3 );
        s += jsonValue3( "mA1s",   mA1s,    3 );
        s += jsonValue3( "mAs",    mAs,     3 );
        s += jsonValue3( "mAh",    mAh,     3 );
        s += jsonString( "uptime", UpTime()   );
        s += jsonEnd();
        return s;
    }
    return  "404 Page not Found";
}

//==================================================================================

void reset( int softReboot )
{
    delay (250);
    if (softReboot) {
        ESP.restart ();
    } else {
        // cause WDT reset
        esp_task_wdt_init (1, true);
        esp_task_wdt_add (NULL);
        while (true);
    }
}


void progressCallBack(size_t currSize, size_t totalSize) {
    Serial.printf("CALLBACK:  Update process at %d of %d bytes...\n", currSize, totalSize);
}


const char * doFWupdate ( void )
{
    static char reply[64];

    bool error = false;

    if ( isFirmwareUploaded )
    {
        Serial.println(F("The uploaded firmware now stored in FS!"));
        Serial.print(F("\nSearch for firmware in FS.."));
        String name = "/firmware.bin";
        File firmware =  LittleFS.open(name, FILE_READ);
        if ( firmware ) {
            Serial.println(F("found!"));
            Serial.println(F("Try to update!"));

            Update.onProgress(progressCallBack);

            Update.begin(firmware.size(), U_FLASH);
            Update.writeStream(firmware);
            if (Update.end()) {
                Serial.println(F("Update finished!"));
            }
            else {
                Serial.println(F("Update error!"));
                Serial.println(Update.getError());
                error = true;
            }

            firmware.close();

            if ( error ) {
                return "Update error!";  // Return to telnet with error message
            }

            String renamed = name;
            renamed.replace(".bin", ".bak");
            if (LittleFS.rename(name, renamed.c_str())) {
                Serial.println(F("Firmware rename succesfully!"));
            }
            else {
                Serial.println(F("Firmware rename error!"));
                error = true;
            }
            if ( error )   // Return to telnet with error message
            {
                return "Firmware update OK, rename error!\r\nRequire reboot";
            }
            delay(2000);   // Wait Serial.print(s) to flow out from serial port...

//          ESP.reset();
            reset( 0 );
        }
        else {
            snprintf(reply, sizeof(reply), "Can not open file: %s", name);
            return reply;
        }
    }
    return "FirmwareUploaded status is false";  // Newer should reach here!
}
