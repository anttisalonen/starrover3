#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <cfloat>

#include <boost/shared_ptr.hpp>

#include "common/SDL_utils.h"
#include "common/DriverFramework.h"
#include "common/FontConfig.h"
#include "common/Math.h"
#include "common/Entity.h"
#include "common/Vehicle.h"
#include "common/Steering.h"
#include "common/Clock.h"
#include "common/Random.h"

static const float SolarSystemSpeedCoefficient = 10.0f;
static const float PlanetSizeCoefficient = 500.0f;
static const float LabourRequiredCoefficient = 0.7f; // labour units required per unit
static const float FruitConsumptionCoefficient = 0.1f; // units consumed per person

using namespace Common;

class SpaceShip;
class SolarObject;
class SolarSystem;

class Storage {
	public:
		Storage(unsigned int maxCapacity);
		unsigned int items(const std::string& product) const;
		unsigned int add(const std::string& product, unsigned int num);
		unsigned int remove(const std::string& product, unsigned int num);
		unsigned int capacityLeft() const;
		const std::map<std::string, unsigned int>& getStorage() const { return mStorage; }
		void clearAll();
		void clearProduct(const std::string& product);
		unsigned int getMaxCapacity() const { return mMaxCapacity; }

	private:
		unsigned int mMaxCapacity;
		unsigned int mCapacityLeft;
		std::map<std::string, unsigned int> mStorage;
};

Storage::Storage(unsigned int maxCapacity)
	: mMaxCapacity(maxCapacity),
	mCapacityLeft(maxCapacity)
{
}

unsigned int Storage::items(const std::string& product) const
{
	auto it = mStorage.find(product);
	if(it == mStorage.end())
		return 0;
	return it->second;
}

unsigned int Storage::add(const std::string& product, unsigned int num)
{
	if(mMaxCapacity && mCapacityLeft < num) {
		num = mCapacityLeft;
	}
	mStorage[product] += num;
	if(mMaxCapacity)
		mCapacityLeft -= num;
	return num;
}

unsigned int Storage::remove(const std::string& product, unsigned int num)
{
	auto it = mStorage.find(product);
	if(it == mStorage.end())
		return 0;
	if(it->second >= num) {
		it->second -= num;
		if(mMaxCapacity)
			mCapacityLeft += num;
		return num;
	} else {
		auto val = it->second;
		it->second = 0;
		if(mMaxCapacity)
			mCapacityLeft += val;
		return val;
	}
}

unsigned int Storage::capacityLeft() const
{
	if(mMaxCapacity)
		return mCapacityLeft;
	else
		return UINT_MAX;
}

void Storage::clearAll()
{
	for(auto it : mStorage) {
		remove(it.first, it.second);
	}
	assert(mCapacityLeft == mMaxCapacity);
}

void Storage::clearProduct(const std::string& product)
{
	auto it = mStorage.find(product);
	auto num = it == mStorage.end() ? 0 : it->second;
	if(num)
		remove(product, num);
}

class Trader {
	public:
		Trader(float money, unsigned int storage);
		float getMoney() const;
		void addMoney(float val);
		float removeMoney(float val);
		unsigned int buy(const std::string& product, unsigned int number, float price, Trader& buyer);
		unsigned int sell(const std::string& product, unsigned int number, float price, Trader& seller);
		unsigned int addToStorage(const std::string& product, unsigned int number);
		unsigned int storageLeft() const;
		unsigned int getMaxCapacity() const { return mStorage.getMaxCapacity(); }
		const std::map<std::string, unsigned int>& getStorage() const { return mStorage.getStorage(); }
		unsigned int items(const std::string& product) const;
		void clearAll();
		void clearProduct(const std::string& product);

	private:
		float mMoney;
		Storage mStorage;
};

Trader::Trader(float money, unsigned int storage)
	: mMoney(money),
	mStorage(storage)
{
}

unsigned int Trader::buy(const std::string& product, unsigned int number, float price, Trader& buyer)
{
	assert(price >= 0.0f);

	unsigned int tobuy = number;
	long double tobuy_float = std::min<long double>(buyer.getMoney() / price, (long double)number);
	tobuy = static_cast<unsigned int>(tobuy_float);
	tobuy = std::min<unsigned int>(tobuy, mStorage.items(product));
	tobuy = std::min<unsigned int>(tobuy, buyer.storageLeft());
	if(tobuy) {
		float total_cost = tobuy * price;
		buyer.removeMoney(total_cost);
		assert(buyer.getMoney() >= 0.0f);
		if(mMoney >= 0.0f)
			mMoney += total_cost;
		auto nums = buyer.addToStorage(product, tobuy);
		assert(nums == tobuy);
		unsigned int num = mStorage.remove(product, tobuy);
		assert(num == tobuy);
		return tobuy;
	} else {
		return 0;
	}
}

unsigned int Trader::sell(const std::string& product, unsigned int number, float price, Trader& seller)
{
	return seller.buy(product, number, price, *this);
}

float Trader::getMoney() const
{
	if(mMoney < 0.0f)
		return FLT_MAX;
	else
		return mMoney;
}

void Trader::addMoney(float val)
{
	assert(val > 0.0f);
	assert(mMoney >= 0.0f);
	mMoney += val;
}

float Trader::removeMoney(float val)
{
	if(mMoney < 0.0f)
		return FLT_MAX;
	assert(mMoney >= val);
	mMoney -= val;
	return mMoney;
}

unsigned int Trader::addToStorage(const std::string& product, unsigned int number)
{
	return mStorage.add(product, number);
}

unsigned int Trader::storageLeft() const
{
	return mStorage.capacityLeft();
}

unsigned int Trader::items(const std::string& product) const
{
	return mStorage.items(product);
}

void Trader::clearAll()
{
	mStorage.clearAll();
}

void Trader::clearProduct(const std::string& product)
{
	mStorage.clearProduct(product);
}

class ProductDefinitions {
	public:
		ProductDefinitions();
		const std::vector<std::string>& getNames() const;

	private:
		std::vector<std::string> mNames;
};

ProductDefinitions::ProductDefinitions()
{
	mNames.push_back("Fruit");
	mNames.push_back("Minerals");
	mNames.push_back("Luxury goods");
	mNames.push_back("Labour");
}

const std::vector<std::string>& ProductDefinitions::getNames() const
{
	return mNames;
}

class Market {
	public:
		Market(float money);
		float getPrice(const std::string& product) const;
		unsigned int items(const std::string& product) const;
		float getMoney() const;
		void addMoney(float val);
		const std::map<std::string, unsigned int>& getStorage() const { return mTrader.getStorage(); }
		unsigned int buy(const std::string& product, unsigned int number, Trader& buyer);
		unsigned int sell(const std::string& product, unsigned int number, Trader& seller);
		const Trader& getTrader() const { return mTrader; }
		void updatePrices();
		unsigned int fixLabour();
		std::map<std::string, std::pair<int, int>> getLastRecord();

	private:
		std::map<std::string, float> mPrices;
		Trader mTrader;
		std::map<std::string, int> mSurplus;
		std::map<std::string, std::pair<int, int>> mRecord;
};

