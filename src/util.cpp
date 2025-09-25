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

// Function code definitions SHOULD NOT write into header files.
// This yeald to compile time error(s) when try to use header file in other C7C++ source code modules.
// Reason: Linker complains multiple defined same function code !!!
//
// Every function surrounded with macro POOR_CODING_PRAXIS is trick to patch original header file's
// function to avoid this multiple definition same function code definition .
#define  POOR_CODING_PRAXIS

// Second trick is to use GCC compiler feature to avoid multiple definition error:
// Define varibles as exten:
// - extern diskTrafficInformationType diskTrafficInformation = {}; // measure disk Traffic on ESP32 level
// - extern fileSys fileSystem;

#if 1
//NOTE(s):
// - Use same file system as defined in "Esp32_servers_config.h"
// - #include  "Esp32_servers_config.h"  // This header can not use , reason poor original code
//
#define     FILE_SYSTEM   FILE_SYSTEM_LITTLEFS 
#include  "servers/fileSystem.hpp"
#else
#include  "LittleFS.h"
#endif

#include  "util.h"
#include  "measure.h"          // Object(s):  MEASURE

void dmesg (char *message1);

// External object references 
extern  fileSys  fileSystem;
extern  MEASURE  Measure;

// Publish object
UTIL  Utils;

int   isFirmwareUploaded = 1;  // set false for debug security

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


