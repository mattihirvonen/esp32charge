#ifndef _LIB_MEASURE_
#define _LIB_MEASURE_

#include <stdint.h>     // uint16_t
#include "Arduino.h"
#include "Wire.h"

class MEASURE
{
    public:
        explicit MEASURE();
        ~MEASURE();

        void  begin( int scale_1000 );

        int   mA();
        int   mAs();
        int   mA1s();

        int   setRshunt( int micro_ohm );
        int   setScale( int scale_1000 );

    private:

};

#endif // _LIB_MEASURE_
