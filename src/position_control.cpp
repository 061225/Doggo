#include "position_control.h"
#include "Arduino.h"
#include "ODriveArduino.h"
#include "config.h"
#include "globals.h"
#include "jump.h"

//------------------------------------------------------------------------------
// PositionControlThread: Motor position control thread
// Periodically calculates result from PID controller and sends off new motor
// current commands to the ODrive(s)

// TODO: add support for multiple ODrives

THD_WORKING_AREA(waPositionControlThread, 512);

THD_FUNCTION(PositionControlThread, arg) {
    (void)arg;

    while(true) {
        // ODrivePosControl();
        // sinTrajectoryPosControl();
        if(ShouldExecuteJump()) {
            ExecuteJump();
        }
        chThdSleepMicroseconds(1000000/POSITION_CONTROL_FREQ);
    }
}

/**
* All the conversions between the different coordinates are given below, we can move them to whatever file we want.
* This needs to define global variable for the Hip angles, leg angles, and foot coordinates so the functions are void.
* We also have to define the upper and lower leg lengths L1 and L2 as global vars
* Here the Hip angles are assumed to be in radians with a 0 position in the
* downward y-direction and positive angles CCW.
* TODO: Make a HipAngles struct with alpha and beta values
* The x,y cartesian coordinates have the x-direction pointed where the robot is
* heading and y-direction is positive towards the ground
* TODO: Make struct with two floats for LegCartesian and give an x,y values
* The leg length (L) is the virtual leg length and is always positive,
* the leg angle, theta, is zero in the downward y-direction and positive when
* going CWW.
* TODO: Make a struct with L,theta values called LegParams
*/


/**
* Converts the two Hip angles (in radians) for a leg to the cartesian coordinates
*/
void HipAngleToCartesian(float alpha, float beta, float& x, float& y) {
    float L1 = 0.09; // upper leg length (m) what is it actually?
    float L2 = 0.162; // lower leg length (m)
    float delta = (beta+alpha+PI)/2.0 - acos(L1/L2*sin((beta-alpha)/2.0));
    x = L1*cos(alpha) + L2*cos(delta);
    y = L1*sin(alpha) + L2*sin(delta);
}

/**
* Takes the leg parameters and returns the gamma angle (rad) of the legs
*/
void GetGamma(float L, float theta, float& gamma) {
    float L1 = 0.09; // upper leg length (m)
    float L2 = 0.162; // lower leg length (m)
    float cos_param = (pow(L1,2.0) + pow(L,2.0) - pow(L2,2.0)) / (2.0*L1*L);
    if (cos_param < -1.0) {
        gamma = PI;
        #ifdef DEBUG_HIGH
        Serial.println("ERROR: L is too small to find valid alpha and beta!");
        #endif
      } else if (cos_param > 1.0) {
        gamma = 0;
        #ifdef DEBUG_HIGH
        Serial.println("ERROR: L is too large to find valid alpha and beta!");
        #endif
      } else {
        gamma = acos(cos_param);
      }
}

/**
* Converts the two Hip angles (in radians) for a leg into the Hip angles alpha, beta (in rads)
*/
void LegParamsToHipAngles(float L, float theta, float& alpha, float& beta) {
    float gamma;
    GetGamma(L, theta, gamma);
    alpha = theta + gamma;
    beta = theta - gamma;
}

/**
* Converts the leg params L, gamma to cartesian coordinates x, y (in m)
* Set x_direction to 1.0 or -1.0 to change which direction the leg walks
*/
void LegParamsToCartesian(float L, float theta, float leg_direction, float& x, float& y) {
    x = leg_direction * L * cos(theta);
    y = L * sin(theta);
}

/**
* Converts the cartesian coords x, y (m) to leg params L (m), theta (rad)
*/
void CartesianToLegParams(float x, float y, float leg_direction, float& L, float& theta) {
    L = pow((pow(x,2.0) + pow(y,2.0)), 0.5);
    theta = atan2(leg_direction * x, y);
}

