#ifndef SR3_SOLAROBJECT_H
#define SR3_SOLAROBJECT_H

#include <string>
#include <map>
#include <vector>

#include "common/Entity.h"

#include "Constants.h"

class Settlement;
class Market;
class Trader;

class SolarObject : public Common::Entity {
	public:
		SolarObject(const std::string& name, float size, float mass);
		~SolarObject();
		SolarObject(const SolarObject&) = delete;
		SolarObject(const SolarObject&&) = delete;
		SolarObject& operator=(const SolarObject&) & = delete;
		SolarObject& operator=(SolarObject&&) & = delete;
		SolarObject(const SolarObject* center, const std::string& name, SOType type,
				float size, float mass, float orbit, float speed, unsigned int marketlevel);
		bool canBeColonised() const;
		float getSize() const { return mSize; }
		float getMass() const { return mMass; }
		virtual void update(float time) override;
		SOType getType() const { return mObjectType; }
		Settlement* getSettlement() { return mSettlement; }
		const Settlement* getSettlement() const { return mSettlement; }
		const std::string& getName() const { return mName; }
		bool hasMarket() const { return mSettlement != nullptr; }
		bool hasSettlement() const { return mSettlement != nullptr; }
		float getSettlementHappiness() const;
		Market* getMarket();
		const Market* getMarket() const;
		const Trader& getTrader() const;
		bool updateSettlement();
		Settlement* getOrCreateSettlement();
		void colonise(SolarObject* target);

	private:
		std::string mName;
		float mSize;
		float mMass;
		float mOrbit = 0.0f;
		float mOrbitPosition = 0.0f;
		float mSpeed = 0.0f;
		const SolarObject* mCenter = nullptr;
		SOType mObjectType = SOType::GasGiant;
		Settlement* mSettlement = nullptr;
};


#endif

