
//  https://randomnerdtutorials.com/esp32-bluetooth-classic-arduino-ide/

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

/* static */ BluetoothSerial SerialBT;

void setup_BluetoothSerial( void )
{
  SerialBT.begin("ESP32serial"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}


void loop_BluetoothSerial( void )
{
    #if 1
    // SerialBT echo loop 
    if (SerialBT.available()) {
        SerialBT.write( SerialBT.read() );
    }
    #else
    // Gateway SerialBT <-> Serial (port)
    if (Serial.available()) {
        SerialBT.write( Serial.read() );
    }
    if (SerialBT.available()) {
        Serial.write( SerialBT.read() );
    }
    #endif
}