/**
* Sinusoidal trajectory generator function with flexibility from parameters described below. Can do 4-beat, 2-beat, trotting, etc with this.
*/
void SinTrajectory (float t, struct GaitParams params, float gaitOffset, float& x, float& y) {
    float stanceHeight = params.stanceHeight;
    float downAMP = params.downAMP;
    float upAMP = params.upAMP;
    float flightPercent = params.flightPercent;
    float stepLength = params.stepLength;
    float FREQ = params.FREQ;

    float gp = fmod((FREQ*t+gaitOffset),1.0); // mod(a,m) returns remainder division of a by m
    if (gp <= flightPercent) {
        x = (gp/flightPercent)*stepLength - stepLength/2.0;
        y = -upAMP*sin(PI*gp/flightPercent) + stanceHeight;
    }
    else {
        float percentBack = (gp-flightPercent)/(1.0-flightPercent);
        x = -percentBack*stepLength + stepLength/2.0;
        y = downAMP*sin(PI*percentBack) + stanceHeight;
    }
}

void CartesianToThetaGamma(float x, float y, float leg_direction, float& theta, float& gamma) {
    float L = 0.0;
    CartesianToLegParams(x, y, leg_direction, L, theta);
    GetGamma(L, theta, gamma);
    Serial.print(theta);
    Serial.print(" ");
    Serial.print(gamma);
    Serial.print('\n');
}

bool isValidGaitParams(struct GaitParams params) {
    const float maxL = 0.25;
    const float minL = 0.08;

    float stanceHeight = params.stanceHeight;
    float downAMP = params.downAMP;
    float upAMP = params.upAMP;
    float flightPercent = params.flightPercent;
    float stepLength = params.stepLength;
    float FREQ = params.FREQ;

    if (stanceHeight + downAMP > maxL || sqrt(pow(stanceHeight, 2) + pow(stepLength / 2.0, 2)) > maxL) {
        Serial.println("Gait overextends leg");
        return false;
    }
    if (stanceHeight - upAMP < minL) {
        Serial.println("Gait underextends leg");
        return false;
    }

    if (flightPercent <= 0 || flightPercent > 1.0) {
        Serial.println("Flight percent is invalid");
        return false;
    }

    if (FREQ < 0) {
        Serial.println("Frequency cannot be negative");
        return false;
    }

    return true;
}

void CoupledMoveLeg(ODriveArduino& odrive, float t, struct GaitParams params, float gait_offset, float leg_direction, struct LegGain gains) {
    float theta;
    float gamma;
    float x; // float x for leg 0 to be set by the sin trajectory
    float y;
    SinTrajectory(t, params, gait_offset, x, y);
    CartesianToThetaGamma(x, y, leg_direction, theta, gamma);
    odrive.SetCoupledPosition(theta, gamma, gains);
}

void gait(struct GaitParams params, float leg0_offset, float leg1_offset, float leg2_offset, float leg3_offset, struct LegGain gains) {
    if (!isValidGaitParams(params)) {
        return; // gait function will not run with invalid parameters
    }

    float t = millis()/1000.0;

    const float leg0_direction = -1.0;
    CoupledMoveLeg(odrv0Interface, t, params, leg0_offset, leg0_direction, gains);

    const float leg1_direction = -1.0;
    CoupledMoveLeg(odrv1Interface, t, params, leg1_offset, leg1_direction, gains);

    const float leg2_direction = 1.0;
    CoupledMoveLeg(odrv2Interface, t, params, leg2_offset, leg2_direction, gains);

    const float leg3_direction = 1.0;
    CoupledMoveLeg(odrv3Interface, t, params, leg3_offset, leg3_direction, gains);
}

/**
* Pronk gait parameters
*/
void pronk() {
    // {stanceHeight, downAMP, upAMP, flightPercent, stepLength, FREQ}
    struct GaitParams params = {0.12, 0.09, 0.0, 0.9, 0.0, 0.8};
    struct LegGain gains = {120, 0.48, 80, 0.48};
    gait(params, 0.0, 0.0, 0.0, 0.0, gains);
}

/**
* Trot gait parameters
*/
void bound() {
    // {stanceHeight, downAMP, upAMP, flightPercent, stepLength, FREQ}
    struct GaitParams params = {0.15, 0.0, 0.05, 0.35, 0.0, 1.0};
    struct LegGain gains = {120, 0.48, 80, 0.48};
    gait(params, 0.0, 0.5, 0.5, 0.0, gains);
}

/**
* Bound gait parameters
*/
void trot() {
    // {stanceHeight, downAMP, upAMP, flightPercent, stepLength, FREQ}
    struct GaitParams params = {0.18, 0.0, 0.06, 0.6, 0.0, 2.0};
    struct LegGain gains = {120, 0.48, 80, 0.48};
    gait(params, 0.0, 0.5, 0.0, 0.5, gains);
}
