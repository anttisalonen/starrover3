#include <climits>

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


Product::Product(const std::string& name, float consumption, float labourreq, float productioncap)
	: mName(name)
{
	mParameters.insert({"consumption", ProductParameter(consumption)});
	mParameters.insert({"productionCap", ProductParameter(productioncap)});

	mGoodsRequired.insert({"Labour", ProductParameter(labourreq)});
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
	return getGoodRequired("Labour", obj);
}

float Product::getGoodRequired(const std::string& name, const SolarObject& obj) const
{
	return mGoodsRequired.at(name).getValue(obj);
}

float Product::getProductionCap(const SolarObject& obj) const
{
	return mParameters.at("productionCap").getValue(obj);
}

void Product::setOverrideValue(const std::string& name, SOType t, float value)
{
	mParameters.at(name).setOverrideValue(t, value);
}

void Product::setGoodRequirement(const std::string& name, float value)
{
	mGoodsRequired.insert({name, value});
}

std::vector<std::string> Product::getRequiredGoods(const SolarObject& obj) const
{
	std::vector<std::string> ret;
	for(auto it : mGoodsRequired)
		ret.push_back(it.first);
	return ret;
}

float Product::getRequiredGoodQuantity(const std::string& reqGood, const SolarObject& obj) const
{
	auto it = mGoodsRequired.find(reqGood);
	if(it == mGoodsRequired.end())
		return 0.0f;
	return it->second.getValue(obj);
}


ProductCatalog* ProductCatalog::getInstance()
{
	static ProductCatalog Instance;
	return &Instance;
}

ProductCatalog::ProductCatalog()
{
	//                                           name               demd  lab    prod cap
#if 0
	mProducts.insert({"Fruit",           Product("Fruit",           0.1f, 0.3f, 0.0f)});
	mProducts.insert({"Luxury goods",    Product("Luxury goods",    0.2f, 0.5f, 1000000.0f)});
	mProducts.insert({"Precious metals", Product("Precious metals", 0.0f, 0.7f, 0.0f)});
#else
	mProducts.insert({"Fruit",           Product("Fruit",           0.1f, 0.3f, 0.0f)});
	mProducts.insert({"Luxury goods",    Product("Luxury goods",    0.1f, 0.3f, 1000000.0f)});
	mProducts.insert({"Precious metals", Product("Precious metals", 0.0f, 0.3f, 0.0f)});
#endif

	mProducts.at("Fruit").setOverrideValue("productionCap", SOType::RockyOxygen, 1000000.0f);
	mProducts.at("Precious metals").setOverrideValue("productionCap", SOType::RockyNoAtmosphere, 1000000.0f);

	mProducts.at("Luxury goods").setGoodRequirement("Precious metals", 2.0f);

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

float ProductCatalog::getProductionCap(const std::string& prod, const SolarObject& obj) const
{
	return mProducts.at(prod).getProductionCap(obj);
}

const std::vector<std::string>& ProductCatalog::getNames() const
{
	return mNames;
}

std::map<std::string, float> ProductCatalog::getRequiredGoods(const std::string& prod, const SolarObject& obj) const
{
	std::map<std::string, float> ret;
	for(const auto& reqGood : mProducts.at(prod).getRequiredGoods(obj)) {
		ret[reqGood] = mProducts.at(prod).getRequiredGoodQuantity(reqGood, obj);
	}
	return ret;
}

