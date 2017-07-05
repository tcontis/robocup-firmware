#pragma once

#include <array>
#include "Pid.hpp"
#include "fpga.hpp"
#include "RobotModel.hpp"

/**
 * Robot controller that runs a PID loop on each of the four wheels.
 */
class PidMotionController {
public:
    PidMotionController() {
        setPidValues(1, 0, 0);
    }

    void setPidValues(float p, float i, float d) {
        for (Pid& ctl : _controllers) {
            ctl.kp = p;
            ctl.ki = i;
            ctl.kd = d;
        }
    }

    void setTargetVel(Eigen::Vector3f target) { _targetVel = target; }

    /**
     * Return the duty cycle values for the motors to drive at the target
     * velocity.
     *
     * @param encoderDeltas Encoder deltas for the four drive motors
     * @param dt Time in ms since the last calll to run()
     *
     * @return Duty cycle values for each of the 4 motors
     */
    std::array<int16_t, 4> run(const std::array<int16_t, 4>& encoderDeltas,
                               float dt) {
        // convert encoder ticks to rad/s
        Eigen::Vector4f wheelVels;
        wheelVels << encoderDeltas[0], encoderDeltas[1], encoderDeltas[2],
            encoderDeltas[3];
        wheelVels *= 2 * M_PI / ENC_TICKS_PER_TURN / dt;

        Eigen::Vector4f targetWheelVels =
            RobotModel2015.BotToWheel * _targetVel;
        // Forwards
        //Eigen::Vector4f targetWheelVels(.288675, .32169, -.32169, -.288675);
        //Eigen::Vector4f targetWheelVels(.32169, .288675, -.288675, -.32169);

        /*
        float front = .34641;
        float back = .257352;
        Eigen::Vector4f targetWheelVels(front, back, -back, -front);
        */
        //Eigen::Vector4f targetWheelVels(.4, -.317803, -.476705, .6);

        // Right
        //Eigen::Vector4f targetWheelVels(.5, -.397254, -.397254, .5);
        //Eigen::Vector4f targetWheelVels(.397254, -.5, -.5, .397254);
        //Eigen::Vector4f targetWheelVels(0, 0, 0, 1);
        //targetWheelVels /= RobotModel2015.WheelRadius;

        Eigen::Vector4f wheelVelErr = targetWheelVels - wheelVels;


        std::array<int16_t, 4> dutyCycles;
        for (int i = 0; i < 4; i++) {
            int16_t dc = targetWheelVels[i] * RobotModel2015.DutyCycleMultiplier;
            //int16_t dc = _controllers[i].run(wheelVelErr[i]);
            dc += _controllers[i].run(wheelVelErr[i], dt);

            if (std::abs(dc) > FPGA::MAX_DUTY_CYCLE) {
                // Limit to max duty cycle
                dc = copysign(FPGA::MAX_DUTY_CYCLE, dc);
                // Conditional integration indicating open loop control
                _controllers[i].set_saturated(true);
            } else {
                _controllers[i].set_saturated(false);
            }

            dutyCycles[i] = dc;
        }

// enable these printouts to get a python-formatted data set than can be
// graphed to visualize pid control and debug problems
#if 0
        printf("{\r\n");
        /*
        printf("'encDelt': [%d, %d, %d, %d],\r\n", encoderDeltas[0], encoderDeltas[1], encoderDeltas[2], encoderDeltas[3]);
        printf("'dt': %f,\r\n", dt);
        */
        printf("'targetVel': [%f, %f, %f]", _targetVel[0], _targetVel[1], _targetVel[2]);
        //printf("'wheelVels': [%f, %f, %f, %f],\r\n", wheelVels[0], wheelVels[1], wheelVels[2], wheelVels[3]);
        //printf("'targetWheelVels': [%f, %f, %f, %f],\r\n", targetWheelVels[0], targetWheelVels[1], targetWheelVels[2], targetWheelVels[3]);
        /*
        printf("'duty': [%d, %d, %d, %d],\r\n", dutyCycles[0], dutyCycles[1], dutyCycles[2], dutyCycles[3]);
        */
        printf("},\r\n");
#endif

        return dutyCycles;
    }

    static const uint16_t ENC_TICKS_PER_TURN = 2048;

private:
    /// controllers for each wheel
    std::array<Pid, 4> _controllers;

    Eigen::Vector3f _targetVel;
};
