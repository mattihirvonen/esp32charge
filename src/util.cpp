//
// File:  util.cpp
//

#include <WiFi.h>
#include "util.h"

UTIL  Utils;

#if 0
        // writing output to telnet with error logging (dmesg)
         int sendTelnet (const char *buf, size_t len) { 
            int i = sendAll (__connectionSocket__, buf, len, __telnet_connection_time_out__);
//          if (i <= 0)
//              dmesg ("[telnetConnection] send error: ", errno, strerror (errno));
            return i;
        }

        int sendTelnet (const char *buf) { return sendTelnet (buf, strlen (buf)); }
#endif


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


const char *  UTIL::fwupdate( int args, const char *filename )
{
    static char reply[64];

    if ( !args ) {
        snprintf(reply, sizeof(reply), "fwupdate function called with: no arguments");
    }
    else {
        snprintf(reply, sizeof(reply), "fwupdate function called with: <%s>", filename);
    }
    Serial.printf("%s", reply);
    Serial.println("");

    return (const char*) reply;
}
