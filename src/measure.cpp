
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
static int _scale  = 6122;     // normalize to 1000

// Trick: "scale" notify wiring resistances im measurement circuit
// There is
// - 0.100 ohm current shunt resistor parallel with
// - (0.32 + 0.1) ohm measurement circuit where
// - 0.32 ohm is wiring resistance (4m wire 0.22 mm2) and series with
// - 0.1  ohm current shunt resistor in Adafruit INA219 module

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
        int sum = 0;

        for ( int i = 0; i < 10; i++ )
        {
            // Wait for the next cycle.
            xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );

            // Perform action here. xWasDelayed value can be used to determine
            // whether a deadline was missed if the code here took too long.

            int mA = INA.shunt_uV() / ( _Rshunt / 1000 );

            _mA  = (mA * _scale ) / 1000;
            sum += _mA;
        }
        _mA1s  =  sum / 10;
        _mAs  += _mA1s;

        if ( ledstate ) {  digitalWrite(LED_BUILTIN, LOW);  ledstate = 0; }  // turn the LED off by making the voltage LOW
        else            {  digitalWrite(LED_BUILTIN, HIGH); ledstate = 1; }  // turn the LED on (HIGH is the voltage level)
    }
}


void MEASURE::init( void )
{
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


int MEASURE::setScale( int promille )
{
    _scale = promille;
    return _scale;
}
