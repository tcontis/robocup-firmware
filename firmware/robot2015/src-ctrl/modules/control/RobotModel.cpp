#include "RobotModel.hpp"

const RobotModel RobotModel2015 = []() {
    RobotModel model;
    model.WheelRadius = 0.02856;
    // note: wheels are numbered clockwise, starting with the top-right
    // 30 degrees for front wheels, 39 degrees for back wheels
    model.WheelAngles = {
        DegreesToRadians(0 + 30), // M4
        DegreesToRadians(360 - 39), // M3
        DegreesToRadians(180 + 39), // M2
        DegreesToRadians(180 - 30), // M1
    };
    model.WheelDist = 0.0798576;

    model.DutyCycleMultiplier = 9;  // TODO: tune this value

    model.recalculateBotToWheel();

    return model;
}();
