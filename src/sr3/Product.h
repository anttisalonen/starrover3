#ifndef SR3_PRODUCT_H
#define SR3_PRODUCT_H

#include <string>
#include <map>
#include <vector>

#include "Constants.h"

class SolarObject;

class ProductParameter {
	public:
		ProductParameter(float value);
		float getValue(const SolarObject& obj) const;
		void setOverrideValue(SOType t, float value);

	private:
		float mBaseValue;
		std::map<SOType, float> mOverrideValue;
};

class Product {
	public:
		Product(const std::string& name, float consumption, float labourreq, float productioncap);

		// getters
		const std::string& getName() const;
		float getConsumption(const SolarObject& obj) const;
		float getLabourRequired(const SolarObject& obj) const;
		float getGoodRequired(const std::string& name, const SolarObject& obj) const;
		float getProductionCap(const SolarObject& obj) const;
		std::vector<std::string> getRequiredGoods(const SolarObject& obj) const;
		float getRequiredGoodQuantity(const std::string& reqGood, const SolarObject& obj) const;

		// setters
		void setOverrideValue(const std::string& name, SOType t, float value);
		void setGoodRequirement(const std::string& name, float value);

	private:
		std::map<std::string, ProductParameter> mParameters;
		std::map<std::string, ProductParameter> mGoodsRequired;
		std::string mName;
};

class ProductCatalog {
	public:
		const std::vector<std::string>& getNames() const;
		static ProductCatalog* getInstance();
		float getConsumption(const std::string& prod, const SolarObject& obj) const;
		float getLabourRequired(const std::string& prod, const SolarObject& obj) const;
		float getProductionCap(const std::string& prod, const SolarObject& obj) const;

		std::map<std::string, float> getRequiredGoods(const std::string& prod, const SolarObject& obj) const;

	private:
		ProductCatalog();

		std::vector<std::string> mNames;
		std::map<std::string, Product> mProducts;

};


#endif

