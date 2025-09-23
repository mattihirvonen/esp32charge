//
// NOTE: !!!!
// ESP32S3 Do not support Classic Bluetooth Serial Port Protocol (SPP - 4.2)
// - https://randomnerdtutorials.com/esp32-bluetooth-classic-arduino-ide/
// ESP32S3 support only Bluetooth Low Energy (BLE)
// - https://registry.platformio.org/libraries/avinabmalla/ESP32_BleSerial/examples/BleSerial_Bridge/BleSerial_Bridge.ino
//
// #include <stdint.h>
// #include "Arduino.h"

#include <BleSerial.h>
/*
#include <esp_attr.h>
#include <esp_task_wdt.h>
#include <driver/rtc_io.h>
#include <esp_mac.h>
*/
const int BUFFER_SIZE = 8192;
const int STACK_SIZE  = 8192;


BleSerial SerialBT;

uint8_t   unitMACAddress[6];   // Use MAC address in BT broadcast and display
char      deviceName[20];      // The serial string that is broadcast.

uint8_t bleReadBuffer[BUFFER_SIZE];
uint8_t serialReadBuffer[BUFFER_SIZE];


void startBluetooth() {
  // Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  
  // Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address
  unitMACAddress[5] += 2;                                                          
  
  //Create device name
  sprintf(deviceName, "ESP32-Bridge-%02X%02X", unitMACAddress[4], unitMACAddress[5]); 

  //Init BLE Serial
  SerialBT.begin(deviceName);
  SerialBT.setTimeout(10);
}


// This function called from "standard" setup() function
void setup_BluetoothSerial( void )
{
  //Start BLE
  startBluetooth();
}


// This function called from "standard" loop() function
void loop_BluetoothSerial( void )
{
    static TickType_t xLastWakeTime;
    
    if ( (int)(xTaskGetTickCount() - xLastWakeTime) < 20 ) {
        return;
    }
    xLastWakeTime = xTaskGetTickCount();

    #if 1
    // SerialBT echo loop 
    if (SerialBT.available()) {
        auto count = SerialBT.readBytes(bleReadBuffer, BUFFER_SIZE);
        SerialBT.write(bleReadBuffer, count);
    }

    #else

    // Gateway SerialBT <-> Serial (port)
    if (Serial.available()) {
      auto count = Serial.readBytes(serialReadBuffer, BUFFER_SIZE);
      SerialBT.write(serialReadBuffer, count);
    }
    if (SerialBT.available()) {
      auto count = SerialBT.readBytes(bleReadBuffer, BUFFER_SIZE);
      Serial.write(bleReadBuffer, count);
    }
    #endif
}

/*
void setup() {
  //Start Serial
  Serial.begin(115200);
  Serial.setRxBufferSize(BUFFER_SIZE);
  Serial.setTimeout(10);

  //Start BLE
  startBluetooth();

  //Disable watchdog timers
  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT();
  esp_task_wdt_delete(NULL);

  //Start tasks
  xTaskCreate(ReadSerialTask, "ReadSerialTask", STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(ReadBtTask, "ReadBtTask", STACK_SIZE, NULL, 1, NULL);
}

void loop() {
  //This task is not used
  vTaskDelete(NULL);
}
*/
 
//=====================================================================================================================
#if 0

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

#endif // 0
