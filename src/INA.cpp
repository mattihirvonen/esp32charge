
#include  "Arduino.h"
#include  <stdlib.h>    // strtol()
#include  <Wire.h>
#include  "INA.h"

#define  RESET_BIT  (1<<15)

#define  CONFIG_BRNG          0x1   // 32V
#define  CONFIG_PG_GAIN       0x3   // 320 mV - gain /8
#define  CONFIG_ADC_SAMPLES   0xc   // 16 samples 8.51 ms
#define  CONFIG_MODE          0x7   // Shunt and bus, continunuous

// Reset default value is 0x399f
#define  CONFIG_VALUE  ((CONFIG_BRNG<<13) | (CONFIG_PG_GAIN<<11) | (CONFIG_ADC_SAMPLES<<7) | (CONFIG_ADC_SAMPLES<<3) | CONFIG_MODE)

////////////////////////////////////////////////////////
//
//  Constructor
//
/*!
 *  @brief  Instantiates a new INA219 class
 *  @param addr the I2C address the device can be found on. Default is 0x40
 */
INA219::INA219(const uint8_t address, TwoWire *wire)
{
  _address     = address;
  _wire        = wire;
}


/*!
 *  @brief INA219 class destructor
 */
INA219::~INA219()
{
//delete i2c_dev;
}


bool INA219::begin()
{
  if (! isConnected()) return false;
  return true;
}


bool INA219::isConnected()
{
  _wire->beginTransmission(_address);
  return ( _wire->endTransmission() == 0);
}


uint8_t INA219::getAddress()
{
  return _address;
}

////////////////////////////////////////////////////////
//
//  Core functions
//
int16_t INA219::reg( int arg, uint8_t reg, uint16_t value )
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


int INA219::shunt_uV( void )
{
    int uV = (int16_t) readRegister( 1 );
    return 10 * uV;
}


int INA219::bus_mV( void )
{
    int value = readRegister( 2 ) >> 3;
    return  4 * value;     // 4 mV/bit
}

////////////////////////////////////////////////////////
//
//  Configuration
//
bool INA219::reset()
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
uint16_t INA219::readRegister(uint8_t reg)
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


uint16_t INA219::writeRegister(uint8_t reg, uint16_t value)
{
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value >> 8);
    _wire->write(value & 0xFF);
    return _wire->endTransmission();
}
