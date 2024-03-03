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

        bool         begin();
        const char  *fwupdate( int args, const char *filename );
    
    private:

};

#endif // _LIB_UTIL_
