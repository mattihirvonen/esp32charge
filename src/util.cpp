//
// File:  util.cpp
//

#include  <WiFi.h>
// hard reset by triggering watchdog
#include  <esp_int_wdt.h>
#include  <esp_task_wdt.h>
#include  <Update.h>
#include  "LittleFS.h"
#include  "util.h"
#include  "measure.h"          // Object(s):  MEASURE

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
    snprintf(s, sizeof(s), "fwupdate function called with arg: <%s>", filename);
    Serial.printf("%s", s);
    Serial.println("");

    isFirmwareUploaded = 1;   // Fake to enable update process
    return doFWupdate();
}


// currently "arg1" is not used here
const char * UTIL::charge( int arg, const char *arg1 ) {

    static char s[64];

    int  mA1s  = Measure.mA1s();   // Charging current without efficiency
    int  mAs   = Measure.mAs();    // Charging sum with efficiency
    int  mAh   = mAs / 3600;
    char sign1 = mA1s >= 0 ? '+' : '-';
    char sign  = mAs  >= 0 ? '+' : '-';

    mA1s = abs( mA1s );
    mAs  = abs( mAs);
    mAh  = abs( mAh );
    snprintf( s, sizeof(s), "charge = %c%i.%03i  %c%i.%03i  %c%i.%03i",
                sign1, mA1s / 1000, mA1s % 1000, sign, mAs / 1000, mAs % 1000, sign, mAh / 1000, mAh % 1000 );

    return s;
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
