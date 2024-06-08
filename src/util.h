//
// File:  util.h
//

#ifndef _LIB_UTIL_
#define _LIB_UTIL_

#include  <stdint.h>
#include  <stdlib.h>    // bool, strtol()
#include  "Arduino.h"

class UTIL
{
    public:
        explicit UTIL();
        ~UTIL();

        bool           begin();
        const char    *fwupdate( int args, const char *filename );
        const char    *charge( int args, const char *arg1 );
        String         httpCharge( int args, const char *arg1, const int arg2 );
    
    private:

};

#endif // _LIB_UTIL_
