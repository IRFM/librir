#include "BaseCalibration.h"
#include "IRVideoLoader.h"
#include <memory>

namespace rir
{
	static std::vector<std::shared_ptr<CalibrationBuilder>> _builders;

	void registerCalibrationBuilder(CalibrationBuilder *builder)
	{
		_builders.push_back(std::shared_ptr<CalibrationBuilder>(builder));
	}
	/**
	 * Returns the first found CalibrationBuilder object with given name
	 */
	const CalibrationBuilder *findCalibrationBuilder(const char *name)
	{
		for (size_t i = 0; i < _builders.size(); ++i)
			if (_builders[i]->name() == name)
				return _builders[i].get();
		return NULL;
	}

	/**
	Build a calibration object from a filename (possibly NULL) and a IRVideoLoader object(possibly NULL).
	Internally scan all registered CalibrationBuilder and, based on the result of probe member, returns the first
	successfully built BaseCalibration object.
	*/
	BaseCalibration *buildCalibration(const char *filename, IRVideoLoader *loader)
	{
		for (size_t i = 0; i < _builders.size(); ++i)
		{
			if (_builders[i]->probe(filename, loader))
			{
				if (BaseCalibration *res = _builders[i]->build(filename, loader))
				{
					if (res->isValid())
						return res;
					delete res;
				}
			}
		}
		return NULL;
	}
	BaseCalibration *buildCalibration(double pulse, const char *view, IRVideoLoader *loader)
	{
		for (size_t i = 0; i < _builders.size(); ++i)
		{
			if (_builders[i]->probe(pulse, view, loader))
			{
				if (BaseCalibration *res = _builders[i]->build(pulse, view, loader))
				{
					if (res->isValid())
						return res;
					delete res;
				}
			}
		}
		return NULL;
	}

}