Market::Market(float money)
	: mTrader(-1.0f, 0)
{
	if(money) {
		int type = rand() % 3;
		std::string want;
		std::string produce;
		std::string dontneed;
		if(type == 0) {
			want = "Fruit";
			produce = "Minerals";
			dontneed = "Luxury goods";
		} else if(type == 1) {
			want = "Luxury goods";
			produce = "Fruit";
			dontneed = "Minerals";
		} else {
			want = "Minerals";
			produce = "Luxury goods";
			dontneed = "Fruit";
		}
		mPrices[want] = 1.0f + ((rand() % 100) * 0.01f);
		mPrices[produce] = 1.0f + ((rand() % 100) * 0.01f);
		mPrices[dontneed] = 1.0f + ((rand() % 100) * 0.01f);
		mTrader.addToStorage(want, rand() % 30);
		mTrader.addToStorage(produce, rand() % 100);
		mTrader.addToStorage(dontneed, rand() % 50);
		mPrices["Labour"] = 0.01f;
	}
}

float Market::getPrice(const std::string& product) const
{
	auto it = mPrices.find(product);
	if(it == mPrices.end())
		return -1.0f;
	else
		return it->second;
}

unsigned int Market::items(const std::string& product) const
{
	return mTrader.items(product);
}

float Market::getMoney() const
{
	return mTrader.getMoney();
}

void Market::addMoney(float val)
{
	mTrader.addMoney(val);
}

unsigned int Market::buy(const std::string& product, unsigned int number, Trader& buyer)
{
	auto i = mTrader.buy(product, number, getPrice(product), buyer);
	mSurplus[product] -= i;
	mRecord[product].first += i;
	return i;
}

unsigned int Market::sell(const std::string& product, unsigned int number, Trader& seller)
{
	auto i = mTrader.sell(product, number, getPrice(product), seller);
	mSurplus[product] += i;
	mRecord[product].second += i;
	return i;
}

unsigned int Market::fixLabour()
{
	auto unemployment = items("Labour");
	mTrader.clearProduct("Labour");
	return unemployment;
}

std::map<std::string, std::pair<int, int>> Market::getLastRecord()
{
	auto m = mRecord;
	mRecord.clear();
	return m;
}

void Market::updatePrices()
{
	for(const auto& it : mTrader.getStorage()) {
		auto prodname = it.first;
		auto trans_it = mSurplus.find(prodname);
		auto hadTrans = trans_it != mSurplus.end();
		if(!hadTrans)
			continue;
		auto surp = trans_it->second;
		assert(mPrices.find(prodname) != mPrices.end());
		if(surp > 0) {
			mPrices[prodname] = mPrices[prodname] * 0.95f;
		} else if(surp < 0 || (surp == 0 && it.second == 0)) {
			mPrices[prodname] = mPrices[prodname] * 1.05f;
		}
	}
	mSurplus.clear();
}


enum class SOType {
	Star,
	GasGiant,
	RockyNoAtmosphere,
	RockyOxygen,
	RockyNitrogen,
	RockyCarbonDioxide,
	RockyMethane
};

class Population {
	public:
		Population(unsigned int num, float money);
		void update(Market& m);
		float getMoney() const;
		void addMoney(float val);
		void removeMoney(float m);
		unsigned int getNum() const;

	private:
		void consume(Market& m);
		void work(Market& m);
		unsigned int mNum;
		Trader mTrader;
};

Population::Population(unsigned int num, float money)
	: mNum(num),
	mTrader(money * num, 0)
{
	assert(mNum <= 1000000); // cap at 1 million
}

void Population::update(Market& m)
{
	consume(m);
	work(m);
}

void Population::consume(Market& m)
{
	float totalFruitConsumption = mNum * FruitConsumptionCoefficient;
	float fruitRem = fmodf(totalFruitConsumption, 1.0f);
	unsigned int remFruitConsumption = fruitRem != 0.0f ? (Random::uniform() < fruitRem ? 1 : 0) : 0;
	unsigned int fruitConsumption = (unsigned int) totalFruitConsumption + remFruitConsumption;
	if(fruitConsumption) {
		unsigned int bought = m.buy("Fruit", fruitConsumption, mTrader);

		if(bought < fruitConsumption) {
			mNum = mNum * 0.999f;
#if 0
			if(mNum > 1000) {
				const char* reason = "unknown";
				if(mTrader.getMoney() < m.getPrice("Fruit"))
					reason = "no money";
				else if(m.items("Fruit") == 0)
					reason = "no fruit";
				printf("Famine! Need %5u fruit, could only buy %5u. Reason: %s\n",
						fruitConsumption, bought, reason);
			}
#endif
		} else {
			mNum = mNum * 1.001f;
		}
		mNum = std::min<unsigned int>(mNum, 1000000);
	}

	mTrader.clearAll();
}

void Population::work(Market& m)
{
	unsigned int labour = mNum * 0.1f;
	mTrader.addToStorage("Labour", labour);
	unsigned int num = m.sell("Labour", labour, mTrader);
	assert(num == labour);
}

float Population::getMoney() const
{
	return mTrader.getMoney();
}

void Population::addMoney(float val)
{
	mTrader.addMoney(val);
}

void Population::removeMoney(float m)
{
	mTrader.removeMoney(m);
}

unsigned int Population::getNum() const
{
	return mNum;
}


class Settlement;

class SolarObject : public Entity {
	public:
		SolarObject(const std::string& name, float size, float mass);
		SolarObject(const SolarObject* center, const std::string& name, SOType type,
				float size, float mass, float orbit, float speed, unsigned int marketlevel);
		float getSize() const { return mSize; }
		float getMass() const { return mMass; }
		virtual void update(float time) override;
		SOType getType() const { return mObjectType; }
		Settlement* getSettlement() { return mSettlement; }
		const Settlement* getSettlement() const { return mSettlement; }
		const std::string& getName() const { return mName; }
		bool hasMarket() const { return mSettlement != nullptr; }
		Market* getMarket();
		const Market* getMarket() const;
		const Trader& getTrader() const;
		void updateSettlement();

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

class Producer {
	public:
		Producer(const std::string& prod, unsigned int money);
		void enhance(float money);
		float deenhance();
		unsigned int produce(Market& m, const Settlement& settlement);
		const std::string& getProduct() const { return mProduct; }

	private:
		std::string mProduct;
		Trader mTrader;
		unsigned int mLevel = 1;
};

class Settlement {
	public:
		Settlement(unsigned int marketlevel, const SolarObject* obj);
		~Settlement();
		Settlement(const Settlement&) = delete;
		Settlement(const Settlement&&) = delete;
		Settlement& operator=(const Settlement&) & = delete;
		Settlement& operator=(Settlement&&) & = delete;
		Market* getMarket() { return &mMarket; }
		const Market* getMarket() const { return &mMarket; }
		const Trader& getTrader() const { return mMarket.getTrader(); }
		void update();
		unsigned int getPopulation() const;
		float getPopulationMoney() const;

	private:
		void createNewProducers();

