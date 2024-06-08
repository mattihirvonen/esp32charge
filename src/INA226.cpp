
#include  "Arduino.h"
#include  <stdlib.h>    // strtol()
#include  <Wire.h>
#include  "INA226.h"

#define   RESET_BIT            0x8000
#define   CONFIG_AVG1          0x0     // Average of 1  samples
#define   CONFIG_AVG4          0x1     // Average of 4  samples
#define   CONFIG_AVG16         0x2     // Average of 16 samples
#define   CONFIG_VBUSCT        0x4     // Conversion time 1.1 ms
#define   CONFIG_VSHCT         0x4     // Conversion time 1.1 ms
#define   CONFIG_MODE          0x7     // Shunt and bus, continuous conversiion

// Default value is 0x4123
#define  CONFIG_VALUE  ((1 << 14) | (CONFIG_AVG4 << 9) | (CONFIG_VBUSCT << 6) | (CONFIG_VSHCT << 3) | CONFIG_MODE)

////////////////////////////////////////////////////////
//
//  Constructor
//
/*!
 *  @brief  Instantiates a new INA226 class
 *  @param addr the I2C address the device can be found on. Default is 0x40
 */
INA226::INA226(const uint8_t address, TwoWire *wire)
{
  _address     = address;
  _wire        = wire;
}


/*!
 *  @brief INA226 class destructor
 */
INA226::~INA226()
{
//delete i2c_dev;
}


bool INA226::begin()
{
  if (! isConnected()) return false;
  return true;
}


bool INA226::isConnected()
{
  _wire->beginTransmission(_address);
  return ( _wire->endTransmission() == 0);
}


uint8_t INA226::getAddress()
{
  return _address;
}

////////////////////////////////////////////////////////
//
//  Core functions
//
int16_t INA226::reg( int arg, uint8_t reg, uint16_t value )
{
    if ( arg == 0 ) {
        reset();
    }
    if ( arg == 1 ) {
        return readRegister( reg );
    }
    if ( arg == 2 ) {
        Serial.printf("write:register: [%i] <- 0x%04x \r\n", reg, value );
        return writeRegister( reg, value );
    }
    return 0;
}


int INA226::shunt_uV( void )
{
    // 2.5 uV/bit
    int value = (int16_t) readRegister( 1 );
    return (5 * value) / 2;
}


int INA226::bus_mV( void )
{
    // 1.25 mV/bit
    int value = readRegister( 2 );
    return  (5 * value) / 4;
}

////////////////////////////////////////////////////////
//
//  Configuration
//
bool INA226::reset()
{
    writeRegister( 0, (RESET_BIT | CONFIG_VALUE)  );
    delay( 10 );
    writeRegister( 0, (CONFIG_VALUE)  );
    return true;
}

////////////////////////////////////////////////////////
//
//  PRIVATE
//
uint16_t INA226::readRegister(uint8_t reg)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission();

    _wire->requestFrom(_address, (uint8_t)2);
    uint16_t value = _wire->read();
    value <<= 8;
    value |= _wire->read();
    return value;
}


uint16_t INA226::writeRegister(uint8_t reg, uint16_t value)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value >> 8);
    _wire->write(value & 0xFF);
    return _wire->endTransmission();
}