// #include <time.h>
// Local replacement for getUptime()
static time_t getUptime( void )
{
    #if 1
    uint64_t uptime = esp_timer_get_time();   // [us]
    return (time_t) (uptime / 1000000);
    #else
    // returns the number of seconfs ESP32 has been running
    time_t t = time (NULL);
    return __timeHasBeenSet__ ? t - __startupTime__ :  millis () / 1000; // if the time has already been set, 2023/06/22 21:12:34 is the time when I'm writing this code, any valid time should be greater than this
    #endif
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


static String jsonBeginS( String name, String value )
{
    String s = "{\" + name + \":\"" + value + "\"";
    return s;
}


static String jsonString( String name, String value )
{
    char    text[64];

    snprintf( text, sizeof(text),  ",\"%s\":\"%s\"", name, value );
    return String (text);
}


static String jsonValue3( String name, int value, int decimal )
{
    char    text[64];
    char    sign       =  (value >= 0) ? '+' : '-';
    int     integer;
    int     fractional;

    if ( decimal == 1 ) {
        integer    =  abs( value / 10 );
        fractional =  abs( value % 10 );
        sprintf( text, ",\"%s\":\"%c%i.%01i\"", name, sign, integer, fractional );
    }
    else if ( decimal == 2 ) {
        integer    =  abs( value / 100 );
        fractional =  abs( value % 100 );
        sprintf( text, ",\"%s\":\"%c%i.%02i\"", name, sign, integer, fractional );
    }
    else if ( decimal == 3 ) {
        integer    =  abs( value / 1000 );
        fractional =  abs( value % 1000 );
        sprintf( text, ",\"%s\":\"%c%i.%03i\"", name, sign, integer, fractional );
    }
    else {
        sprintf( text, ",\"%s\":\"%c%i\"",      name, sign, abs(value)          );
    }
    return String (text);
}


static String jsonEnd( void )
{
    return  "}\r\n";
}


String UTIL::httpCharge( int args, const char *arg1, const int arg2 )
{
    #define SERVICE   "charge"
    #define IDSTRING  "ESP32_SRV"

    String  s;

    int  uV         = Measure.uV();     // Shunt
    int  mV         = Measure.mV();     // Bus
    int  mA1s       = Measure.mA1s();   // Charging current without efficiency
    int  mAs        = Measure.mAs();    // Charging sum with efficiency
    int  mAh        = mAs / 3600;
    int  Rshunt     = Measure.Rshunt();
    int  offset     = Measure.offset();
    int  scaleI     = Measure.scaleI();
    int  scaleU     = Measure.scaleU();
    int  efficiency = Measure.efficiency();
    int  compU      = Measure.compU();

    if ( args == 0 )
    {
        s = String ( UTIL::charge(0,0) );
        s += " # Up " + UpTime();
        return s;
    }
    if ( (args == 1) && strstr(arg1,"data") )
    {
        s  = jsonBeginS( "id",         IDSTRING       );
        s += jsonValue3( "uV",         uV,          0 );
        s += jsonValue3( "mV",         mV,          3 );
        s += jsonValue3( "mA1s",       mA1s,        3 );
        s += jsonValue3( "mAs",        mAs,         3 );
        s += jsonValue3( "mAh",        mAh,         3 );
        s += jsonValue3( "Rshunt",     Rshunt,      0 );
        s += jsonValue3( "offset",     offset,      0 );
        s += jsonValue3( "efficiency", efficiency,  0 );
        s += jsonValue3( "scaleI",     scaleI,      0 );
        s += jsonValue3( "scaleU",     scaleU,      1 );
        s += jsonValue3( "compU",      compU,       0 );
        s += jsonString( "uptime",     UpTime()       );
        s += jsonEnd();
        return s;
    }
    if ( (args == 1) && strstr(arg1,"setAh") )
    {
        Measure.setAh( arg2 );   // Battery nominal capacity 90 Ah
        return "OK";
    }
    return  "404 Page not Found";
}

// https://randomnerdtutorials.com/esp32-write-data-littlefs-arduino/
// https://github.com/lorol/LITTLEFS/issues/10

const char * UTIL::exportHistory( const char *hours )
{
    #define FLUSH_TRIGGER 0x2000

    MEASURE   data;
    char      fileName[32] = "history.dat";
    int       bytesFlush   = 0;

#if 1  // See example from ftpServer.hpp

    if (!fileSystem.mounted ()) {
        return (char *) "421 file system not mounted\r\n";
    }
    string  fp = fileSystem.makeFullPath (fileName, "/");

    if (fileSystem.isFile (fp)) {
        if (!fileSystem.deleteFile (fp)) {
            return (char *) "452 could not delete file\r\n";

        }
    }
    File  f = fileSystem.open (fp, "w", true);

//  unsigned long fSize = 0;
//  if (f) { fSize = f.size (); f.close (); }

#else
    File f = LittleFS.open(filename, "w");

    if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
    }
#endif

    int  samples = atoi(hours);
    
    if ( samples ) {  samples = samples * 3600 / data.getHistoryPeriod();  }
    else           {  samples =      12 * 3600 / data.getHistoryPeriod();  }

    data.getHistoryData( NULL, 0 );
    for ( int index = -samples; index <= 0; index++ )
    {
        dataset_t dataset;

        data.getHistoryData( &dataset, index );
        f.write( (const uint8_t*) &dataset, sizeof(dataset_t) );
        bytesFlush += sizeof(dataset_t);

        if (bytesFlush > FLUSH_TRIGGER) {
            bytesFlush = 0;
            f.flush();  // No compile time error
          //f.sync();   // Not supported method
        }
        #if 1
        char s[64];
        snprintf(s, sizeof(s), "%s: i=%d mAs=%d mA=%d, mV=%d", __func__, index, dataset.mAs, dataset.mA, dataset.mV);
        dmesg(s);
        #endif
    }
    f.close();
    return "Export done";
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
    string  name = "firmware.bin";
    string  fp   =  fileSystem.makeFullPath (name, "/");

    static char reply[64];

    bool error = false;

    if ( !isFirmwareUploaded ) {
        Serial.println(F("Debug: Return with out rral firmware update"));
        snprintf(reply, sizeof(reply), "Debug: Return with out rral firmware update\n");
        return reply;

    }
    Serial.println(F("The uploaded firmware now stored in FS!"));
    Serial.print(F("\nSearch firmware from FS.."));

//  if ( !fileSystem.isFile (fp) ) { }

    File  firmware = fileSystem.open (fp, "r", false);
    if ( !firmware ) {
        snprintf(reply, sizeof(reply), "Can not open file: %s", fp.c_str());
        return reply;
    }

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

    Serial.println(F("Reset...."));
    delay(2000);   // Wait Serial.print(s) to flow out from serial port...
//  ESP.reset();
    reset( 0 );

    return "FirmwareUploaded status is false";  // Newer should reach here!
}
