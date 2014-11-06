#include "config.h"
#ifdef HAVE_PostgreSQL

#include <boost/tokenizer.hpp>
#include <boost/timer/timer.hpp>
#include <libpqtypes.h>
#include <util/Logger.h>
#include "PostgreSqlSegmentStore.h"
#include "PostgreSqlUtils.h"

logger::LogChannel postgresqlsegmentstorelog("postgresqlsegmentstorelog", "[PostgreSqlSegmentStore] ");

PostgreSqlSegmentStore::PostgreSqlSegmentStore(
		const ProjectConfiguration& config) : _config(config)
{
	_pgConnection = PostgreSqlUtils::getConnection(
			_config.getPostgreSqlHost(),
			_config.getPostgreSqlPort(),
			_config.getPostgreSqlDatabase(),
			_config.getPostgreSqlUser(),
			_config.getPostgreSqlPassword());
	PQinitTypes(_pgConnection);
}

PostgreSqlSegmentStore::~PostgreSqlSegmentStore() {

	if (_pgConnection != 0)
		PQfinish(_pgConnection);
}

void
PostgreSqlSegmentStore::associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) {

	PGresult* queryResult;
	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				block, _config.getCatmaidRawStackId());

	queryResult = PQexec(_pgConnection, blockQuery.c_str());
	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockQuery);
	std::string blockId(PQgetvalue(queryResult, 0, 0));
	PQclear(queryResult);

	boost::timer::cpu_timer queryTimer;

	// Remove any existing segment associations for this block.
	std::ostringstream clearBlockQuery;
	clearBlockQuery <<
			"DELETE FROM djsopnet_segmentblockrelation WHERE id = " + blockId;

	std::ostringstream segmentQuery;
	segmentQuery <<
			"INSERT INTO djsopnet_segment (id, stack_id, section_inf, "
			"min_x, min_y, max_x, max_y, ctr_x, ctr_y, type) VALUES";

	std::ostringstream sliceQuery;
	sliceQuery <<
			"INSERT INTO djsopnet_segmentslice (segment_id, slice_id, direction) VALUES";

	std::ostringstream segmentFeatureQuery;
	segmentFeatureQuery <<
			"INSERT INTO djsopnet_segmentfeatures (segment_id, features) VALUES";

	std::ostringstream blockSegmentQuery;
	blockSegmentQuery <<
			"INSERT INTO djsopnet_segmentblockrelation (block_id, segment_id) VALUES";

	char separator = ' ';
	char sliceSeparator = ' ';

	foreach (const SegmentDescription& segment, segments) {
		std::string segmentId = boost::lexical_cast<std::string>(
			PostgreSqlUtils::hashToPostgreSqlId(segment.getHash()));
		const util::rect<unsigned int>& segmentBounds = segment.get2DBoundingBox();
		const util::point<double>& segmentCenter = segment.getCenter();

		// Create segment.
		segmentQuery << separator << '(' <<
				boost::lexical_cast<std::string>(segmentId) << ", " <<
				boost::lexical_cast<std::string>(_config.getCatmaidRawStackId()) << ", " <<
				boost::lexical_cast<std::string>(segment.getSection()) << ", " <<
				boost::lexical_cast<std::string>(segmentBounds.minX) << ", " <<
				boost::lexical_cast<std::string>(segmentBounds.minY) << ", " <<
				boost::lexical_cast<std::string>(segmentBounds.maxX) << ", " <<
				boost::lexical_cast<std::string>(segmentBounds.maxY) << ", " <<
				boost::lexical_cast<std::string>(segmentCenter.x) << ", " <<
				boost::lexical_cast<std::string>(segmentCenter.y) << ", " <<
				boost::lexical_cast<std::string>(segment.getType()) << ')';

		// Associate slices to segment.
		foreach (const SliceHash& hash, segment.getLeftSlices()) {
			sliceQuery << sliceSeparator << '(' <<
				segmentId << ',' <<
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) <<
				",TRUE)";
			sliceSeparator = ',';
		}

		foreach (const SliceHash& hash, segment.getRightSlices()) {
			sliceQuery << sliceSeparator << '(' <<
				segmentId << ',' <<
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) <<
				",FALSE)";
			sliceSeparator = ',';
		}

		// Store segment features.
		segmentFeatureQuery << separator << '(' << segmentId << ", '{";
		char featureSeparator = ' ';
		foreach (const double featVal, segment.getFeatures()) {
			segmentFeatureQuery << featureSeparator << boost::lexical_cast<std::string>(featVal);
			featureSeparator = ',';
		}
		segmentFeatureQuery << "}')";

		// Associate segment to block.
		blockSegmentQuery << separator << '(' << blockId << ',' << segmentId << ')';

		separator = ',';
	}

	segmentQuery << ';' << sliceQuery.str() << ';' << segmentFeatureQuery.str()
			<< ';' << clearBlockQuery.str() << ';' << blockSegmentQuery.str() << ';';

	// Set block flag to show segments have been stored.
	segmentQuery <<
			"UPDATE djsopnet_block SET segments_flag = TRUE WHERE id = " << blockId;
	std::string query = segmentQuery.str();
	queryResult = PQexec(_pgConnection, query.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, query);
	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Stored " << segments.size() << " segments in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * segments.size()/queryElapsed.count()) << " segments/s)" << std::endl;
}

