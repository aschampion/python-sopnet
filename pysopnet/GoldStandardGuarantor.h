#ifndef SOPNET_PYTHON_GOLD_STANDARD_GUARANTOR_H__
#define SOPNET_PYTHON_GOLD_STANDARD_GUARANTOR_H__

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/BackendClient.h>
#include "GoldStandardGuarantorParameters.h"
#include "Locations.h"

namespace python {

class GoldStandardGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of a solution from a set of
	 * membrane segment hypotheses best matching a ground truth labeling.
	 *
	 * @param coreLocation
	 *             The location of the requested core.
	 *
	 * @param parameters
	 *             Gold standard extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 *
	 * @return
	 *             A list of block locations, for which additional data is
	 *             needed to process the request. Empty on success.
	 */
	Locations fill(
			const util::point<unsigned int, 3>& coreLocation,
			const GoldStandardGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_GOLD_STANDARD_GUARANTOR_H__
