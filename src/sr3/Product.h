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
		Product(const std::string& name, float consumption, float labourreq, float labcap, float productioncap);
		const std::string& getName() const;
		float getConsumption(const SolarObject& obj) const;
		float getLabourRequired(const SolarObject& obj) const;
		unsigned int getLabourCap(const SolarObject& obj) const;
		float getProductionCap(const SolarObject& obj) const;

		void setOverrideValue(const std::string& name, SOType t, float value);

	private:
		std::map<std::string, ProductParameter> mParameters;
		std::string mName;
};

class ProductCatalog {
	public:
		const std::vector<std::string>& getNames() const;
		static ProductCatalog* getInstance();
		float getConsumption(const std::string& prod, const SolarObject& obj) const;
		float getLabourRequired(const std::string& prod, const SolarObject& obj) const;
		float getLabourCap(const std::string& prod, const SolarObject& obj) const;
		float getProductionCap(const std::string& prod, const SolarObject& obj) const;

	private:
		ProductCatalog();

		std::vector<std::string> mNames;
		std::map<std::string, Product> mProducts;

};


#endif