boost::shared_ptr<SegmentDescriptions>
PostgreSqlSegmentStore::getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {

	boost::shared_ptr<SegmentDescriptions> segmentDescriptions =
			boost::make_shared<SegmentDescriptions>();

	if (blocks.empty()) return segmentDescriptions;

	boost::timer::cpu_timer queryTimer;

	// Check if any requested block do not have slices flagged.
	std::string blockIdsStr = PostgreSqlUtils::checkBlocksFlags(
			blocks, _config.getCatmaidRawStackId(),
			"segments_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return segmentDescriptions;

	// Register composite segmentslice field row type for libpqtypes.
	PGregisterType segmentsliceType = {"pgsqlsegmentstore_segslice", NULL, NULL};
	PostgreSqlUtils::checkPQTypesError(
		PQregisterTypes(_pgConnection, PQT_COMPOSITE, &segmentsliceType, 1, 0));

	// Prepare the libpqtype specifier for result rows
	PostgreSqlUtils::checkPQTypesError(PQspecPrepare(_pgConnection, "segmentrow_spec",
			"%int8 %int4 %int4 %int4 %int4 %int4 "
			"%float8 %float8 %float8[] %pgsqlsegmentstore_segslice[]", 0));

	// Query segments for this set of blocks
	std::string blockSegmentsQuery =
			"SELECT s.id, s.section_inf, s.min_x, s.min_y, s.max_x, s.max_y, "
			"s.ctr_x, s.ctr_y, sf.id, sf.features, " // sf.id is needed for GROUP
			"array_agg(DISTINCT (ss.slice_id, ss.direction)::pgsqlsegmentstore_segslice) "
			"FROM djsopnet_segmentblockrelation sbr "
			"JOIN djsopnet_segment s ON sbr.segment_id = s.id "
			"JOIN djsopnet_segmentslice ss ON s.id = ss.segment_id "
			"JOIN djsopnet_segmentfeatures sf ON s.id = sf.segment_id "
			"WHERE sbr.block_id IN (" + blockIdsStr + ") "
			"GROUP BY s.id, sf.id";

	LOG_DEBUG(postgresqlsegmentstorelog) << blockSegmentsQuery << std::endl;

	enum { FIELD_ID, FIELD_SECTION, FIELD_MIN_X, FIELD_MIN_Y,
			FIELD_MAX_X, FIELD_MAX_Y, FIELD_CTR_X, FIELD_CTR_Y,
			FIELD_SFID_UNUSED, FIELD_FEATURES, FIELD_SLICE_ARRAY };
	PGresult* queryResult = PQparamExec(_pgConnection, NULL, blockSegmentsQuery.c_str(), 1);

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockSegmentsQuery);
	int nSegments = PQntuples(queryResult);

	struct {
		PGint8 id;
		PGint4 section;
		PGint4 min_x; PGint4 min_y;
		PGint4 max_x; PGint4 max_y;
		PGfloat8 ctr_x; PGfloat8 ctr_y;
		PGarray features;
		PGarray slices;
	} row;

	// Build SegmentDescription for each row
	for (int i = 0; i < nSegments; ++i) {
		PostgreSqlUtils::checkPQTypesError(PQgetf(queryResult, i, "@segmentrow_spec",
				FIELD_ID, &row.id,
				FIELD_SECTION, &row.section,
				FIELD_MIN_X, &row.min_x, FIELD_MIN_Y, &row.min_y,
				FIELD_MAX_X, &row.min_x, FIELD_MAX_Y, &row.min_y,
				FIELD_CTR_X, &row.min_x, FIELD_CTR_Y, &row.min_y,
				FIELD_FEATURES, &row.features,
				FIELD_SLICE_ARRAY, &row.slices));
		SegmentHash segmentHash = PostgreSqlUtils::postgreSqlIdToHash(
				static_cast<PostgreSqlHash>(row.id));
		unsigned int section = static_cast<unsigned int>(row.section);
		unsigned int minX = static_cast<unsigned int>(row.min_x);
		unsigned int minY = static_cast<unsigned int>(row.min_y);
		unsigned int maxX = static_cast<unsigned int>(row.max_x);
		unsigned int maxY = static_cast<unsigned int>(row.max_y);
		double ctrX = static_cast<double>(row.ctr_x);
		double ctrY = static_cast<double>(row.ctr_y);
		SegmentDescription segmentDescription(
				section,
				util::rect<unsigned int>(minX, minY, maxX, maxY),
				util::point<double>(ctrX, ctrY));

		int nFeatures = PQntuples(row.features.res);
		std::vector<double> segmentFeatures;
		PGfloat8 feature;
		for (int j = 0; j < nFeatures; ++j) {
			PostgreSqlUtils::checkPQTypesError(
					PQgetf(row.features.res, j, "%float8", 0, &feature));
			segmentFeatures.push_back(static_cast<double>(feature));
		}

		segmentDescription.setFeatures(segmentFeatures);

		int nSlices = PQntuples(row.slices.res);
		PGint8 sliceId;
		PGbool direction;
		for (int j = 0; j < nSlices; ++j) {
			PostgreSqlUtils::checkPQTypesError(
					PQgetf(row.slices.res, j, "%int8 %bool", 0, &sliceId, 1, &direction));
			SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
					static_cast<PostgreSqlHash>(sliceId));
			if (direction) segmentDescription.addLeftSlice(sliceHash);
			else segmentDescription.addRightSlice(sliceHash);
		}

		PQclear(row.features.res);
		PQclear(row.slices.res);

		// Check that the loaded segment has the correct hash.
		if (segmentDescription.getHash() != segmentHash) {
			std::ostringstream errorMsg;
			errorMsg << "Retrieved segment has wrong hash. Original: " << segmentHash <<
					" Retrieved: " << segmentDescription.getHash() << std::endl;
			errorMsg << "Retrieved segment left slice hashes: ";
			foreach (SliceHash hash, segmentDescription.getLeftSlices()) errorMsg << hash << " ";
			errorMsg << std::endl << "Retrieved segment right slice hashes: ";
			foreach (SliceHash hash, segmentDescription.getRightSlices()) errorMsg << hash << " ";

			LOG_ERROR(postgresqlsegmentstorelog) << errorMsg.str() << std::endl;
			UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg.str());
		}

		segmentDescriptions->add(segmentDescription);
	}

	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Retrieved " << segmentDescriptions->size() << " segments in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * segmentDescriptions->size()/queryElapsed.count()) << " segments/s)" << std::endl;

	return segmentDescriptions;
}

