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

UTIL  Utils;

int  isFirmwareUploaded = 0;  // set false for security

void fw_update ( void );

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
const char *  UTIL::fwupdate( int args, const char *filename )
{
    static char reply[64];

    snprintf(reply, sizeof(reply), "fwupdate function called with arg: <%s>", filename);

    isFirmwareUploaded = 1;   // Fake to enable update process
    fw_update();

    Serial.printf("%s", reply);
    Serial.println("");

    return (const char*) reply;
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


void fw _update ( void )
{
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
            }

            firmware.close();

            String renamed = name;
            renamed.replace(".bin", ".bak");
            if (LittleFS.rename(name, renamed.c_str())) {
                Serial.println(F("Firmware rename succesfully!"));
            }
            else {
                Serial.println(F("Firmware rename error!"));
            }
            delay(2000);

//          ESP.reset();
//          ESP.restart();
            reset( 0 );
        }
    }
}