		Market mMarket;
		Population mPopulation;
		std::map<std::string, Producer*> mProducers;
		const SolarObject* mSolarObject;
};

Producer::Producer(const std::string& prod, unsigned int money)
	: mProduct(prod),
	mTrader(money, 0)
{
}

void Producer::enhance(float money)
{
	mTrader.addMoney(money);
	mLevel++;
}

float Producer::deenhance()
{
	auto money = mTrader.getMoney();
	if(money > 1000.0f && mLevel > 1) {
		mTrader.removeMoney(1000.0f);
		mLevel--;
		return 1000.0f;
	} else {
		return 0.0f;
	}
}

unsigned int Producer::produce(Market& m, const Settlement& settlement)
{
	unsigned int wantToProduce = 1.2f * FruitConsumptionCoefficient * settlement.getPopulation();
	unsigned int requiredLabour = wantToProduce * LabourRequiredCoefficient;
	unsigned int numLabour = m.buy("Labour", requiredLabour, mTrader);
	assert(mLevel > 0);
	float wholeProd = numLabour / LabourRequiredCoefficient;
	wholeProd = wholeProd * (1.0f + (mLevel - 1) * 0.01f);
	float rem = fmodf(wholeProd, 1.0f);
	unsigned int remProd = rem != 0.0f ? (Random::uniform() < rem ? 1 : 0) : 0;
	unsigned int prod = (unsigned int) wholeProd + remProd;

	if(prod) {
		mTrader.addToStorage(mProduct, prod);
	}

	unsigned int num = 0;
	if(mTrader.items(mProduct)) {
		num = m.sell(mProduct, mTrader.items(mProduct), mTrader);
	}

#if 0
	if(num < wantToProduce) {
		const char* reason = "unknown";
		if(mTrader.getMoney() - num * m.getPrice("Fruit") < m.getPrice("Labour"))
			reason = "factory has no money for labour";
		else if(numLabour < requiredLabour)
			reason = "no labour available";
		printf("Production stop! Wanted to sell %5u %s, could only sell %5u. Bought %u/%u labour. Have %.2f money. Reason: %s\n",
				wantToProduce, mProduct.c_str(), num, numLabour, requiredLabour,
				mTrader.getMoney(), reason);
	}
#endif

	mTrader.clearProduct("Labour");
	return num;
}


Settlement::Settlement(unsigned int marketlevel, const SolarObject* obj)
	: mMarket(marketlevel * 1000000.0f),
	mPopulation(pow(5, marketlevel) + 200, marketlevel * 10),
	mSolarObject(obj)
{
	assert(marketlevel <= 8);
}

Settlement::~Settlement()
{
	for(auto it : mProducers)
		delete it.second;
}

void Settlement::update()
{
	createNewProducers();
	if(mPopulation.getNum() > 20) {
		mPopulation.update(mMarket);
		for(auto it : mProducers) {
			auto num = it.second->produce(mMarket, *this);
			if(num == 0) {
				auto money = it.second->deenhance();
				if(money > 0.0f)
					mPopulation.addMoney(money);
			}
		}

		auto unemployment = mMarket.fixLabour();
		if(unemployment) {
			auto labourPrice = mMarket.getPrice("Labour");
			mPopulation.removeMoney(unemployment * labourPrice);
		}
	}

	mMarket.updatePrices();
}

unsigned int Settlement::getPopulation() const
{
	return mPopulation.getNum();
}

float Settlement::getPopulationMoney() const
{
	return mPopulation.getMoney();
}


void Settlement::createNewProducers()
{
	if(mSolarObject->getType() == SOType::RockyOxygen &&
			mMarket.getPrice("Fruit") > LabourRequiredCoefficient * mMarket.getPrice("Labour")) {
		if(mPopulation.getMoney() > 1000.0f) {
			mPopulation.removeMoney(1000.0f);
			auto it = mProducers.find("fruit");
			if(it == mProducers.end()) {
				mProducers["fruit"] = new Producer("Fruit", 1000.0f);
			} else {
				it->second->enhance(1000.0f);
			}
		}
	}
}


SolarObject::SolarObject(const std::string& name, float size, float mass)
	: mName(name),
	mSize(size * 20.0f),
	mMass(mass * 20.0f),
	mObjectType(SOType::Star)
{
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
	auto origo = mCenter ? mCenter->getPosition() : Vector3();
	mPosition.x = origo.x + mOrbit * sin(mOrbitPosition * PI * 2.0f);
	mPosition.y = origo.y + mOrbit * cos(mOrbitPosition * PI * 2.0f);
}

void SolarObject::updateSettlement()
{
	if(mSettlement)
		mSettlement->update();
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

const Trader& SolarObject::getTrader() const
{
	assert(hasMarket());
	return mSettlement->getMarket()->getTrader();
}

class TradeRoute {
	public:
		TradeRoute(SolarObject* from, SolarObject* to, const std::string& product);
		SolarObject* getFrom() { return mFrom; }
		SolarObject* getTo() { return mTo; }
		const SolarObject* getFrom() const { return mFrom; }
		const SolarObject* getTo() const { return mTo; }
		const std::string& getProduct() const { return mProduct; }

	private:
		SolarObject* mFrom;
		SolarObject* mTo;
		std::string mProduct;
};

TradeRoute::TradeRoute(SolarObject* from, SolarObject* to, const std::string& product)
	: mFrom(from),
	mTo(to),
	mProduct(product)
{
}


class TradeNetwork {
	public:
		void addTradeRoute(SolarObject* from, SolarObject* to, const std::string& product);
		void clearTradeRoutes();
		std::vector<boost::shared_ptr<TradeRoute>>& getTradeRoutesFrom(const SolarObject* from);
		const std::vector<boost::shared_ptr<TradeRoute>>& getTradeRoutesFrom(const SolarObject* from) const;
		const std::map<const SolarObject*, std::vector<boost::shared_ptr<TradeRoute>>>& getTradeRoutes() const;

	private:
		std::map<const SolarObject*, std::vector<boost::shared_ptr<TradeRoute>>> mTradeRoutes;
};

void TradeNetwork::addTradeRoute(SolarObject* from, SolarObject* to, const std::string& product)
{
	mTradeRoutes[from].push_back(boost::shared_ptr<TradeRoute>(new TradeRoute(from, to, product)));
}

void TradeNetwork::clearTradeRoutes()
{
	mTradeRoutes.clear();
}

std::vector<boost::shared_ptr<TradeRoute>>& TradeNetwork::getTradeRoutesFrom(const SolarObject* from)
{
	static std::vector<boost::shared_ptr<TradeRoute>> empty;
	auto it = mTradeRoutes.find(from);
	return it == mTradeRoutes.end() ? empty : it->second;
}

const std::map<const SolarObject*, std::vector<boost::shared_ptr<TradeRoute>>>& TradeNetwork::getTradeRoutes() const
{
	return mTradeRoutes;
}

const std::vector<boost::shared_ptr<TradeRoute>>& TradeNetwork::getTradeRoutesFrom(const SolarObject* from) const
{
	static std::vector<boost::shared_ptr<TradeRoute>> empty;
	auto it = mTradeRoutes.find(from);
	return it == mTradeRoutes.end() ? empty : it->second;
}

class SolarSystem {
	public:
		SolarSystem();
		~SolarSystem();
		SolarSystem(const SolarSystem&) = delete;
		SolarSystem(const SolarSystem&&) = delete;
		SolarSystem& operator=(const SolarSystem&) & = delete;
		SolarSystem& operator=(SolarSystem&&) & = delete;

		const std::vector<SolarObject*>& getObjects() const;
		void update(float time);
		TradeNetwork& getTradeNetwork() { return mTradeNetwork; }
		const TradeNetwork& getTradeNetwork() const { return mTradeNetwork; }
		void updatePrices();

	private:
		void updateTradeNetwork();

		std::vector<SolarObject*> mObjects;
		TradeNetwork mTradeNetwork;
};


class SpaceShipAI {
	public:
		SpaceShipAI();
		void control(SpaceShip* ss, float time);

	private:
		void handleLanding(SpaceShip* ss);
		float getPotentialRevenue(const TradeRoute& tr) const;

		SolarObject* mTarget = nullptr;
		Countdown mLandedTimer;
		boost::shared_ptr<TradeRoute> mTradeRoute;
		SpaceShip* mSS = nullptr;
};

class SpaceShip : public Vehicle {
	public:
		SpaceShip(bool players, SolarSystem* s);
		bool isAlive() const { return mAlive; }
		void setAlive(bool b) { mAlive = b; }
		bool isPlayer() const { return mPlayers; }
		virtual void update(float time) override;
		SolarSystem* getSystem() { return mSystem; }
		const SolarSystem* getSystem() const { return mSystem; }
		bool canLand(const SolarObject& obj) const;
		bool landed() const;
		void land(const SolarObject* obj);
		void takeoff();
		const SolarObject* getLandObject() const { return mLandObject; }
		const SolarObject* getClosestObject(float* dist) const;
		unsigned int getID() const { return mID; }
		const Trader& getTrader() const { return mTrader; }
		Trader& getTrader() { return mTrader; }

		float Scale = 10.0f;
		float EnginePower = 1000.0f;
		float Thrust = 0.0f;
		float SidePower = 2.0f;
		float SideThrust = 0.0f;
		Color Color;

	private:
		bool mAlive = true;
		bool mPlayers;
		SpaceShipAI mAgent;
		SolarSystem* mSystem;
		Trader mTrader;
		const SolarObject* mLandObject = nullptr;
		unsigned int mID;
		static unsigned int NextID;
};

unsigned int SpaceShip::NextID = 0;

SpaceShip::SpaceShip(bool players, SolarSystem* s)
	: Vehicle(1.0f, 10000000.0f, 10000000.0f, true),
	mPlayers(players),
	mSystem(s),
	mTrader(100.0f, 20),
	mID(++NextID)
{
	Color = mPlayers ? Color::White : Color::Red;
}

SpaceShipAI::SpaceShipAI()
	: mLandedTimer(5.0f)
{
}

void SpaceShip::update(float time)
{
	if(isAlive() && !landed()) {
		auto rot = getXYRotation();
		auto th = Thrust;
		Vector3 accel;

		if(mSystem) {
			th *= SolarSystemSpeedCoefficient;
			for(const auto& obj : mSystem->getObjects()) {
				auto dist = Entity::distanceBetween(*this, *obj);
				if(dist) {
					auto f = Entity::vectorFromTo(*this, *obj) * (1e+6 * obj->getMass() / (dist * dist));
					assert(!isnan(f.x));
					accel += f;
				}
			}
		}

		accel += Vector3(th * EnginePower * cos(rot),
				th * EnginePower * sin(rot), 0.0f);

		setAcceleration(accel);
		setXYRotationalVelocity(SidePower * SideThrust);
	}
	if(!mPlayers) {
		mAgent.control(this, time);
	}

	if(!landed()) {
		Vehicle::update(time);
	} else {
		setPosition(mLandObject->getPosition());
	}
}

bool SpaceShip::canLand(const SolarObject& obj) const
{
	if(mLandObject)
		return false;

	// TODO: should actually check relative speed
	auto dist = Entity::distanceBetween(*this, obj);
	if(dist > std::max(0.5f, obj.getSize()) * PlanetSizeCoefficient + 500.0f ||
			getVelocity().length() > 10000.0f) {
		return false;
	} else {
		return true;
	}
}

bool SpaceShip::landed() const
{
	return mLandObject != nullptr;
}

void SpaceShip::land(const SolarObject* obj)
{
	assert(obj);
	assert(canLand(*obj));
	assert(!mLandObject);
	mLandObject = obj;
}

void SpaceShip::takeoff()
{
	assert(mLandObject);
	mLandObject = nullptr;
}

const SolarObject* SpaceShip::getClosestObject(float* dist) const
{
	const SolarObject* ret = nullptr;
	auto mindist = FLT_MAX;
	if(!mSystem)
		return ret;

	for(const auto& so : mSystem->getObjects()) {
		auto thisdist = Entity::distanceBetween(*this, *so);
		if(thisdist < mindist) {
			ret = so;
			mindist = thisdist;
		}
	}

	if(dist)
		*dist = mindist;

	return ret;
}

class LaserShot : public Entity {
	public:
		LaserShot(const SpaceShip* shooter);
		bool testHit(const SpaceShip* other);

	private:
		const SpaceShip* mShooter;
};

LaserShot::LaserShot(const SpaceShip* shooter)
	: mShooter(shooter)
{
	setVelocity(shooter->getVelocity());
	auto rot = shooter->getXYRotation();
	Vector3 dir(cos(rot), sin(rot), 0.0f);
	setXYRotation(rot);
	setVelocity(shooter->getVelocity() + dir * 1000.0f);
	setPosition(shooter->getPosition() + getVelocity().normalized() * shooter->Scale);
}

bool LaserShot::testHit(const SpaceShip* other)
{
	if(mShooter == other)
		return false;

	if(mPosition.distance(other->getPosition()) < other->Scale * 1.0f)
		return true;
	return false;
}


SolarSystem::SolarSystem()
{
	srand(21);
	auto star = new SolarObject("Sol", 1.0f, 1.0f);
	mObjects.push_back(star);
	mObjects.push_back(new SolarObject(star, "Mercury", SOType::RockyNoAtmosphere, 0.5f, 0.5f, 0.4f, 3.0f, 0));
	mObjects.push_back(new SolarObject(star, "Venus", SOType::RockyCarbonDioxide, 0.9f, 0.9f, 0.7f, 2.0f, 1));
	auto p1 = new SolarObject(star, "Earth", SOType::RockyOxygen, 1.0f, 1.0f, 1.0f, 1.0f, 8);
	auto m1 = new SolarObject(p1, "Moon", SOType::RockyNoAtmosphere, 0.4f, 0.2f, 0.1f, 3.0f, 3);
	mObjects.push_back(p1);
	mObjects.push_back(m1);
	mObjects.push_back(new SolarObject(star, "Mars", SOType::RockyCarbonDioxide, 0.7f, 0.7f, 2.0f, 0.5f, 6));
	auto p2 = new SolarObject(star, "Jupiter", SOType::GasGiant, 15.0f, 15.0f, 4.0f, 0.25f, 0);
	auto m2 = new SolarObject(p2, "Io", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.3f, 3.0f, 1);
	auto m3 = new SolarObject(p2, "Europa", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.4f, 3.0f, 1);
	auto m4 = new SolarObject(p2, "Ganymede", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.5f, 3.0f, 0);
	auto m5 = new SolarObject(p2, "Callisto", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.6f, 3.0f, 0);
	mObjects.push_back(p2);
	mObjects.push_back(m2);
	mObjects.push_back(m3);
	mObjects.push_back(m4);
	mObjects.push_back(m5);
	auto p3 = new SolarObject(star, "Saturn", SOType::GasGiant, 10.0f, 10.0f, 8.0f, 0.25f, 0);
	auto m6 = new SolarObject(p3, "Dione", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.3f, 2.0f, 0);
	auto m7 = new SolarObject(p3, "Rhea", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.4f, 2.0f, 0);
	auto m8 = new SolarObject(p3, "Titan", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.5f, 2.0f, 1);
	auto m9 = new SolarObject(p3, "Iapetus", SOType::RockyNoAtmosphere, 0.2f, 0.2f, 0.6f, 2.0f, 0);
	mObjects.push_back(p3);
	mObjects.push_back(m6);
	mObjects.push_back(m7);
	mObjects.push_back(m8);
	mObjects.push_back(m9);

	updateTradeNetwork();
}

void SolarSystem::updateTradeNetwork()
{
	mTradeNetwork.clearTradeRoutes();
	ProductDefinitions pd;
	const auto& products = pd.getNames();

	for(SolarObject* o : mObjects) {
		if(!o->hasMarket())
			continue;

		const auto& m1 = o->getMarket();
		const auto& stor = m1->getStorage();
		for(SolarObject* o2 : mObjects) {
			if(o == o2)
				continue;

			if(!o2->hasMarket())
				continue;

			const auto& m2 = o2->getMarket();
			for(const auto& prod : products) {
				auto it = stor.find(prod);
				if(it != stor.end() && it->second > 0 && m2->getPrice(prod) > 1.05f * m1->getPrice(prod)) {
					mTradeNetwork.addTradeRoute(o, o2, prod);
				}
			}
		}
	}
}

void SolarSystem::updatePrices()
{
	for(auto& obj : mObjects) {
		if(obj->hasMarket()) {
			obj->updateSettlement();
		}
	}
	updateTradeNetwork();
}

SolarSystem::~SolarSystem()
{
	for(auto& o : mObjects)
		delete o;
}

const std::vector<SolarObject*>& SolarSystem::getObjects() const
{
	return mObjects;
}

void SolarSystem::update(float time)
{
	for(auto& o : mObjects) {
		o->update(time);
	}
}

void SpaceShipAI::control(SpaceShip* ss, float time)
{
	if(!mSS)
		mSS = ss;
	else
		assert(mSS == ss);

	if(ss->getSystem()) {
		if(ss->landed()) {
			if(mLandedTimer.countdownAndRewind(time)) {
				ss->takeoff();
			}
		} else {
			if(mTarget) {
				auto desiredVelocity = mTarget->getPosition() - ss->getPosition();
				auto velDiff = desiredVelocity - ss->getVelocity() * 2.5f;
				velDiff = Math::rotate2D(velDiff, -ss->getXYRotation());
				auto velDiffNorm = velDiff / (ss->EnginePower * SolarSystemSpeedCoefficient);
				ss->SideThrust = clamp(-1.0f, velDiffNorm.y, 1.0f);
				ss->Thrust = clamp(-1.0f, velDiffNorm.x, 1.0f);

				if(ss->canLand(*mTarget)) {
					ss->land(mTarget);
					handleLanding(ss); // resets mTarget
				}
			} else {
				const auto& objs = ss->getSystem()->getObjects();
				assert(objs.size() > 0);
				int index = rand() % objs.size();
				if(index == 0 && objs.size() > 1)
					index++;
				mTarget = objs[index];
			}
		}
	}
}

float SpaceShipAI::getPotentialRevenue(const TradeRoute& tr) const
{
	assert(mSS);
	auto from = tr.getFrom();
	auto to = tr.getTo();
	auto m1 = from->getMarket();
	auto m2 = to->getMarket();
	const auto& prod = tr.getProduct();
	auto p1 = m1->getPrice(prod);
	auto p2 = m2->getPrice(prod);
	auto volume = std::max(mSS->getTrader().getMaxCapacity(), m1->items(prod));
	return volume * (p2 - p1);
}

void SpaceShipAI::handleLanding(SpaceShip* ss)
{
	assert(mTarget);
	assert(mTarget == ss->getLandObject());
	auto& trader = ss->getTrader();
	auto landobj = mTarget;

	mTarget = nullptr;
	mTradeRoute = nullptr;

	// always sell everything on arrival if possible
	if(landobj->hasMarket()) {
		for(auto it : trader.getStorage()) {
			landobj->getMarket()->sell(it.first, it.second, trader);
		}
	}

	// choose next trade
	{
		// prefer routes from current location, search all routes otherwise
		auto& tn = ss->getSystem()->getTradeNetwork();
		auto routes = tn.getTradeRoutesFrom(landobj);
		if(routes.size() == 0) {
			auto routemap = tn.getTradeRoutes();
			for(auto it : routemap)
				routes.insert(routes.end(), it.second.begin(), it.second.end());
		}

		if(routes.size() != 0) {
			std::sort(routes.begin(), routes.end(), [&] (const boost::shared_ptr<TradeRoute>& r1,
						const boost::shared_ptr<TradeRoute>& r2) -> bool {
					return getPotentialRevenue(*r1) < getPotentialRevenue(*r2); } );
			int index = routes.size() - 1;
			mTradeRoute = routes[index];
			mTarget = mTradeRoute->getFrom();

			// found route, now buy if already on target
			if(mTradeRoute && landobj == mTradeRoute->getFrom()) {
				auto prod = mTradeRoute->getProduct();
				assert(landobj->hasMarket());
				landobj->getMarket()->buy(prod, trader.storageLeft(), trader);
				mTarget = mTradeRoute->getTo();
			}
		} else {
			// no routes, wander aimlessly
			assert(0);
			const auto& objs = ss->getSystem()->getObjects();
			assert(objs.size() > 0);
			int index = rand() % objs.size();
			if(index == 0 && objs.size() > 1)
				index++;
			mTarget = objs[index];
			mTradeRoute = boost::shared_ptr<TradeRoute>();
		}
	}

}

class GameState {
	public:
		GameState();
		~GameState();
		GameState(const GameState&) = delete;
		GameState(const GameState&&) = delete;
		GameState& operator=(const GameState&) & = delete;
		GameState& operator=(GameState&&) & = delete;
		SpaceShip* getPlayerShip();
		const SpaceShip* getPlayerShip() const;
		const std::vector<SpaceShip*>& getShips() const;
		std::vector<SpaceShip*>& getShips();
		std::vector<LaserShot>& getShots();
		const SolarSystem& getSolarSystem() const { return mSystem; }
		bool isSolar() { return mSolar; }
		void update(float t);
		void endCombat();
		void shoot(SpaceShip* s);

	private:
		void spawnSolarShip();

		std::vector<SpaceShip*> mCombatShips;
		std::vector<SpaceShip*> mSolarShips;
		std::vector<LaserShot> mShots;
		bool mSolar = false;
		SolarSystem mSystem;
		SteadyTimer mSpawnSolarShipTimer;
		SteadyTimer mUpdatePricesTimer;
};

GameState::GameState()
	: mSpawnSolarShipTimer(0.8f),
	mUpdatePricesTimer(10.0f)
{
	// player
	mCombatShips.push_back(new SpaceShip(true, nullptr));
	for(int i = 0; i < 3; i++) {
		mCombatShips.push_back(new SpaceShip(false, nullptr));
		mCombatShips[i + 1]->setPosition(Vector3(rand() % 100 - 50, rand() % 100 - 50, 0.0f));
	}
	auto ss = new SpaceShip(true, &mSystem);
	ss->setPosition(Vector3(20000.0f, 20000.0f, 0.0f));
	mSolarShips.push_back(ss);
	for(int i = 0; i < 5; i++)
		spawnSolarShip();
}

GameState::~GameState()
{
	for(auto s : mCombatShips)
		delete s;
	for(auto s : mSolarShips)
		delete s;
}

void GameState::update(float t)
{
	if(!mSolar) {
		for(auto& ps : mCombatShips) {
			for(auto it = mShots.begin(); it != mShots.end(); ) {
				if(it->testHit(ps)) {
					if(ps->isAlive()) {
						it = mShots.erase(it);
						ps->setAlive(false);
					} else {
						++it;
					}
				} else {
					++it;
				}
			}
			ps->update(t);
		}

		for(auto& ls : mShots) {
			ls.update(t);
		}
	} else {
		mSystem.update(t);
		for(auto& ps : mSolarShips) {
			ps->update(t);
		}

		if(mSpawnSolarShipTimer.check(t)) {
			if(getSolarSystem().getTradeNetwork().getTradeRoutes().size() > 3 && (rand() % 3) == 0) {
				spawnSolarShip();
			}
		}

		if(mUpdatePricesTimer.check(t)) {
			mSystem.updatePrices();
		}
	}
}

void GameState::endCombat()
{
	assert(!mSolar);
	for(auto s : mCombatShips)
		delete s;
	mCombatShips.clear();
	mShots.clear();
	mSolar = true;
}

std::vector<LaserShot>& GameState::getShots()
{
	return mShots;
}

void GameState::shoot(SpaceShip* s)
{
	mShots.push_back(LaserShot(s));
}

const SpaceShip* GameState::getPlayerShip() const
{
	if(!mSolar) {
		assert(mCombatShips.size() > 0);
		return mCombatShips[0];
	} else {
		assert(mSolarShips.size() > 0);
		return mSolarShips[0];
	}
}

SpaceShip* GameState::getPlayerShip()
{
	if(!mSolar) {
		assert(mCombatShips.size() > 0);
		return mCombatShips[0];
	} else {
		assert(mSolarShips.size() > 0);
		return mSolarShips[0];
	}
}

const std::vector<SpaceShip*>& GameState::getShips() const
{
	return mSolar ? mSolarShips : mCombatShips;
}

std::vector<SpaceShip*>& GameState::getShips()
{
	return mSolar ? mSolarShips : mCombatShips;
}

void GameState::spawnSolarShip()
{
	auto ss = new SpaceShip(false, &mSystem);
	const auto& objs = mSystem.getObjects();
	assert(objs.size() > 0);
	int index = rand() % objs.size();
	const auto& obj = objs[index];
	ss->setPosition(obj->getPosition());
	mSolarShips.push_back(ss);
}


enum class AppDriverState {
	MainMenu,
	SpaceCombat,
	CombatWon,
	SolarSystem,
	Landed
};

enum class CutsceneText {
	AllEnemyShot,
	EvadedEnemy
};

class AppDriver : public Driver {
	public:
		AppDriver();

	protected:
		virtual bool init() override;
		virtual void drawFrame() override;
		virtual bool handleMousePress(float frameTime, Uint8 button) override;
		virtual bool handleKeyDown(float frameTime, SDLKey key) override;
		virtual bool handleKeyUp(float frameTime, SDLKey key) override;
		virtual bool prerenderUpdate(float frameTime) override;
		virtual void render() override;

	private:
		void drawMenu();
		void drawMarket();
		void drawSpace();
		void drawCutscene();
		bool handleSpaceKey(SDLKey key, bool down);
		bool checkCombat();
		void printInfo();
		std::string getPopulationString(const SolarObject& obj) const;

		TTF_Font* mFont;
		TTF_Font* mMonoFont;
		TextMap mTextMap;
		AppDriverState mState = AppDriverState::MainMenu;
		GameState mGameState;
		Vector2 mCamera;
		SteadyTimer mCheckCombatTimer;
		CutsceneText mText;
		float mZoomSpeed = 0.0f;
		float mZoom = 1.0f;
		const float MaxZoomLevel = 0.001f;
		const SolarObject* mLandTarget = nullptr;
		unsigned int mFrameSkip = 1;
		unsigned int mFramesSkipped = 0;
};

AppDriver::AppDriver()
	: Driver(1280, 720, "Star Rover 3"),
	mCamera(-300.0f, -300.0f),
	mCheckCombatTimer(0.5f)
{
	setFixedTime(60, false);
	mFont = TTF_OpenFont("share/DejaVuSans.ttf", 36);
	assert(mFont);
	mMonoFont = TTF_OpenFont("share/DejaVuSansMono.ttf", 36);
	assert(mMonoFont);
}

bool AppDriver::init()
{
	SDL_utils::setupOrthoScreen(getScreenWidth(), getScreenHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	return true;
}

std::string AppDriver::getPopulationString(const SolarObject& obj) const
{
	assert(obj.hasMarket());
	char buf[256];
	auto pop = obj.getSettlement()->getPopulation();
	snprintf(buf, 255, "%u", pop);
	return std::string(buf);
}

void AppDriver::drawMarket()
{
	std::vector<std::string> text;
	const auto* ps = mGameState.getPlayerShip();
	assert(ps->landed());
	const auto* obj = ps->getLandObject();
	assert(obj);
	if(obj->hasMarket()) {
		const auto* m = obj->getMarket();
		assert(m);
		const auto& stor = m->getStorage();
		char buf[256];
		snprintf(buf, 255, "%s", obj->getName().c_str());
		text.push_back(std::string(buf));

		auto popstr = getPopulationString(*obj);
		snprintf(buf, 255, "Population of %s with %.2f credits", 
				popstr.c_str(), obj->getSettlement()->getPopulationMoney());
		text.push_back(std::string(buf));
		snprintf(buf, 255, "%-20s %-10s %-10s", "Product", "Quantity", "Price");
		text.push_back(std::string(buf));
		for(const auto& st : stor) {
			snprintf(buf, 255, "%-20s %-10d %-10.2f", st.first.c_str(), st.second, m->getPrice(st.first));
			text.push_back(std::string(buf));
		}
		snprintf(buf, 255, "Ship storage %d      %.2f credits", ps->getTrader().storageLeft(), ps->getTrader().getMoney());
		text.push_back(std::string(buf));
	} else {
		text.push_back("No market. Press space to exit.");
	}

	float i = getScreenHeight() * 0.9f;
	for(auto& t : text) {
		SDL_utils::drawText(mTextMap, mMonoFont, Vector3(0.0f, 0.0f, 0.0f), 0.5f,
				getScreenWidth(), getScreenHeight(),
				getScreenWidth() * 0.5f, i, FontConfig(t.c_str(), Color::White, 1.0f),
				true, true);
		i -= 40.0f;
	}
}

void AppDriver::drawMenu()
{
	switch(mState) {
		case AppDriverState::MainMenu:
			{
				float fx = getScreenWidth() * 0.5f - 200;
				float fy = getScreenHeight() - 50;
				SDL_utils::drawRectangle(fx, fy,
						getScreenWidth() * 0.5f + 200, getScreenHeight() - 250, Color::White, 1.0f, true);
				SDL_utils::drawText(mTextMap, mFont, Vector3(0.0f, 0.0f, 0.0f), 1.0f,
						getScreenWidth(), getScreenHeight(),
						getScreenWidth() * 0.5f, getScreenHeight() - 150, FontConfig("Start game", Color::White, 1.0f),
						true, true);
			}
			break;

		case AppDriverState::Landed:
			drawMarket();
			break;

		default:
			assert(0);
			break;
	}
}

void AppDriver::drawSpace()
{
	auto width = getScreenWidth();
	auto height = getScreenHeight();
	glDisable(GL_TEXTURE_2D);
	Vector3 trdiff(width * 0.5f - mCamera.x * mZoom, height * 0.5f - mCamera.y * mZoom, 0.0f);

	for(const auto& ps : mGameState.getShips()) {
		if(ps->landed())
			continue;
		glPushMatrix();
		if(ps->isAlive())
			glColor4ub(ps->Color.r, ps->Color.g, ps->Color.b, 255);
		else
			glColor4ub(50, 0, 0, 255);
		auto tr = ps->getPosition() * mZoom + trdiff;
		glTranslatef(tr.x, tr.y, 0.0f);
		glRotatef(Math::radiansToDegrees(ps->getXYRotation()), 0.0f, 0.0f, 1.0f);
		float sc = ps->Scale * pow(mZoom, 0.1f);
		glScalef(sc, sc, 1.0f);
		glBegin(GL_TRIANGLES);
		glVertex2f( 1.0f,  0.0f);
		glVertex2f(-1.0f, -0.7f);
		glVertex2f(-1.0f,  0.7f);
		glEnd();

		// thrusters
		glLineWidth(2.0f);
		glColor3f(0.5f, 0.5f, 1.0f);
		if(ps->Thrust) {
			glBegin(GL_LINES);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(-ps->Thrust * 2.0f, 0.0f);
			glEnd();
		}
		if(ps->SideThrust) {
			glBegin(GL_LINES);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(0.0f, -ps->SideThrust * 1.0f);
			glEnd();
		}
		glLineWidth(1.0f);

		glPopMatrix();
	}

	glLineWidth(3.0f);
	for(const auto& ls : mGameState.getShots()) {
		glPushMatrix();
		glColor4ub(255, 0, 0, 255);
		auto tr = ls.getPosition() * mZoom + trdiff;
		glTranslatef(tr.x, tr.y, 0.0f);
		glRotatef(Math::radiansToDegrees(ls.getXYRotation()), 0.0f, 0.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex2f( 6.0f, 0.0f);
		glVertex2f(-6.0f, 0.0f);
		glEnd();
		glPopMatrix();
	}
	glLineWidth(1.0f);

	if(mGameState.isSolar()) {
		for(const auto& so : mGameState.getSolarSystem().getObjects()) {
			glPushMatrix();
			switch(so->getType()) {
				case SOType::Star:
					glColor4ub(255, 255, 0, 255);
					break;
				case SOType::GasGiant:
					glColor4ub(255, 128, 128, 255);
					break;
				case SOType::RockyNoAtmosphere:
					glColor4ub(60, 60, 60, 255);
					break;
				case SOType::RockyOxygen:
					glColor4ub(128, 128, 255, 255);
					break;
				case SOType::RockyNitrogen:
					glColor4ub(192, 192, 192, 255);
					break;
				case SOType::RockyCarbonDioxide:
					glColor4ub(255, 0, 0, 255);
					break;
				case SOType::RockyMethane:
					glColor4ub(128, 60, 60, 255);
					break;
			}
			auto tr = so->getPosition() * mZoom * 1.0f + trdiff;
			glTranslatef(tr.x, tr.y, 0.0f);
			float s = so->getSize();
			float points = clamp(16.0f, s * 8.0f, 128.0f);
			s = s * mZoom * PlanetSizeCoefficient;
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.0f,  0.0f);
			for(int i = 0; i < points + 1; i++) {
				glVertex2f(sin(2.0f * PI * i / points) * s, cos(2.0f * PI * i / points) * s);
			}
			glEnd();
			glPopMatrix();
		}

		if(mLandTarget) {
			SDL_utils::drawText(mTextMap, mFont, Vector3(0.0f, 0.0f, 0.0f), 1.0f,
					getScreenWidth(), getScreenHeight(),
					10, 40, FontConfig("Press Return to land", Color::White, 1.0f),
					true, false);
		}
	}
}

void AppDriver::drawCutscene()
{
	std::vector<std::string> text;
	switch(mText) {
		case CutsceneText::AllEnemyShot:
			text.push_back("You shot all enemy! Hooray!");
			break;

		case CutsceneText::EvadedEnemy:
			text.push_back("You managed to escape from the evil enemy.");
			text.push_back("You'll commence exploring the solar system now.");
			break;
	}

	float i = getScreenHeight() * 0.9f;
	for(auto& t : text) {
		SDL_utils::drawText(mTextMap, mFont, Vector3(0.0f, 0.0f, 0.0f), 1.0f,
				getScreenWidth(), getScreenHeight(),
				getScreenWidth() * 0.5f, i, FontConfig(t.c_str(), Color::White, 1.0f),
				true, true);
		i -= 100.0f;
	}
}

void AppDriver::drawFrame()
{
	switch(mState) {
		case AppDriverState::MainMenu:
		case AppDriverState::Landed:
			drawMenu();
			break;

		case AppDriverState::SpaceCombat:
		case AppDriverState::SolarSystem:
			drawSpace();
			break;

		case AppDriverState::CombatWon:
			drawCutscene();
			break;
	}
}

bool AppDriver::handleMousePress(float frameTime, Uint8 button)
{
	auto ps = mGameState.getPlayerShip();
	switch(mState) {
		case AppDriverState::MainMenu:
			if(button == SDL_BUTTON_LEFT) {
				mState = AppDriverState::SpaceCombat;
			}
			break;

		case AppDriverState::Landed:
			if(button == SDL_BUTTON_LEFT) {
				// TODO
				mState = AppDriverState::SolarSystem;
				ps->takeoff();
			}
			break;

		case AppDriverState::SpaceCombat:
		case AppDriverState::SolarSystem:
			break;

		case AppDriverState::CombatWon:
			if(button == SDL_BUTTON_LEFT) {
				mState = AppDriverState::SolarSystem;
			}
			break;
	}
	return false;
}

bool AppDriver::handleKeyDown(float frameTime, SDLKey key)
{
	auto ps = mGameState.getPlayerShip();
	switch(mState) {
		case AppDriverState::SpaceCombat:
		case AppDriverState::SolarSystem:
			return handleSpaceKey(key, true);

		case AppDriverState::Landed:
			switch(key) {
				case SDLK_SPACE:
				case SDLK_RETURN:
					mState = AppDriverState::SolarSystem;
					ps->takeoff();
					break;

				default:
					break;
			}
			break;

		case AppDriverState::MainMenu:
			switch(key) {
				case SDLK_ESCAPE:
					return true;

				case SDLK_SPACE:
				case SDLK_RETURN:
					mState = AppDriverState::SpaceCombat;
					break;

				default:
					break;
			}
			break;

		case AppDriverState::CombatWon:
			switch(key) {
				case SDLK_SPACE:
				case SDLK_RETURN:
					mState = AppDriverState::SolarSystem;
					break;

				default:
					break;
			}
			break;
	}

	return false;
}

bool AppDriver::handleKeyUp(float frameTime, SDLKey key)
{
	if(mState == AppDriverState::SpaceCombat || mState == AppDriverState::SolarSystem)
		return handleSpaceKey(key, false);

	return false;
}

void AppDriver::printInfo()
{
	for(auto ss : mGameState.getShips()) {
		auto t = ss->getTrader();
		printf("Spaceship %3u, %.2f money, %3d space.\n",
				ss->getID(), t.getMoney(), t.storageLeft());
		for(const auto& it : t.getStorage()) {
			if(it.second)
				printf("\t%-20s %-3u\n", it.first.c_str(), it.second);
		}
	}

	ProductDefinitions pd;
	const auto& products = pd.getNames();
	printf("%-20s %-16s %-16s ", "System", "Population", "Pop money");
	for(auto& p : products) {
		printf("%-16s ", p.c_str());
	}
	printf("\n");
	for(const auto& obj : mGameState.getSolarSystem().getObjects()) {
		if(!obj->hasMarket())
			continue;

		printf("%-20s %-16s %-16.2f ", obj->getName().c_str(),
				getPopulationString(*obj).c_str(),
				obj->getSettlement()->getPopulationMoney());
		const auto& m = obj->getMarket();
		for(auto& p : products) {
			auto i = m->getPrice(p);
			if(i >= 0.0f)
				printf("%-7.2f %-5d    ", m->getPrice(p), m->items(p));
			else
				printf("                 ");
		}
		printf("\n");
	}

	printf("%-10s %-10s %-10s %-10s %-10s\n", "Object", "Product", "Supply", "Demand", "Price");
	for(const auto& obj : mGameState.getSolarSystem().getObjects()) {
		if(obj->hasMarket()) {
			auto rec = obj->getMarket()->getLastRecord();
			for(auto prod : {"Fruit", "Labour"}) {
				auto it = rec.find(prod);
				if(it != rec.end()) {
					auto demand = it->second.first;
					auto supply = it->second.second;
					if(demand)
						printf("%-10s %-10s %-10u %-10u %-10.2f\n", obj->getName().c_str(),
								prod, supply, demand, obj->getMarket()->getPrice(prod));
				}
			}
		}
	}

	for(auto it : mGameState.getSolarSystem().getTradeNetwork().getTradeRoutes()) {
		for(auto it2 : it.second) {
			printf("Trade route from %-20s to %-20s for %-20s\n",
					it2->getFrom()->getName().c_str(),
					it2->getTo()->getName().c_str(),
					it2->getProduct().c_str());
		}
	}

}

bool AppDriver::handleSpaceKey(SDLKey key, bool down)
{
	float acc = 0.0f;
	float side = 0.0f;
	auto ps = mGameState.getPlayerShip();

	switch(key) {
		case SDLK_w:
			if(down)
				acc = 1.0f;
			else
				acc = 0.0f;
			ps->Thrust = acc;
			break;

		case SDLK_s:
			if(down)
				acc = -1.0f;
			else
				acc = 0.0f;
			ps->Thrust = acc;
			break;

		case SDLK_a:
			if(down)
				side = 1.0f;
			else
				side = 0.0f;
			ps->SideThrust = side;
			break;

		case SDLK_d:
			if(down)
				side = -1.0f;
			else
				side = 0.0f;
			ps->SideThrust = side;
			break;

		case SDLK_PLUS:
			if(mState == AppDriverState::SolarSystem) {
				if(down)
					mZoomSpeed = 1.0f;
				else
					mZoomSpeed = 0.0f;
			}
			break;

		case SDLK_MINUS:
			if(mState == AppDriverState::SolarSystem) {
				if(down)
					mZoomSpeed = -1.0f;
				else
					mZoomSpeed = 0.0f;
			}
			break;

		case SDLK_m:
			if(down && mState == AppDriverState::SolarSystem)
				mZoom = MaxZoomLevel;
			break;

		case SDLK_ESCAPE:
			return true;

		case SDLK_SPACE:
			if(down && mState == AppDriverState::SpaceCombat)
				mGameState.shoot(mGameState.getPlayerShip());
			break;

		case SDLK_RETURN:
			if(down && mLandTarget) {
				mState = AppDriverState::Landed;
				ps->land(mLandTarget);
			}
			break;

		case SDLK_F1:
			if(down)
				printInfo();
			break;

		case SDLK_F11:
			if(down && mFrameSkip > 1) {
				mFrameSkip /= 2;
				std::cout << "Frame skip: " << mFrameSkip << "\n";
				mFramesSkipped = mFrameSkip;
			}
			break;

		case SDLK_F12:
			if(down && mFrameSkip < 128) {
				mFrameSkip *= 2;
				std::cout << "Frame skip: " << mFrameSkip << "\n";
				mFramesSkipped = mFrameSkip;
			}
			break;

		default:
			break;
	}

	return false;
}

bool AppDriver::prerenderUpdate(float frameTime)
{
	if(mState == AppDriverState::SpaceCombat || mState == AppDriverState::SolarSystem) {
		mZoom = clamp(MaxZoomLevel, mZoom + 8.0f * mZoom * frameTime * mZoomSpeed, 100.0f);

		mGameState.update(frameTime);

		auto ps = mGameState.getPlayerShip();
		if(mState == AppDriverState::SpaceCombat) {
			if(mCheckCombatTimer.check(frameTime)) {
				checkCombat();
			}
		} else {
			mLandTarget = ps->getClosestObject(nullptr);

			if(mLandTarget && !ps->canLand(*mLandTarget)) {
				mLandTarget = nullptr;
			}
		}

		// ps might have changed due to end of combat
		ps = mGameState.getPlayerShip();
		mCamera.x = ps->getPosition().x;
		mCamera.y = ps->getPosition().y;
	}
	return false;
}

void AppDriver::render()
{
	if(mFrameSkip < 2) {
		Driver::render();
	} else {
		if(mFramesSkipped-- <= 0) {
			mFramesSkipped = mFrameSkip;
			Driver::render();
		}
	}
}

bool AppDriver::checkCombat()
{
	int numOpponents = 0;
	int numNearbyOpponents = 0;
	const auto& ps = mGameState.getPlayerShip();
	for(const auto& ss : mGameState.getShips()) {
		if(ss->isPlayer())
			continue;

		if(ss->isAlive()) {
			numOpponents++;
			if(Entity::distanceBetween(*ps, *ss) < 500.0f)
				numNearbyOpponents++;
		}
	}

	if(numNearbyOpponents == 0) {
		mState = AppDriverState::CombatWon;
		mGameState.endCombat();

		if(numOpponents == 0)
			mText = CutsceneText::AllEnemyShot;
		else
			mText = CutsceneText::EvadedEnemy;

		return true;
	}

	return false;
}

class App {
	public:
		void go();

	private:
		AppDriver mDriver;
};

void App::go()
{
	mDriver.run();
}

int main(void)
{
	App app;
	app.go();

	return 0;
}