boost::shared_ptr<SegmentConstraints>
PostgreSqlSegmentStore::getConstraintsByBlocks(
		const Blocks& blocks) {

	boost::shared_ptr<SegmentConstraints> constraints = boost::make_shared<SegmentConstraints>();

	if (blocks.empty()) return constraints;

	boost::timer::cpu_timer queryTimer;

	const std::string blocksQuery = PostgreSqlUtils::createBlockIdQuery(
			blocks, _config.getCatmaidRawStackId());

	// Query constraints for this set of blocks
	std::string blockConstraintsQuery =
			"SELECT cst.id, cst.relation, cst.value, "
			"array_agg(DISTINCT ROW(csr.segment_id, csr.coefficient)) "
			"FROM djsopnet_constraint cst "
			"JOIN djsopnet_blockconstraintrelation bcr ON bcr.constraint_id = cst.id "
			"JOIN djsopnet_constraintsegmentrelation csr ON csr.constraint_id = cst.id "
			"WHERE bcr.block_id IN (" + blocksQuery + ") "
			"GROUP BY cst.id";

	enum { FIELD_ID_UNUSED, FIELD_RELATION, FIELD_VALUE, FIELD_SEGMENT_ARRAY };
	PGresult* queryResult = PQexec(_pgConnection, blockConstraintsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockConstraintsQuery);
	int nConstraints = PQntuples(queryResult);

	// Build constraint for each row
	for (int i = 0; i < nConstraints; ++i) {
		SegmentConstraint constraint;

		char* cellStr;
		cellStr = PQgetvalue(queryResult, i, FIELD_RELATION);
		Relation relation = cellStr[0] == 'L' ? LessEqual : (cellStr[0] == 'E' ? Equal : GreaterEqual);
		constraint.setRelation(relation);
		cellStr = PQgetvalue(queryResult, i, FIELD_VALUE);
		constraint.setValue(boost::lexical_cast<double>(cellStr));

		// Parse constraint->segment tuples for segment of form: {"(segment_id)",...}
		cellStr = PQgetvalue(queryResult, i, FIELD_SEGMENT_ARRAY);
		std::string segmentsString(cellStr);

		boost::char_separator<char> separator("{}()\", \t");
		boost::tokenizer<boost::char_separator<char> > tuples(segmentsString, separator);
		for (boost::tokenizer<boost::char_separator<char> >::iterator tuple = tuples.begin();
			tuple != tuples.end();
			++tuple) {
			SegmentHash segment = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(*tuple));
			double coeff = boost::lexical_cast<double>(*(++tuple));

			constraint.setCoefficient(segment, coeff);
		}

		constraints->push_back(constraint);
	}

	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Retrieved " << constraints->size() << " constraints in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * constraints->size()/queryElapsed.count()) << " constraints/s)" << std::endl;

	return constraints;
}

