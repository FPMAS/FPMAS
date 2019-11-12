#ifndef DISTRIBUTION_MODE_H
#define DISTRIBUTION_MODE_H

enum DistributionMode {
	GRAPH
};

template<class T> class Foo{};

class SimulationParameters {

	private:
		DistributionMode distributionMode = GRAPH;
	
	public:
		DistributionMode getDistributionMode() const;

};

#endif
