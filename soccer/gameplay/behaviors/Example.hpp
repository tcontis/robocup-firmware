#pragma once

#include "../Behavior.hpp"

namespace Gameplay
{
	namespace Behaviors
	{
		class ChangeMe: public Behavior
		{
			public:
				ChangeMe(GameplayModule *gameplay);
				
				virtual bool run();
				virtual bool done();
				
				virtual float score(Robot* robot);
		};
	}
}
