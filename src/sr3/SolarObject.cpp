#include <cassert>

#include "SolarObject.h"
#include "Settlement.h"

SolarObject::SolarObject(const std::string& name, float size, float mass)
	: mName(name),
	mSize(size * 20.0f),
	mMass(mass * 20.0f),
	mObjectType(SOType::Star)
{
}

SolarObject::~SolarObject()
{
	if(mSettlement)
		delete mSettlement;
}

SolarObject::SolarObject(const SolarObject* center, const std::string& name, SOType type,
		float size, float mass, float orbit, float speed, unsigned int marketlevel)
	: mName(name),
	mSize(size),
	mMass(mass),
	mOrbit(orbit * 50000.0f),
	mOrbitPosition(rand() % 100 * 0.01f),
	mSpeed(speed * 0.002f),
	mCenter(center),
	mObjectType(type)
{
	if(marketlevel > 0) {
		mSettlement = new Settlement(marketlevel, this);
	}
	update(0.0f);
}

void SolarObject::update(float time)
{
	mOrbitPosition += time * mSpeed;
	auto origo = mCenter ? mCenter->getPosition() : Common::Vector3();
	mPosition.x = origo.x + mOrbit * sin(mOrbitPosition * PI * 2.0f);
	mPosition.y = origo.y + mOrbit * cos(mOrbitPosition * PI * 2.0f);
}

bool SolarObject::canBeColonised() const
{
	return mMass < 10.0f && mObjectType != SOType::Star && mObjectType != SOType::GasGiant;
}

bool SolarObject::updateSettlement()
{
	if(mSettlement) {
		return mSettlement->update();
	} else {
		return false;
	}
}

Settlement* SolarObject::getOrCreateSettlement()
{
	if(!mSettlement)
		mSettlement = new Settlement(0, this);
	return mSettlement;
}

void SolarObject::colonise(SolarObject* target)
{
	printf("Moving population from %s to %s.\n", getName().c_str(), target->getName().c_str());
	auto newSettlement = target->getOrCreateSettlement();
	auto havePop = mSettlement->getPopulation();
	assert(havePop >= Constants::MinPopulationForColonisation);
	auto popMigration = (unsigned int)(havePop * 0.2f);
	mSettlement->getPopulationObj()->removePop(popMigration);
	newSettlement->getPopulationObj()->addPop(popMigration);
	assert(mSettlement->getPopulationMoney() >= Constants::MinPopulationMoneyForColonisation);
	auto moneyMigration = mSettlement->getPopulationMoney() * 0.2f;
	mSettlement->getPopulationObj()->removeMoney(moneyMigration);
	newSettlement->getPopulationObj()->addMoney(moneyMigration);
}

Market* SolarObject::getMarket()
{
	assert(hasMarket());
	return mSettlement->getMarket();
}

const Market* SolarObject::getMarket() const
{
	assert(hasMarket());
	return mSettlement->getMarket();
}

float SolarObject::getSettlementHappiness() const
{
	assert(mSettlement);
	return mSettlement->getHappiness();
}

const Trader& SolarObject::getTrader() const
{
	assert(hasMarket());
	return mSettlement->getMarket()->getTrader();
}


