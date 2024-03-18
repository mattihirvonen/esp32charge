
#include <stdint.h>
#include "Arduino.h"
#include <Wire.h>
#include <task.h>
#include "INA.h"
#include "measure.h"

void dmesg (char *message1);

// Strategy:
// - Raise measure task's priority over other task's priority to
//   offer better measurement sample timing
// - With dual core processor dedicate measurement task to core where
//   is less other tasks to to offer better measurement sample timing
// - 32 bit integer arithmetics is enough to calculate charge.
//   Use unit [mA*s] where 0x7fffffff equals 596.5 [Ah]
// - Configure INA219 to make continuous hardware avraging of 16 A/D
//   samples which takes 8.5 ms time

#if       CONFIG_IDF_TARGET_ESP32S3
#define   TASK_MEASURE_CORE   1       // loop() run on core 1, other task's default is 0
#endif
#define   tskNORMAL_PRIORITY  2       // Task's and loop() default priority is 1
#define   MEASURE_STACK_SIZE  2048    // Bytes not words!  configMINIMAL_STACK_SIZE is 828

// Public object(s)
INA219  INA( INA219_ADDRESS );

// Local variables, which accessed with multiple CPU cores
// (how to make sure latest valid data is in inter CPU common memory and
// do not reside only in one one CPU core's cache memory ???)
// Read also https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

// NOTE(s):
// - All MEASURE instances should use this same data!
// - There is started only one measure task
// 
static volatile int started = 0;        // allow to start only one measurement task
static volatile int timingDelay[22];    // task's measured timing statistics 
                                        //
static volatile int _mV;                // "raw" voltage measurement result in milli volts
static volatile int _mA;                // "raw" current measurement result in milli amperes
static volatile int _mAs = 0;           // Cumulative charge in milli ampere seconds
static volatile int _mA1s;              // Average current measurement over 1 second period
static volatile int _Rshunt = 100000;   // micro ohm
static volatile int _scale  = 100;      // normalize to 100
static volatile int _efficiency = 80;   // battery charging efficiency [%]

