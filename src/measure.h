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

        void  begin( int scale_100 );

        int   uV();    // Shunt
        int   mV();    // Bus
        int   mA();
        int   mAs();
        int   mA1s();

        int   setUcomp(  int mVA );
        int   setRshunt( int micro_ohm );
        int   setIscale( int scale_100 );
        int   setAh( int Ah );
        int   setEfficiency( int percent );
        int   getEfficiency( void );

        int   getStat( int select );

    private:
/*
        volatile int _mA;                // "raw" current measurement result in milli amperes
        volatile int _mAs = 0;           // Cumulative charge in milli ampere seconds
        volatile int _mA1s;              // Average current measurement over 1 second period
        volatile int _Rshunt = 100000;   // micro ohm
        volatile int _scale  = 100;      // normalize to 100
        volatile int _efficiency = 80;   // battery charging efficiency [%]
*/
};

#endif // _LIB_MEASURE_
