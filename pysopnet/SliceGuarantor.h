#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include <catmaid/ProjectConfiguration.h>
#include "SliceGuarantorParameters.h"
#include "BackendClient.h"
#include "Locations.h"

namespace python {

class SliceGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of slices in a block.
	 *
	 * @param blockLocation
	 *             The location of the requested block.
	 *
	 * @param parameters
	 *             Slice extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 *
	 * @return
	 *             A list of block locations, for which stack images are needed
	 *             to process the request. Empty on success.
	 */
	Locations fill(
			const util::point3<unsigned int>& blockLocation,
			const SliceGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

