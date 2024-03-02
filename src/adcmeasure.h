
#ifndef _LIB_ADC_MEASURE_
#define _LIB_ADC_MEASURE_

#include <stdint.h>   // uint16_t

#define  ERROR_ADC  0xffff

class ADCPU {

    public:

        void       init();
        int16_t    measure( adc1_channel_t adc1channel, int average );
        int16_t    gpio( int gpio, int average );


    private:

        int        i2c_regptr = 0;

        void       print_error( int gpio );
};

#endif // _LIB_ADC_MEASURE_
