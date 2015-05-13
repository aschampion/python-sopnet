#ifndef SOPNET_INFERENCE_SOLUTIONS_H__
#define SOPNET_INFERENCE_SOLUTIONS_H__

#include <pipeline/all.h>
#include <solvers/Solution.h>

/**
 * Collection of solutions for a set of problems.
 */
class Solutions : public pipeline::Data {

public:

	void addSolution(boost::shared_ptr<Solution> solution) {

		_solutions.push_back(solution);
	}

	boost::shared_ptr<Solution> getSolution(unsigned int i) {

		return _solutions[i];
	}

	unsigned int size() {

		return _solutions.size();
	}

	void clear() {

		_solutions.clear();
	}

private:

	std::vector<boost::shared_ptr<Solution> > _solutions;
};

#endif // SOPNET_INFERENCE_SOLUTIONS_H__

