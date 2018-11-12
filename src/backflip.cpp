#include "backflip.h"
#include "ODriveArduino.h"
#include "globals.h"
#include "position_control.h"
#include "imu.h"

float flip_start_time_ = 0.0f;

void pointDown() {
    float pitch = global_debug_values.imu.pitch;
    if (pitch > M_PI/2 || pitch < -M_PI/2) return;
    float y = gait_params.stance_height;
    float theta, gamma;
    CartesianToThetaGamma(0.0, y, 1, theta, gamma);
    odrv0Interface.SetCoupledPosition(pitch, gamma, gait_gains);
    odrv1Interface.SetCoupledPosition(pitch, gamma, gait_gains);
    odrv2Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
    odrv3Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
}

void StartFlip(float start_time_s) {
    state = FLIP;
    IMUTarePitch();
    Serial.println("FLIP");
    flip_start_time_ = start_time_s;
    //            {s.h, d.a., u.a., f.p., s.l., fr.}
    gait_params = {0.15, 0.05, 0.06, 0.2, 0, 1.0};
    gait_gains = {120,1,140,1};
    PrintGaitParams();
}

void ExecuteFlip() {
    const float prep_time = 0.6f; // Duration before jumping [s]
    const float launch_time = 0.1f; // Duration before retracting the leg [s]

    struct LegGain rear_gains = {120,1,80,1};
    float rear_up_amp = 0.75f * 0.06f;
    float rear_down_amp = 0.07f;

    float t = millis()/1000.0f - flip_start_time_;
    float pitch = global_debug_values.imu.pitch;

    float theta=0,gamma=0;
    if (t < prep_time) {
        float y = gait_params.stance_height - rear_up_amp;
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv1Interface.SetCoupledPosition(pitch, gamma, rear_gains);
        odrv2Interface.SetCoupledPosition(-pitch, gamma, rear_gains);

        y = gait_params.stance_height - gait_params.up_amp;
        CartesianToThetaGamma(0.0, y, -1, theta, gamma);
        odrv0Interface.SetCoupledPosition(pitch, gamma, gait_gains);
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv3Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
    } else if (t >= prep_time && t < prep_time + launch_time) {
        float y = gait_params.stance_height - rear_up_amp;
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv1Interface.SetCoupledPosition(pitch, gamma, rear_gains);
        odrv2Interface.SetCoupledPosition(-pitch, gamma, rear_gains);

        y = gait_params.stance_height + gait_params.down_amp;
        CartesianToThetaGamma(0.0, y, -1, theta, gamma);
        odrv0Interface.SetCoupledPosition(pitch, gamma, gait_gains);
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv3Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
    } else if (pitch < 85.0*M_PI/180.0) {
        float y = gait_params.stance_height - rear_up_amp;
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv1Interface.SetCoupledPosition(pitch, gamma, rear_gains);
        odrv2Interface.SetCoupledPosition(-pitch, gamma, rear_gains);

        y = gait_params.stance_height;
        CartesianToThetaGamma(0.0, y, -1, theta, gamma);
        odrv0Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv3Interface.SetCoupledPosition(pitch, gamma, gait_gains);
    } else {
        float y = gait_params.stance_height + rear_down_amp;
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv1Interface.SetCoupledPosition(pitch, gamma, gait_gains);
        odrv2Interface.SetCoupledPosition(-pitch, gamma, gait_gains);

        y = gait_params.stance_height;
        CartesianToThetaGamma(0.0, y, -1, theta, gamma);
        odrv0Interface.SetCoupledPosition(-pitch, gamma, gait_gains);
        CartesianToThetaGamma(0.0, y, 1, theta, gamma);
        odrv3Interface.SetCoupledPosition(pitch, gamma, gait_gains);
    }

    global_debug_values.odrv0.sp_theta = -pitch;
    global_debug_values.odrv0.sp_gamma = gamma;
    global_debug_values.odrv3.sp_theta = pitch;
    global_debug_values.odrv3.sp_gamma = gamma;

    CartesianToThetaGamma(0.0, gait_params.stance_height, 1, theta, gamma);
    global_debug_values.odrv1.sp_theta = -pitch;
    global_debug_values.odrv1.sp_gamma = gamma;
    global_debug_values.odrv2.sp_theta = pitch;
    global_debug_values.odrv2.sp_gamma = gamma;
}
