#ifndef GLOBALS_H
#define GLOBALS_H

#include "ChRt.h"
#include "Arduino.h"
#include "ODriveArduino.h"

// define debug to enable printing debug messages
// I've made two flags so you have more options in terms of which messages to show
#define DEBUG_LOW // for non so important debug messages
#define DEBUG_HIGH // for more important debug messages

//------------------------------------------------------------------------------
// Helper utilities
// Add support for using "<<" to stream stuff to the usb serial
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

//------------------------------------------------------------------------------
// Initialize objects related to ODrives

// TODO: There's a lot of repetition in this section that's hinting we should
// somehow encapsulate more behavior. We could put the serial references inside
// the ODriveArduino class and put the pos estimate struct in there too

// Make references to Teensy <-> computer serial (aka USB) and the ODrive(s)
HardwareSerial& odrv0Serial = Serial1;
HardwareSerial& odrv1Serial = Serial2;
HardwareSerial& odrv2Serial = Serial3;
HardwareSerial& odrv3Serial = Serial4;

// Make structs to hold motor readings
// TODO: figure out if I want to mimic the ODive struct style or not
struct ODrive {
    struct Axis {
        float pos_estimate = 0; // in counts
        float ENCODER_OFFSET = 0; // in counts, TODO: need to configure this

        // NOTE: abs_pos is the SUM of estiamte and offset
        float abs_pos_estimate = pos_estimate + ENCODER_OFFSET;
    };
    Axis axis0,axis1;
} odrv0, odrv1, odrv2, odrv3;

// ODriveArduino objects
// These objects are responsible for sending commands to the ODrive
ODriveArduino odrv0Interface(odrv0Serial);
ODriveArduino odrv1Interface(odrv1Serial);
ODriveArduino odrv2Interface(odrv2Serial);
ODriveArduino odrv3Interface(odrv3Serial);

//------------------------------------------------------------------------------
// Global variables. These are needed for cross-thread communication!!

struct LegGain {
    float Kp_theta = 0;
    float Kd_theta = 0;

    float Kp_gamma = 0;
    float Kd_gamma = 0;
} leg0;

volatile uint32_t count = 0;
volatile uint32_t maxDelay = 0;

volatile long latest_send_timestamp = 0;
volatile long latest_feedback_delay = 0;

#endif
