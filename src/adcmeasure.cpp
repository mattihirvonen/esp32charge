
#include <arduino.h>          // Serial.print()
#include <stdint.h>
#include "driver/adc.h"       // to use adc1_get_raw instead of analogRead
#include "adcmeasure.hpp"
#include "INA.hpp"

adc_atten_t       atten = ADC_ATTEN_DB_11;      // ADC_ATTEN_DB_11 - DB_0=0  DB_2_5=1  DB_6=2  DB_11=3
adc_bits_width_t  width = ADC_WIDTH_BIT_12;     // 12, 11, 10 or 9

// Scale:
// - atten DB_11 value 1192 equals 1020 mv

int16_t  ADCmeasure::measure( adc1_channel_t adc1channel, int average )
{
    int sum = 0;

    if ( average < 1) {
        return ERROR_ADC;
    }

    adc1_config_width(width);
    adc1_config_channel_atten(adc1channel, atten);

    for (int i = 0; i < average; i++ ) {
        sum += adc1_get_raw( adc1channel );
    }
    return sum / average;
}


void ADCmeasure::print_error( int gpio )
{
    char s[64];

    snprintf( s, sizeof(s), "[adc] can't read GPIO %i", gpio );
    Serial.println( s );
//  sendTelnet ( s ); // send error
}


int16_t  ADCmeasure::gpio( int gpio, int average )
{
    adc1_channel_t  adc1channel;

    // GPIO to CHANNEL mapping depending on the board type: 
    // https://github.com/espressif/arduino-esp32/blob/master/boards.txt

    #if CONFIG_IDF_TARGET_ESP32
 
        switch (gpio) {
            // ADC1
            case 36: adc1channel = ADC1_CHANNEL_0; break;
            case 37: adc1channel = ADC1_CHANNEL_1; break;
            case 38: adc1channel = ADC1_CHANNEL_2; break;
            case 39: adc1channel = ADC1_CHANNEL_3; break;
            case 32: adc1channel = ADC1_CHANNEL_4; break;
            case 33: adc1channel = ADC1_CHANNEL_5; break;
            case 34: adc1channel = ADC1_CHANNEL_6; break;
            case 35: adc1channel = ADC1_CHANNEL_7; break;
            // other GPIOs do not have ADC
            default: print_error( gpio );
                     return ERROR_ADC;
        }

    #elif CONFIG_IDF_TARGET_ESP32S3

        // ESP32 S3 board: https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/adc.html
        switch (gpio) {
            // ADC1
            case  1: adc1channel = ADC1_CHANNEL_0;  break;
            case  2: adc1channel = ADC1_CHANNEL_1;  break;
            case  3: adc1channel = ADC1_CHANNEL_2;  break;
            case  4: adc1channel = ADC1_CHANNEL_3;  break;
            case  5: adc1channel = ADC1_CHANNEL_4;  break;
            case  6: adc1channel = ADC1_CHANNEL_5;  break;

            case  7: adc1channel = ADC1_CHANNEL_6;  break;
            case  8: adc1channel = ADC1_CHANNEL_7;  break;
            case  9: adc1channel = ADC1_CHANNEL_8;  break;
            case 10: adc1channel = ADC1_CHANNEL_9;  break;

            // ADC2 (GPIOs 11, 12, 13, 14, 15, 16, 17, 18, 19, 20), the reading blocks when used together with WiFi?
            // other GPIOs do not have ADC
            default: print_error( gpio );
                     return ERROR_ADC;
       }

    #else
        #error "Your board (CONFIG_IDF_TARGET) is not supported by ADCmeasure"
    #endif

    return measure( adc1channel, average );
}


int  ADCmeasure::ina( int arg )
{

}