
//
// NOTE: 0x7fffffff [mAs] equals 596.5 [Ah]
//
#include <stdint.h>
#include "Arduino.h"
#include <Wire.h>
#include <task.h>
#include "INA.h"
#include "measure.h"

  #define   MEASURE_STACK_SIZE  2048    // Bytes not words!  configMINIMAL_STACK_SIZE
  #define   tskNORMAL_PRIORITY  2       // Task's default priority is 1
//#define   TASK_MEASURE_CORE   1       // loop() run on core 1, other tasks default is 0

// Public object(s)
INA219  INA( INA219_ADDRESS );

// Local variables 
static int _mA;
static int _mAs = 0;           // Charge in milli ampere seconds
static int _mA1s;              // Average current measurement over 1 second period
static int _Rshunt = 100000;   // micro ohm
static int _scale  = 100;      // normalize to 100
static int _efficiency = 80;   // charging efficiency [%]

// Trick: Set "scale" to 612
// Notify wiring resistances in measurement circuit
// There is
// - 0.100 ohm current shunt resistor parallel with
// - (0.32 + 0.1) ohm measurement circuit where
// - 0.32 ohm is wiring resistance (4m wire 0.22 mm2) series with
// - 0.1  ohm current shunt resistor in Adafruit INA219 module
// --> scale = 612
//

////////////////////////////////////////////////////////
//
//  Constructor
//
/*!
 *  @brief  Instantiates a new MEASURE class
 *  @param  none
 */
MEASURE::MEASURE()
{
}


/*!
 *  @brief MEASURE class destructor
 */
MEASURE::~MEASURE()
{
}


// https://www.freertos.org/a00125.html
void vTaskMeasure( void * pvParameters )
{
    #define SAMPLE_PERIOD  100  // [ms]

    static int ledstate = 0;

    TickType_t        xLastWakeTime;
    const TickType_t  xFrequency = SAMPLE_PERIOD;
    BaseType_t        xWasDelayed;

    Serial.print(  "LED blinker started on: Core ");
    Serial.println( xPortGetCoreID() );
    Serial.print(  "LED blinker task stack size: " );
    Serial.println( MEASURE_STACK_SIZE );
    Serial.print(  "LED blinker task priority:   ");
    Serial.println( uxTaskPriorityGet(NULL) );
    Serial.println();
    
    // initialize digital pin LED_BUILTIN as an output.
    // pinMode(LED_BUILTIN, OUTPUT);

    INA.reset();   // Set INA mode 16 samples

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount ();

    while ( 1 )
    {
        int sum_mA1s = 0;
        int sum_mAs  = 0;

        for ( int i = 0; i < 10; i++ )
        {
            // Wait for the next cycle.
            xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );

            // Perform action here. xWasDelayed value can be used to determine
            // whether a deadline was missed if the code here took too long.

            int mA_charge,  mA = INA.shunt_uV() / ( _Rshunt / 1000 );

            _mA = (mA * _scale ) / 100;

            if ( _mA >=0 ) {  mA_charge = ( _mA * _efficiency ) / 100;  }    // Charging
            else           {  mA_charge =   _mA;                        }    // Discharging

            sum_mA1s += _mA;         // Raw charging current (measure with DMM)
            sum_mAs  += mA_charge;   // Charging with efficiency
        }
        _mA1s  = sum_mA1s / 10;
        _mAs  += sum_mAs  / 10;

        if ( ledstate ) {  digitalWrite(LED_BUILTIN, LOW);  ledstate = 0; }  // turn the LED off by making the voltage LOW
        else            {  digitalWrite(LED_BUILTIN, HIGH); ledstate = 1; }  // turn the LED on (HIGH is the voltage level)
    }
}


void MEASURE::begin( int scale_100 )
{
    _scale = scale_100;

    Wire.begin();
    if ( INA.begin() )  { Serial.println("I2C connect to INA ok");     }
    else                { Serial.println("could not I2C connect to INA. Fix and Reboot"); }
    Serial.println();

    #ifdef TASK_MEASURE_CORE
        BaseType_t led_taskCreated = xTaskCreatePinnedToCore (vTaskMeasure, "Measure", MEASURE_STACK_SIZE, NULL, tskNORMAL_PRIORITY, NULL, TASK_MEASURE_CORE);
    #else
        BaseType_t led_taskCreated = xTaskCreate (vTaskMeasure, "Measure", MEASURE_STACK_SIZE, NULL, tskNORMAL_PRIORITY, NULL);
    #endif
}


int MEASURE::mA( void )
{   return _mA;  }


int MEASURE::mAs( void )
{   return _mAs;  }


int MEASURE::mA1s( void )
{   return _mA1s;  }


int MEASURE::setRshunt( int micro_ohm )
{
    _Rshunt = micro_ohm;
    return _Rshunt;
}


int MEASURE::setScale( int scale_100 )
{
    _scale = scale_100;
    return _scale;
}


//Set battery charge state
int MEASURE::setAh( int Ah )
{
    _mAs = 1000 * (3600 * Ah);
    return Ah;
}


// Set charging operating efficiency [%]
int MEASURE::setEfficiency( int percent )
{
    _efficiency = percent;
    return _efficiency;
}


// Get charging operating efficiency [%]
int MEASURE::getEfficiency( void )
{
    return _efficiency;
}
