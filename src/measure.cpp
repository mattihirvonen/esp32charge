
#if 0  //=======================================================================

Voltage meters are also used to indicate battery state of charge. Digital voltmeters provide
the accuracy to read the voltage in hundredths and are relatively inexpensive and easy to
use. The main problem with relying on voltage reading is the high degree of battery voltage
variation through out the day. Battery voltage reacts highly to charging and discharging. As a
battery is charged the indicated voltage increases and, as discharging occurs, the indicated
voltage decreases. With experience, one can accurately determine state of charge using a
voltmeter.

Percentage  12 Volt           Specific 
of Charge   Battery           Gravity
            Voltage
--------------------------------------
   100      12.70             1.265
   95       12.64             1.257
   90       12.58             1.249
   85       12.52             1.241
   80       12.46             1.233
   75       12.40             1.225
   70       12.36             1.218
   65       12.32             1.211
   60       12.28             1.204
   55       12.24             1.197
   50       12.20             1.190  (some tables claims 60% at 12.20V)
   45       12.16             1.183
   40       12.12             1.176
   35       12.08             1.169
   30       12.04             1.162
   25       12.00             1.155
   20       11.98             1.148
   15       11.96             1.141
   10       11.94             1.134
   5        11.92             1.127
Discharged  11.90             1.120

INA219 :
* Resolution 10 uV / bit (full scale range +-40 mV)
* Full scale ranges 40 mV ... 320 mV (depend on gain setting)
* +-4000 (+-12 bits)

INA226:
* Resolution 2.5 uV / bit
* Full scale range 80 mV
* +-32000 (+-15 bits)

Rmeasure external shunt:
* 75mV / 100A (750 uOhm)
* INA219 fs  53.3 A (40 mV range) -> 13.0 mA/bit
* INA226 fs 106.7 A (80 mV range) ->  3.3 mA/bit

Rmeasure INA module:
* 0.1 ohm (100000 uOhm)

#endif //=======================================================================

#include <stdint.h>
#include "Arduino.h"
#include <Wire.h>
#include <task.h>
#include "INA219.h"
#include "measure.h"

#define  CAPASITY_Ah  90

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
static volatile int _uV;                // "raw" shunt voltage measurement result in micro volts
static volatile int _mV;                // "raw" bus   voltage measurement result in milli volts
static volatile int _mA;                // "raw" bus   current measurement result in milli amperes
static volatile int _mAs = 0;           // cumulative charge in milli ampere seconds
static volatile int _mA1s;              // average current measurement over 1 second period
static volatile int _efficiency = 75;   // battery charging efficiency [%]
static volatile int _offset     = -10;  // input offset error [uV]

static volatile int _capacity_mAs = 1000 * 3600 * CAPASITY_Ah;

#if 1
// Real boat
static volatile int _Rshunt = 750;      // micro ohm - 75 mV / 100 A
static volatile int _scaleI = 100;      // current scaling normalize to 100 %
static volatile int _scaleU = 1000;     // voltage scaling normalize to 100.0 %
static volatile int _compU  = 0;        // bus voltage sense wire loss compensation  [mV/A]
#else
// Home test
static volatile int _Rshunt = 100000;   // micro ohm
static volatile int _scaleI = 612;      // current scaling normalize to 100%
static volatile int _compU  = 50;       // bus voltage sense wire loss compensation  [mV/A]
#endif

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

    _mAs = _capacity_mAs;
    
    while ( 1 )
    {
        int sum_uV   = 0;    // Shunt
        int sum_mV   = 0;    // Bus
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

            int uV =  INA.shunt_uV() - _offset;
            int mV = (INA.bus_mV()   * _scaleU) / 1000;

            int mA = (1000 * uV) / _Rshunt;
            int mA_charge;
 
            mA  = (mA * _scaleI ) / 100;
            mV -= (mA * _compU  ) / 1000;

            if ( mA >=0 ) {  mA_charge = ( mA * _efficiency ) / 100;  }    // Charging
            else          {  mA_charge =   mA;                        }    // Discharging

            if ( msDelay >= 0 )          // Some cases at start msDelay might be negative ?!!
            {
                samples  += 1;
                sum_uV   += uV;
                sum_mV   += mV;
                sum_mA1s += mA;          // Raw charging current (measure with slow DMM)
                sum_mAs  += mA_charge;   // Charging with efficiency                
                _mA       = mA;          // Update "raw" current  measuremeunt result
            }
        }

        // Update one time per second
        if ( samples )
        {
            _uV    = sum_uV   / samples;
            _mV    = sum_mV   / samples;
            _mA1s  = sum_mA1s / samples;
            if( (_mAs < _capacity_mAs) || (sum_mAs < 0) ) {
                 _mAs += sum_mAs / samples;
            }
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
    if (scale_100 ) {
        _scaleI = scale_100;
    }

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


// Latest "raw" shunt voltage mearurement result [uV]
int MEASURE::uV( void )
{   return _uV;  }


// Latest "raw" bus voltage mearurement result [mV]
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


// Get R shunt resistence[ u ohm]
int MEASURE::Rshunt( void )
{
    return _Rshunt;
}


// Get differential amplifier offset error [uV]
int MEASURE::offset( void )
{
    return _offset;
}


// Get charging operating efficiency [%]
int MEASURE::efficiency( void )
{
    return _efficiency;
}


// Get current measurement calibration scale [%]
int MEASURE::scaleI( void )
{
    return _scaleI;
}


// Get bus voltage measurement calibration scale [%]
int MEASURE::scaleU( void )
{
    return _scaleU;
}


// Get bus voltage measurement correction [mV/A]
int MEASURE::compU( void )
{
    return _compU;
}


// Set R shunt resistence[ u ohm]
int MEASURE::setRshunt( int micro_ohm )
{
    _Rshunt = micro_ohm;
    return _Rshunt;
}


// Set diff apmlifier offset error [uV]
int MEASURE::setOffset( int uV )
{
    _offset = uV;
    return _offset;
}


int MEASURE::setIscale( int scale_100 )
{
    _scaleI = scale_100;
    return _scaleI;
}


int MEASURE::setUscale( int scale_100 )
{
    _scaleU = scale_100;
    return _scaleU;
}


int MEASURE::setUcomp( int mVA )
{
    _compU = mVA;
    return _compU;
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


// Get schedule statistics
int MEASURE::getStat( int select )
{
    select = abs( select );
    if ( select > (sizeof(timingDelay) / sizeof(int)) ) return -1;  // ERROR

    return timingDelay[ select ]; 
}