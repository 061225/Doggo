#include "ChRt.h"
#include "Arduino.h"
#include "ODriveArduino.h"
#include "config.hpp"
#include "globals.hpp"

#ifndef POSITION_CONTROL_H
#define POSITION_CONTROL_H

void CoupledPIDControl();
void ODrivePosControl();

//------------------------------------------------------------------------------
// PositionControlThread: Motor position control thread
// Periodically calculates result from PID controller and sends off new motor
// current commands to the ODrive(s)

// TODO: add support for multiple ODrives

static THD_WORKING_AREA(waPositionControlThread, 128);

static THD_FUNCTION(PositionControlThread, arg) {
    (void)arg;

    while(true) {
        // CoupledPIDControl();
        ODrivePosControl();
    }
}

/**
 * Drives the ODrives in an open-loop, position-control trajectory.
 */
void ODrivePosControl() {

    float t = millis()/800.0; //.5hz
    float sp00 = 1200*sin(2*PI*t+PI/2.0);
    float sp01 = 1200*sin(2*PI*t);
    float sp10 = 1200*sin(2*PI*t+3*PI/2.0);
    float sp11 = 1200*sin(2*PI*t+PI);
    odrv0Interface.SetPosition(0,sp00);
    odrv0Interface.SetPosition(1,sp01);
    odrv1Interface.SetPosition(0,sp10);
    odrv1Interface.SetPosition(1,sp11);

    // float sp20 = 1200*sin(2*PI*t);
    // float sp21 = 1200*sin(2*PI*t+PI/2.0);
    // float sp30 = 1200*sin(2*PI*t+PI);
    // float sp31 = 1200*sin(2*PI*t+3*PI/2.0);
    float sp30 = 1200*sin(2*PI*t);
    float sp31 = 1200*sin(2*PI*t+PI/2.0);
    float sp20 = 1200*sin(2*PI*t+PI);
    float sp21 = 1200*sin(2*PI*t+3*PI/2.0);

    odrv2Interface.SetPosition(0,sp20);
    odrv2Interface.SetPosition(1,sp21);
    odrv3Interface.SetPosition(0,sp30);
    odrv3Interface.SetPosition(1,sp31);

    chThdSleepMilliseconds(5);
}

/**
 * Computes PID onboard the teensy to enable coupled control on the legs.
 * Right now it only computes control for a single leg.
 */
void CoupledPIDControl() {
    float alpha = (float) odrv0.axis0.abs_pos_estimate;
    float beta = (float) odrv0.axis1.abs_pos_estimate;

    float theta = alpha/2.0 + beta/2.0;
    float gamma = beta/2.0 - alpha/2.0;

    float theta_sp = 0; // TODO take as struct or something
    float gamma_sp = 0; // TODO take as struct or something

    float p_term_theta = leg_default.Kp_theta * (theta_sp - theta);
    float d_term_theta = leg_default.Kd_theta * (0); // TODO: Add motor velocities to position message from odrive

    float p_term_gamma = leg_default.Kp_gamma * (gamma_sp - gamma);
    float d_term_gamma = leg_default.Kd_gamma * (0); // TODO: Add motor velocities to position message from odrive

    // TODO: clamp (ie constrain) the outputs to -1.0 to 1.0
    float tau_theta = p_term_theta + d_term_theta;
    float tau_gamma = p_term_gamma + d_term_gamma;

    // TODO: check signs
    float tau_alpha = tau_theta*0.5 - tau_gamma*0.5;
    float tau_beta = tau_theta*0.5 + tau_gamma*0.5;
    // odrv0Interface.SetDualCurrent(tau_alpha, tau_gamma);

    latest_send_timestamp = micros();

    // DEBUG only: send two zero current commands
    odrv0Interface.SetDualCurrent(0, 0);

    // The duration of sleep controls the loop frequency
    chThdSleepMicroseconds(1000000/POSITION_CONTROL_FREQ);
}

#endif
