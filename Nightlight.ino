#include <avr/sleep.h>
#include <avr/wdt.h>

#ifdef DEBUG_SERIAL
#define SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
#define SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define SERIAL_PRINT(...)
#define SERIAL_PRINTLN(...)
#endif

#define LED_OUTPUT       10
#define PIR_SENSOR_PIN   11
#define LIGHT_SENSOR_PIN A1

void setup()
{
    pinMode(LED_OUTPUT, OUTPUT);
    pinMode(PIR_SENSOR_PIN, INPUT);
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#ifdef DEBUG_SERIAL
    Serial.begin(57600);
#endif
}

// All the power-saving wizardy comes from Nick Gammon:
// http://www.gammon.com.au/forum/?id=11497
//
// The end result (since I also desoldered the Pro-Mini's
// power LED) is less than 1mA.

// Nick Gammon's watchdog interrupt
ISR (WDT_vect)
{
    wdt_disable();  // disable watchdog
} // end of WDT_vect

void loop()
{
    static int time_on_remaining = 0;

    // Due to the power deep-sleeping for 0.125s, counting
    // is the only way to keep track of the time;
    // millis() doesn't work properly because of the sleep!
    if (time_on_remaining > 0)
        time_on_remaining -= 125;
    else
        digitalWrite(LED_OUTPUT, LOW);   // Turn the LED off

    // My PIR sensor's behavior leaves a lot to be desired :-)
    // The best configuration I found, was turning both pots
    // fully counter-clockwise, which sets sensitivity to minimum
    // (3 meters) and time-staying-high also to minimum (3 sec).
    // I then use this to keep my own tally and turn the white 
    // LED at pin LED_OUTPUT to HIGH for 30 seconds.
    auto light_sensor = analogRead(LIGHT_SENSOR_PIN);
    auto pir_sensor = digitalRead(PIR_SENSOR_PIN);
    SERIAL_PRINT("Light sensor:");
    SERIAL_PRINT(light_sensor);
    SERIAL_PRINT(" PIR sensor:");
    SERIAL_PRINT(pir_sensor);
    SERIAL_PRINT(" Remaining:");
    SERIAL_PRINTLN(time_on_remaining);
    // For illumination in my area, this threshold works quite nicely.
    if (light_sensor > 700) {
        // ...so if it's dark enough, and the PIR detected someone...
        if (pir_sensor == HIGH) {
            // ...light up the LED; and keep it up for 30 seconds.
            digitalWrite(LED_OUTPUT, HIGH);
            time_on_remaining = 30000;
        }
    }
    delay(10); // This seems out of place; but the power shenanigans
               // mess up serial output unless I use this (presumably
               // because it allows the serial output to drain before
               // we go to sleep).

    /////////////////////////////////////////////
    // Nick Gammon's original code henceforth.
    // He started by disabling ADC, but I need ADC to sample the light sensor,
    // so I commented this part out.
    // disable ADC
    // ADCSRA = 0;

    // clear various "reset" flags
    MCUSR = 0;
    // allow changes, disable reset
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval
    // set WDIE, and 0.125 second delay
    WDTCSR = bit (WDIE) | bit (WDP0) | bit (WDP1);
    wdt_reset();  // pat the dog

    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    noInterrupts ();           // timed sequence follows
    sleep_enable();

    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS);
    interrupts ();             // guarantees next instruction executed
    sleep_cpu ();

    // cancel sleep as a precaution
    sleep_disable();
}
