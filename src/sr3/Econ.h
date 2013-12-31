#ifndef SR3_ECON_H
#define SR3_ECON_H

#include <map>

#include "Constants.h"

class SolarObject;

namespace Econ {
	struct DataSet {
		unsigned int Production = 0;
		unsigned int Consumption = 0;
		unsigned int Import = 0;
		unsigned int Export = 0;
	};

	class Stats {
		public:
			static Stats* getInstance();
			void addEvent(Econ::Event event, const std::string& product,
					Econ::Entity ent, const SolarObject* obj, unsigned int num);
			void clearData();
			DataSet getData(const SolarObject* obj, const std::string& product) const;

		private:
			Stats() { }

			std::map<const SolarObject*, std::map<std::string, DataSet>> mData;
	};
}

#endif

