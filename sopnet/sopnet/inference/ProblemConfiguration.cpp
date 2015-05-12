#include <util/Logger.h>
#include "ProblemConfiguration.h"

logger::LogChannel problemconfigurationlog("problemconfigurationlog", "[ProblemConfiguration] ");

ProblemConfiguration::ProblemConfiguration() {

	clear();
}

void
ProblemConfiguration::setVariable(const Segment& segment, unsigned int variable) {

	setVariable(segment.getId(), variable);
	fit(segment);
	_interSectionIntervals[variable] = segment.getInterSectionInterval();
}

void
ProblemConfiguration::setVariable(unsigned int segmentId, unsigned int variable) {

	_variables[segmentId] = variable;
	_segmentIds[variable] = segmentId;
}

unsigned int
ProblemConfiguration::getVariable(unsigned int segmentId) {

	if (!_variables.count(segmentId))
		BOOST_THROW_EXCEPTION(
				NoSuchSegment()
				<< error_message(
						std::string("variable map does not contain an entry for segment id ") +
						boost::lexical_cast<std::string>(segmentId))
				<< STACK_TRACE);

	return _variables[segmentId];
}

unsigned int
ProblemConfiguration::getSegmentId(unsigned int variable) {

	if (!_segmentIds.count(variable))
		BOOST_THROW_EXCEPTION(
				NoSuchSegment()
				<< error_message(
						std::string("segment id map does not contain an entry for variable ") +
						boost::lexical_cast<std::string>(variable))
				<< STACK_TRACE);

	return _segmentIds[variable];
}

std::vector<unsigned int>
ProblemConfiguration::getVariables(unsigned int minInterSectionInterval, unsigned int maxInterSectionInterval) {

	std::vector<unsigned int> variables;
	for (const auto& pair : _interSectionIntervals) {

		if (pair.second >= minInterSectionInterval && pair.second < maxInterSectionInterval)
			variables.push_back(pair.first);
	}

	return variables;
}

std::set<unsigned int>
ProblemConfiguration::getVariables() {

	unsigned int variable;
	unsigned int segmentId;

	std::set<unsigned int> variables;

	foreach (boost::tie(variable, segmentId), _segmentIds)
		variables.insert(variable);

	return variables;
}

void
ProblemConfiguration::clear() {

	_variables.clear();
	_segmentIds.clear();

	_minInterSectionInterval = -1;
	_maxInterSectionInterval = -1;

	_minX = -1; _maxX = -1;
	_minY = -1; _maxY = -1;
}

void
ProblemConfiguration::fit(const Segment& segment) {

	LOG_ALL(problemconfigurationlog) << "fitting segment " << segment.getId() << " with inter-section interval " << segment.getInterSectionInterval() << std::endl;

	if (_minInterSectionInterval < 0) {

		_minInterSectionInterval = segment.getInterSectionInterval();
		_maxInterSectionInterval = segment.getInterSectionInterval();
		_minX = (int)segment.getCenter().x();
		_maxX = (int)segment.getCenter().x();
		_minY = (int)segment.getCenter().y();
		_maxY = (int)segment.getCenter().y();

	} else {

		_minInterSectionInterval = std::min(_minInterSectionInterval, (int)segment.getInterSectionInterval());
		_maxInterSectionInterval = std::max(_maxInterSectionInterval, (int)segment.getInterSectionInterval());
		_minX = std::min(_minX, (int)segment.getCenter().x());
		_maxX = std::max(_maxX, (int)segment.getCenter().x());
		_minY = std::min(_minY, (int)segment.getCenter().y());
		_maxY = std::max(_maxY, (int)segment.getCenter().y());
	}

	LOG_ALL(problemconfigurationlog) << "extents are now " << _minInterSectionInterval << "-" << _maxInterSectionInterval << std::endl;
}
