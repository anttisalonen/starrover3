#include <cassert>

#include "Econ.h"

#include "SolarObject.h"

namespace Econ {
	Stats* Stats::getInstance()
	{
		static Stats Instance;
		return &Instance;
	}

	void Stats::addEvent(Econ::Event event, const std::string& product,
			Econ::Entity ent, const SolarObject* obj, unsigned int num)
	{
		DataSet& ds = mData[obj][product];
		switch(event) {
			case Event::Buy:
				switch(ent) {
					case Entity::Population:
					case Entity::Industry:
						ds.Consumption += num;
						break;

					case Entity::Trader:
						ds.Export += num;
						break;

					case Entity::IndustryCancel:
						assert(0);
						break;
				}
				break;

			case Event::Sell:
				switch(ent) {
					case Entity::Population:
					case Entity::Industry:
						ds.Production += num;
						break;

					case Entity::IndustryCancel:
						assert(ds.Consumption >= num);
						ds.Consumption -= num;
						break;

					case Entity::Trader:
						ds.Import += num;
						break;

				}
				break;
		}
	}

	void Stats::clearData()
	{
		mData.clear();
	}

	DataSet Stats::getData(const SolarObject* obj, const std::string& product) const
	{
		auto it = mData.find(obj);
		if(it == mData.end())
			return DataSet();
		auto it2 = it->second.find(product);
		if(it2 == it->second.end())
			return DataSet();
		return it2->second;
	}

}

