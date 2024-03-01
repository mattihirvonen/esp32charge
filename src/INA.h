#ifndef _LIB_INA_MEASURE_
#define _LIB_INA_MEASURE_

#include <stdint.h>   // uint16_t
#include "Arduino.h"
#include "Wire.h"

#define  ERROR_INA  0x8000

/** default I2C address **/
#define INA219_ADDRESS (0x40)  // 1000000 (A0+A1=GND)

class INA219
{
    public:
        // address 0x40
        explicit INA219(const uint8_t address, TwoWire *wire = &Wire);
        ~INA219();

        bool       begin();
        bool       isConnected();
        uint8_t    getAddress();
        bool       reset();

        int        shunt_uV();
        int        bus_mV();

        int16_t    reg( int arg, uint8_t reg, uint16_t value );
        uint16_t   readRegister( uint8_t reg);
        uint16_t   writeRegister(uint8_t reg, uint16_t value);

      //void       init();
    
    private:

        uint8_t    _address;
        TwoWire *  _wire;

        void       print_error( int gpio );
};

#endif // _LIB_INA_MEASURE_
