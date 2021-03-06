#ifndef SOPNET_PYTHON_PROJECT_CONFIGURATION_H__
#define SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

#include <string>
#include <util/point3.hpp>

namespace python {

/**
 * Project specific configuration to be passed to all the stateless python 
 * wrappers.
 */
class ProjectConfiguration {

public:

	enum BackendType {

		/**
		 * Use local stores. For this, the block size, the volume size, and the 
		 * core size have to be set.
		 */
		Local,

		/**
		 * Use Django stores. For this, the URL to the Django server, the 
		 * hostname of the CATMAID instance, and the CATMAID project and stack 
		 * id have to be set.
		 */
		Django
	};

	/**
	 * Set the backend type (Local or Django).
	 */
	void setBackendType(BackendType type);

	/**
	 * Get the backend type (Local or Django).
	 */
	BackendType getBackendType() const;

	/**
	 * Set the CATMAID host name in the form "host:port". The ":port" can be 
	 * omited if it is 80 (http).
	 */
	void setCatmaidHost(const std::string& host);

	/**
	 * Get the CATMAID host name.
	 */
	const std::string& getCatmaidHost() const;

	/**
	 * Set the CATMAID stack id of the raw data to be used in the Django 
	 * backend.
	 */
	void setCatmaidRawStackId(unsigned int stackId);

	/**
	 * Get the CATMAID stack id of the raw data to be used in the Django 
	 * backend.
	 */
	unsigned int getCatmaidRawStackId() const;

	/**
	 * Set the CATMAID stack id of the membrane data to be used in the Django 
	 * backend.
	 */
	void setCatmaidMembraneStackId(unsigned int stackId);

	/**
	 * Get the CATMAID stack id of the membrane data to be used in the Django 
	 * backend.
	 */
	unsigned int getCatmaidMembraneStackId() const;

	/**
	 * Set the CATMAID project id to be used in the Django backend.
	 */
	void setCatmaidProjectId(unsigned int projectId);

	/**
	 * Get the CATMAID project id to be used in the Django backend.
	 */
	unsigned int getCatmaidProjectId() const;

	/**
	 * Set the size of a block in voxels.
	 */
	void setBlockSize(const util::point3<unsigned int>& blockSize);

	/**
	 * Get the size of a block in voxels.
	 */
	const util::point3<unsigned int>& getBlockSize() const;

	/**
	 * Set the size of the whole volume in voxels.
	 */
	void setVolumeSize(const util::point3<unsigned int>& volumeSize);

	/**
	 * Get the size of a volume in voxels.
	 */
	const util::point3<unsigned int>& getVolumeSize() const;

	/**
	 * Set the size of a core in blocks.
	 */
	void setCoreSize(const util::point3<unsigned int>& coreSizeInBlocks);

	/**
	 * Get the size of a core in blocks.
	 */
	const util::point3<unsigned int>& getCoreSize() const;

private:

	BackendType _backendType;

	std::string _catmaidHost;

	unsigned int _rawStackId;
	unsigned int _membraneStackId;
	unsigned int _projectId;

	util::point3<unsigned int> _blockSize;
	util::point3<unsigned int> _coreSize;
	util::point3<unsigned int> _volumeSize;
	util::point3<unsigned int> _coreSizeInBlocks;
};

} // namespace python

#endif // SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