std::vector<double>
PostgreSqlSegmentStore::getFeatureWeights() {

	std::string query =
			"SELECT weights FROM djsopnet_featureinfo WHERE stack_id="
			+ boost::lexical_cast<std::string>(_config.getCatmaidRawStackId());
	PGresult* queryResult = PQexec(_pgConnection, query.c_str());
	PostgreSqlUtils::checkPostgreSqlError(queryResult, query);

	int nRows = PQntuples(queryResult);
	if (!nRows) {
		std::string errorMsg = "No feature weights found for stack.";
		LOG_ERROR(postgresqlsegmentstorelog) << errorMsg << std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg);
	}

	char* cellStr = PQgetvalue(queryResult, 0, 0);
	std::string weightsString(cellStr);
	weightsString = weightsString.substr(1, weightsString.length() - 2); // Remove { and }
	boost::char_separator<char> separator("{}()\", \t");
	boost::tokenizer<boost::char_separator<char> > weightsTokens(weightsString, separator);

	std::vector<double> weights;

	foreach (const std::string& weight, weightsTokens) {
		weights.push_back(boost::lexical_cast<double>(weight));
	}

	PQclear(queryResult);

	return weights;
}

void
PostgreSqlSegmentStore::storeSolution(
		const std::vector<SegmentHash>& segmentHashes,
		const Core& core) {

	boost::timer::cpu_timer queryTimer;

	const std::string coreQuery = PostgreSqlUtils::createCoreIdQuery(
			core, _config.getCatmaidRawStackId());

	std::ostringstream query;
	query << "DELETE FROM djsopnet_segmentsolution WHERE core_id = (" << coreQuery << ");"
			"WITH c AS (" << coreQuery << "), segments AS (VALUES ";
	char separator = ' ';

	foreach (const SegmentHash& segmentHash, segmentHashes) {
		query << separator << '('
			  << boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(segmentHash))
			  << ')';
		separator = ',';
	}

	query << ") INSERT INTO djsopnet_segmentsolution (core_id, segment_id) "
			"SELECT c.id, s.id FROM c, segments AS s (id);"
			"UPDATE djsopnet_core SET solution_set_flag = TRUE "
			"WHERE id = (" << coreQuery << ')';

	std::string queryString(query.str());
	PGresult* queryResult = PQexec(_pgConnection, queryString.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, queryString);
	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Stored " << segmentHashes.size() << " solutions in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * segmentHashes.size()/queryElapsed.count()) << " solutions/s)" << std::endl;
}

bool
PostgreSqlSegmentStore::getSegmentsFlag(const Block& block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				block, _config.getCatmaidRawStackId());
	std::string blockFlagQuery = "SELECT segments_flag FROM djsopnet_block "
			"WHERE id = (" + blockQuery + ")";
	PGresult* queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

	bool result = 0 == strcmp(PQgetvalue(queryResult, 0, 0), "t");

	PQclear(queryResult);

	return result;
}

#endif //HAVE_PostgreSQL
