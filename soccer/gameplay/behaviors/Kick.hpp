#pragma once

#include "../Behavior.hpp"

#include "Intercept.hpp"
#include <iostream>

namespace Gameplay
{
	class Window;
	class WindowEvaluator;

	namespace Behaviors
	{
		class Kick: public Behavior
		{
			public:
				Kick(GameplayModule *gameplay);
				~Kick();

				virtual bool assign(std::set<Robot *> &available);
				virtual bool run();

				bool isIntercept()
				{
					return _state == Intercept;
				}

				Robot *targetRobot;
				bool automatic;

				enum State
				{
					Intercept,
					Aim,
					Shoot,
					Done
				};
				State getState() {return _state;}

				/** Restarts the kick play - keep going after ball */
				void restart() {_state = Intercept;}

				/**
				 * velocity bounding close to the ball
				 * Set scale to 1.0 for no thresholding
				 */
				void setVScale(float scale, float range) {
					_ballHandlingScale = scale;
					_ballHandlingRange = range;
				}

			protected:
				virtual float score(Robot* robot);

				WindowEvaluator *_win;
				Gameplay::Behaviors::Intercept* _intercept;

				State _state;
				float _lastMargin;
				Geometry2d::Segment _target;

				//we lock in a pivot point
				Geometry2d::Point _pivot;

				// The robot's position when it entered the Shoot state
				Geometry2d::Point _shootStart;

				Geometry2d::Point _shootMove;
				Geometry2d::Point _shootBallStart;

				// velocity scaling close to the ball
				float _ballHandlingScale;
				float _ballHandlingRange;

				// Kick evaluation via obstacles
				Geometry2d::Segment evaluatePass(); /// finds a pass segment
				Geometry2d::Segment evaluateShot(); /// finds a shot segment

				// determining how hard to kick
				int calcKickStrength(const Geometry2d::Point& targetCenter);

				// checks whether other robots intersect the shot properly
				bool checkRobotIntersections(const Geometry2d::Segment& shotLine);

				// Individual state functions - these should execute a state's
				// activities, and return what the next state should be
				State intercept(const Geometry2d::Point& targetCenter);
				State aim(const Geometry2d::Point& targetCenter, bool canKick);
				State shoot(const Geometry2d::Point& targetCenter, int kickStrength);
		};
	}
}
