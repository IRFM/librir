#include <mutex>
#include <algorithm>
#include <map>
#include <fstream>
#include <memory>

#include "IRVideoLoader.h"
#include "ReadFileChunk.h"

namespace rir
{

	static std::vector<IRVideoLoader *> _instances;
	static std::recursive_mutex _mutex;

	IRVideoLoader::IRVideoLoader()
		: m_globalEmi(1)
	{
		_mutex.lock();
		_instances.push_back(this);
		_mutex.unlock();
	}
	IRVideoLoader::~IRVideoLoader()
	{
		_mutex.lock();
		_instances.erase(std::find(_instances.begin(), _instances.end(), this));
		_mutex.unlock();
	}

	std::vector<IRVideoLoader *> IRVideoLoader::instances()
	{
		_mutex.lock();
		std::vector<IRVideoLoader *> res = _instances;
		_mutex.unlock();
		return res;
	}

	void IRVideoLoader::closeAll()
	{
		_mutex.lock();
		for (size_t i = 0; i < _instances.size(); ++i)
		{
			_instances[i]->close();
			/*IRVideoLoader * tmp = _instances[0];
			tmp->close();
			std::vector<IRVideoLoader*> ::iterator it = std::find(_instances.begin(), _instances.end(), tmp);
			if (it != _instances.end())
				_instances.erase(it);*/
		}
		_mutex.unlock();
	}

	StringList IRVideoLoader::tableNames() const
	{
		if (auto *c = calibration())
			return c->tableNames();
		return StringList();
	}
	std::vector<float> IRVideoLoader::getTable(const char *name) const
	{
		if (auto *c = calibration())
		{
			auto p = c->getTable(name);
			if (!p.first)
				return std::vector<float>();

			std::vector<float> res(p.second);
			std::copy(p.first, p.first + p.second, res.begin());
			return res;
		}
		return std::vector<float>();
	}

	static std::vector<std::shared_ptr<IRVideoLoaderBuilder>> _builders;

	void registerIRVideoLoaderBuilder(IRVideoLoaderBuilder *builder)
	{
		_builders.push_back(std::shared_ptr<IRVideoLoaderBuilder>(builder));
	}
	/**
	 * Returns the first found CalibrationBuilder object with given name
	 */
	const IRVideoLoaderBuilder *findIRVideoLoaderBuilder(const char *name)
	{
		for (size_t i = 0; i < _builders.size(); ++i)
			if (strcmp(_builders[i]->typeName(), name) == 0)
				return _builders[i].get();
		return NULL;
	}

	/**
	Build a calibration object from a filename (possibly NULL) and a IRVideoLoader object(possibly NULL).
	Internally scan all registered CalibrationBuilder and, based on the result of probe member, returns the first
	successfully built BaseCalibration object.
	*/
	IRVideoLoader *buildIRVideoLoader(const char *filename, void *file_reader)
	{
		std::vector<char> bytes;
		if (filename)
		{
			std::ifstream fin(filename, std::ios::binary);
			if (fin)
			{
				bytes.resize(1000);
				fin.read(bytes.data(), bytes.size());
				size_t read_size = fin.gcount();
				bytes.resize(read_size);
			}
		}
		if (bytes.empty() && file_reader)
		{
			bytes.resize(1000);
			seekFile(file_reader, 0, AVSEEK_SET);
			size_t read_size = readFile(file_reader, bytes.data(), bytes.size());
			seekFile(file_reader, 0, AVSEEK_SET);
			bytes.resize(read_size);
		}

		if (!filename && bytes.empty())
			return NULL;

		for (size_t i = 0; i < _builders.size(); ++i)
		{
			if (_builders[i]->probe(filename, bytes.data(), bytes.size()))
			{
				if (IRVideoLoader *res = _builders[i]->build(filename, file_reader))
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