// Trick: Set "scale" multiplier to 612 (== 6.12x)
// Notify wiring resistances in my measurement test circuit
// There is
// - 0.100 ohm external current shunt resistor parallel with
// - (0.32 + 0.1) ohm measurement circuit where
// - 0.32 ohm is wiring resistance (4m wire 0.22 mm2) series with
// - 0.1  ohm current shunt resistor in Adafruit INA219 module
// --> scale = 612
//
// https://www.freertos.org/a00125.html
//
void vTaskMeasure( void * pvParameters )
{
    #define DBGPIN  2  // GPIO

    static char s[64];  // Do not use stack space

    // Note: Select SAMPLES_PER_SECOND between range 10 ... 100
    // ( 10, 20, 40, 50, 100 )

    #define SAMPLES_PER_SECOND   20
    #define SAMPLE_PERIOD       (1000 / SAMPLES_PER_SECOND)    // [ms]

    int ledstate = 0;
    int dbgpin   = 0;

    TickType_t        xLastWakeTime;
    const TickType_t  xFrequency = SAMPLE_PERIOD;   // in tick(s) [ms]
    BaseType_t        xWasDelayed;

    Serial.print(  "Measure started on: Core ");
    Serial.println( xPortGetCoreID() );
    Serial.print(  "Measure task stack size: " );
    Serial.println( MEASURE_STACK_SIZE );
    Serial.print(  "Measure task priority:   ");
    Serial.println( uxTaskPriorityGet(NULL) );
    Serial.println();

    snprintf(s, sizeof(s), "[%s] is running on core %i (priority %i)",
                __func__, xPortGetCoreID(), uxTaskPriorityGet(NULL) );
    dmesg(s);
    
    // initialize digital pin LED_BUILTIN as an output.
    // pinMode(LED_BUILTIN, OUTPUT);
       pinMode(DBGPIN, OUTPUT);

    INA.reset();   // Set INA chip mode continuous 16 samples averaging

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount ();
    memset( (void*)timingDelay, sizeof(timingDelay), 0 );

    while ( 1 )
    {
        int sum_mA1s = 0;
        int sum_mAs  = 0;
        int samples  = 0;

        for ( int i = 0; i < SAMPLES_PER_SECOND; i++ )
        {
            TickType_t  scheduleTime = xLastWakeTime + xFrequency;

            // Wait for the next cycle.
            xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );
            digitalWrite(DBGPIN, (dbgpin++ & 1));

            int msDelay = xTaskGetTickCount() - scheduleTime;

            if ( msDelay < 0 ) {
                timingDelay[ 21 ] += 1;
//              continue;
            }
            if ( msDelay >= 100 ) {
                timingDelay[ 20 ] += 1;
            }
            else if ( msDelay >= 10 ) {
                timingDelay[ 10 + (msDelay / 10) ] += 1;
            }
            else {
                timingDelay[ msDelay ] += 1;
                timingDelay[ 10   ]    += 1;
            }

            // Perform action here. xWasDelayed value can be used to determine
            // whether a deadline was missed if the code here took too long.
            // NOTE:
            // - neg.input: battery terminal (bus voltage to ground measurement terminal)
            // - pos.input: load    terminal

            int mA_charge,  mA = INA.shunt_uV() / ( _Rshunt / 1000 );

            mA = (mA * _scale ) / 100;

            if ( mA >=0 ) {  mA_charge = ( mA * _efficiency ) / 100;  }    // Charging
            else          {  mA_charge =   mA;                        }    // Discharging

            if ( msDelay >= 0 )          // Some cases at start msDelay might be negative ?!!
            {
                samples  += 1;
                sum_mA1s += mA;          // Raw charging current (measure with slow DMM)
                sum_mAs  += mA_charge;   // Charging with efficiency                
                _mA       = mA;          // Update "raw" current  measuremeunt result
            }
        }

        // Update one time per second
        if ( samples )
        {
            _mV    = INA.bus_mV();       // 14150 / 14220;
            _mA1s  = sum_mA1s / samples;
            _mAs  += sum_mAs  / samples;
        }
        if ( ledstate ) {  digitalWrite(LED_BUILTIN, LOW);  ledstate = 0; }  // turn the LED off by making the voltage LOW
        else            {  digitalWrite(LED_BUILTIN, HIGH); ledstate = 1; }  // turn the LED on (HIGH is the voltage level)
    }
}

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


// Arduino style module initialization
void MEASURE::begin( int scale_100 )
{
    _scale = scale_100;

    if ( started ) {    // Run only one measurement task 
         return;
    }
    started = 1;

    Wire.begin();
    if ( INA.begin() )  { Serial.println("I2C connect to INA ok");     }
    else                { Serial.println("could not I2C connect to INA. Fix and Reboot"); }
    Serial.println();

    #ifdef TASK_MEASURE_CORE
    BaseType_t measure_taskCreated = xTaskCreatePinnedToCore (vTaskMeasure, "Measure", MEASURE_STACK_SIZE, NULL, tskNORMAL_PRIORITY, NULL, TASK_MEASURE_CORE);
    #else
    BaseType_t measure_taskCreated = xTaskCreate (vTaskMeasure, "Measure", MEASURE_STACK_SIZE, NULL, tskNORMAL_PRIORITY, NULL);
    #endif
}


// Latest "raw" voltage mearurement result [mV]
int MEASURE::mV( void )
{   return _mV;  }


// Latest "raw" current measurement result [mA]
int MEASURE::mA( void )
{   return _mA;  }


// Cumulative charge sum [mA*s]
int MEASURE::mAs( void )
{   return _mAs;  }


// Current measurement average over 1 second period [mA]
// Average of SAMPLES_PER_SECOND
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


// Set battery charge state
// Use for reset/init cumulative sum calculation
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


// Get schedule statistics
int MEASURE::getStat( int select )
{
    select = abs( select );
    if ( select > (sizeof(timingDelay) / sizeof(int)) ) return -1;  // ERROR

    return timingDelay[ select ]; 
}