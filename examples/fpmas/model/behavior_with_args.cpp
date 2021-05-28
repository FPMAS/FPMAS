#include "fpmas.h"

/**
 * @example fpmas/model/behavior_with_args.cpp
 *
 * BehaviorWithArgs example usage.
 *
 *
 * @par output
 * ```
 * 8: hello
 * 8: world
 * ```
 */

class ExampleAgent : public fpmas::model::AgentBase<ExampleAgent> {
	public:
		void behavior(int n, std::string& message) {
			std::cout << n << ": " << message << std::endl;
		}
};

int main() {
	std::string message = "hello";

	fpmas::model::BehaviorWithArgs<ExampleAgent, int, std::string&> behavior {
		&ExampleAgent::behavior, 8, message
	};

	ExampleAgent agent;
	behavior.execute(&agent);

	message = "world";

	behavior.execute(&agent);
}
