#include "Product.h"
#include "SolarObject.h"

ProductParameter::ProductParameter(float value)
	: mBaseValue(value)
{
}

float ProductParameter::getValue(const SolarObject& obj) const
{
	auto it = mOverrideValue.find(obj.getType());
	if(it == mOverrideValue.end())
		return mBaseValue;
	else
		return it->second;
}

void ProductParameter::setOverrideValue(SOType t, float value)
{
	mOverrideValue[t] = value;
}


Product::Product(const std::string& name, float consumption, float labourreq, float labcap, float productioncap)
	: mName(name)
{
	mParameters.insert({"consumption", ProductParameter(consumption)});
	mParameters.insert({"labourRequired", ProductParameter(labourreq)});
	mParameters.insert({"labourCap", ProductParameter(labcap)});
	mParameters.insert({"productionCap", ProductParameter(productioncap)});
}

const std::string& Product::getName() const
{
	return mName;
}

float Product::getConsumption(const SolarObject& obj) const
{
	return mParameters.at("consumption").getValue(obj);
}

float Product::getLabourRequired(const SolarObject& obj) const
{
	return mParameters.at("labourRequired").getValue(obj);
}

unsigned int Product::getLabourCap(const SolarObject& obj) const
{
	return mParameters.at("labourCap").getValue(obj);
}

float Product::getProductionCap(const SolarObject& obj) const
{
	return mParameters.at("productionCap").getValue(obj);
}

void Product::setOverrideValue(const std::string& name, SOType t, float value)
{
	mParameters.at(name).setOverrideValue(t, value);
}


ProductCatalog* ProductCatalog::getInstance()
{
	static ProductCatalog Instance;
	return &Instance;
}

ProductCatalog::ProductCatalog()
{
	mProducts.insert({"Fruit",        Product("Fruit",        0.1f, 0.3f, 0.0f, 0.0f)});
	mProducts.insert({"Luxury goods", Product("Luxury goods", 0.2f, 0.5f, 1000.0f, 100000.0f)});

	mProducts.at("Fruit").setOverrideValue("productionCap", SOType::RockyOxygen, 1000000.0f);

	for(auto it : mProducts) {
		mNames.push_back(it.first);
	}
}

float ProductCatalog::getConsumption(const std::string& prod, const SolarObject& obj) const
{
	return mProducts.at(prod).getConsumption(obj);
}

float ProductCatalog::getLabourRequired(const std::string& prod, const SolarObject& obj) const
{
	return mProducts.at(prod).getLabourRequired(obj);
}

float ProductCatalog::getLabourCap(const std::string& prod, const SolarObject& obj) const
{
	return mProducts.at(prod).getLabourCap(obj);
}

float ProductCatalog::getProductionCap(const std::string& prod, const SolarObject& obj) const
{
	return mProducts.at(prod).getProductionCap(obj);
}

const std::vector<std::string>& ProductCatalog::getNames() const
{
	return mNames;
}


