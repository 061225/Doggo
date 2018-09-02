#include "jump.h"
#include "ODriveArduino.h"
#include "globals.h"
#include "position_control.h"

// Privates
bool execute_jump_ = false;
float start_time_ = 0.0f;

/**
 * Tell the position control thread to do the jump
 * @param start_time_s The timestamp of when the jump command was sent
 */
void StartJump(float start_time_s) {
    start_time_ = start_time_s;
    execute_jump_ = true;
}
/**
 * Query if a jump should be executed
 */
bool ShouldExecuteJump() {
    return execute_jump_;
}

/**
* Linear increase in height for jump.
*/
void TrajectoryJump(float t, float launchTime, float stanceHeight,
    float downAMP, float& x, float& y) {
    //Need to check if n works
    float n = t/launchTime;
    x = 0;
    y = downAMP*n + stanceHeight;
    //y = downAMP*sin(PI/4 + PI/4*n) + stanceHeight;
}

/**
* Drives the ODrives in an open-loop, position-control sinTrajectory.
*/
void ExecuteJump() {
    Serial.println("Jump Initialized");

    // min radius = 0.8
    // max radius = 0.25
    const float prep_time = 0.5f; // Duration before jumping [s]
    const float launch_time = 5.0f ; // Duration before retracting the leg [s]
    const float fall_time = 1.0f; //Duration after retracting leg to go back to normal behavior [s]

    const float stance_height = 0.09f; // Desired leg extension before the jump [m]
    const float jump_extension = 0.24f; // Maximum leg extension in [m]
    const float fall_extension = 0.13f; // Desired leg extension during fall [m]

    float t = millis()/1000.0f - start_time_; // Seconds since jump was commanded

    if (t < prep_time) {
        float x = 0;
        float y = stance_height;
        float theta,gamma;
        CartesianToThetaGamma(x, y, 1.0, theta, gamma);

        // Use gains with small stiffness and lots of damping
        struct LegGain gains = {50, 1.0, 50, 1.0};
        CommandAllLegs(theta,gamma,gains);
        Serial << "Jump: +" << t << "s, y: " << y;
    } else if (t >= prep_time && t < prep_time + launch_time) {
        float x = 0;
        float y = jump_extension;
        float theta, gamma;
        CartesianToThetaGamma(x, y, 1.0, theta, gamma);

        // Use high stiffness and low damping to execute the jump
        struct LegGain gains = {160, 0.5, 160, 0.3};
        CommandAllLegs(theta, gamma, gains);
        Serial << "Jump: +" << t << "s, y: " << y;
    } else if (t >= prep_time + launch_time && t < prep_time + launch_time + fall_time) {
        float x = 0;
        float y = fall_extension;
        float theta,gamma;
        CartesianToThetaGamma(x, y, 1.0, theta, gamma);

        // Use low stiffness and lots of damping to handle the fall
        struct LegGain gains = {50, 1.0, 50, 1.0};

        CommandAllLegs(theta, gamma, gains);
        Serial << "Jump: +" << t << "s, y: " << y;
    } else {
        execute_jump_ = false;
        Serial.println("Jump Complete.");
    }
}

void CommandAllLegs(float theta, float gamma, LegGain gains) {
    odrv0Interface.SetCoupledPosition(theta, gamma, gains);
    odrv1Interface.SetCoupledPosition(theta, gamma, gains);
    odrv2Interface.SetCoupledPosition(theta, gamma, gains);
    odrv3Interface.SetCoupledPosition(theta, gamma, gains);
}
