#include <cassert>
#include <cfloat>
#include <climits>

#include "Settlement.h"
#include "SolarObject.h"
#include "Product.h"

#include "common/Random.h"

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

Market::Market(float money)
	: mTrader(money, 0)
{
}

float Market::getPrice(const std::string& product) const
{
	auto it = mPrices.find(product);
	if(it == mPrices.end())
		return 1.0f;
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
	auto p = getPrice(product);
	auto i = mTrader.buy(product, number, p, buyer);
	if(i) {
		mSurplus[product] -= i;
		if(product == "Labour") {
			// pay labour credit
			mTrader.removeMoney(p * i);
		}
	}

	mRecord[product].first += i;
	return i;
}

unsigned int Market::sell(const std::string& product, unsigned int number, Trader& seller)
{
	auto p = getPrice(product);
	if(number && product == "Labour") {
		// loan money for labour since it will always even itself out
		mTrader.addMoney(p * number);
	}

	auto i = mTrader.sell(product, number, p, seller);

	if(i)
		mSurplus[product] += i;

	mRecord[product].second += i;
	return i;
}

unsigned int Market::fixLabour()
{
	// can simply remove items and consider the labour credit paid
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
	assert(mTrader.items("Labour") == 0);
	for(const auto& it : mTrader.getStorage()) {
		auto prodname = it.first;
		auto trans_it = mSurplus.find(prodname);
		auto hadTrans = trans_it != mSurplus.end();
		if(!hadTrans)
			continue;
		auto surp = trans_it->second;

		if(mPrices.find(prodname) == mPrices.end()) {
			mPrices[prodname] = 1.0f;
		}

		if(surp > 0) {
			mPrices[prodname] = mPrices[prodname] * 0.9f;
			if(mPrices[prodname] < 0.01f)
				mPrices[prodname] = 0.01f;
		} else if(mTrader.items(prodname) == 0) {
			mPrices[prodname] = mPrices[prodname] * 1.1f;
		}
	}
	mSurplus.clear();
}


Population::Population(unsigned int num, float money, const SolarObject* obj)
	: mNum(num),
	mTrader(money * num, 0),
	mSolarObject(obj)
{
	assert(mNum <= Constants::MaxPopulation); // cap at 1 million
}

bool Population::update(Market& m)
{
	auto famine = consume(m);
	work(m);
	return famine;
}

unsigned int Population::calculateConsumption(float coeff) const
{
	float totalConsumption = mNum * coeff;
	float rem = fmodf(totalConsumption, 1.0f);
	unsigned int remConsumption = rem != 0.0f ? (Common::Random::uniform() < rem ? 1 : 0) : 0;
	return (unsigned int) totalConsumption + remConsumption;
}

bool Population::consume(Market& m)
{
	bool famine = false;
	unsigned int fruitConsumption = calculateConsumption(ProductCatalog::getInstance()->getConsumption("Fruit", *mSolarObject));
	if(fruitConsumption) {
		unsigned int bought = m.buy("Fruit", fruitConsumption, mTrader);

		if(bought < fruitConsumption) {
			mNum = mNum * 0.999f;
			famine = true;
#if 1
			const char* reason = "unknown";
			if(mTrader.getMoney() < m.getPrice("Fruit"))
				reason = "no money";
			else if(m.items("Fruit") == 0)
				reason = "no fruit";
			printf("Famine! Need %5u fruit, could only buy %5u. Reason: %s\n",
					fruitConsumption, bought, reason);
#endif
		} else {
			mNum = mNum * 1.001f;
		}
		mNum = std::min<unsigned int>(mNum, Constants::MaxPopulation);
	}

	unsigned int luxuryConsumption = calculateConsumption(ProductCatalog::getInstance()->getConsumption("Luxury goods", *mSolarObject));
	if(luxuryConsumption) {
		m.buy("Luxury goods", luxuryConsumption, mTrader);
	}

	mTrader.clearAll();
	return famine;
}

void Population::work(Market& m)
{
	unsigned int labour = mNum * Constants::LabourProducedByCitizen;
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

void Population::removePop(unsigned int num)
{
	assert(mNum >= num);
	mNum -= num;
}

void Population::addPop(unsigned int num)
{
	assert(mNum + num <= Constants::MaxPopulation);
	mNum += num;
}

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
	float labourCoeff = ProductCatalog::getInstance()->getLabourRequired(mProduct, *settlement.getSolarObject());
	unsigned int labourCap = ProductCatalog::getInstance()->getLabourCap(mProduct, *settlement.getSolarObject());

	if(!labourCap)
		labourCap = UINT_MAX;

	if(m.getPrice(mProduct) < m.getPrice("Labour") * labourCoeff) {
		return 0;
	}

	unsigned int wantToProduce = UINT_MAX;
	unsigned int requiredLabour = std::min<unsigned int>(labourCap, wantToProduce * labourCoeff);
	unsigned int numLabour = m.buy("Labour", requiredLabour, mTrader);
	assert(mLevel > 0);
	float wholeProd = numLabour / labourCoeff;
	wholeProd = wholeProd * (1.0f + (mLevel - 1) * 0.01f);
	float rem = fmodf(wholeProd, 1.0f);
	unsigned int remProd = rem != 0.0f ? (Common::Random::uniform() < rem ? 1 : 0) : 0;
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
		if(mTrader.getMoney() - num * m.getPrice(mProduct) < m.getPrice("Labour"))
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
	mPopulation(pow(5, marketlevel) + 200, marketlevel * 1000, obj),
	mSolarObject(obj)
{
	assert(marketlevel <= 8);
}

Settlement::~Settlement()
{
	for(auto it : mProducers)
		delete it.second;
}

bool Settlement::update()
{
	bool foundNewSettlement = false;
	if(mPopulation.getNum() > 20) {
		if(mPopulation.getMoney() > 10000.0f && mMarket.getMoney() < 10000.0f) {
			// transfer money from population to market for more liquidity
			mPopulation.removeMoney(5000.0f);
			mMarket.addMoney(5000.0f);
		}

		auto famine = mPopulation.update(mMarket);
		for(auto it : mProducers) {
			auto num = it.second->produce(mMarket, *this);
			if(num == 0) {
				auto money = it.second->deenhance();
				if(money > 0.0f)
					mPopulation.addMoney(money);
			}
		}

		auto unemployment = mMarket.fixLabour();
		auto totalLabour = mPopulation.getNum() * Constants::LabourProducedByCitizen;
		auto unemploymentRate = unemployment / totalLabour;
		auto happiness = famine ? 0.0f : (1.0f - unemploymentRate);
		mHappiness = happiness * 0.2f + 0.8f * mHappiness;
		if(mPopulation.getNum() > Constants::MinPopulationForColonisation &&
				mPopulation.getMoney() > Constants::MinPopulationMoneyForColonisation) {
			if(Common::Random::uniform() < (1.0f - mHappiness)) {
				foundNewSettlement = true;
			}
		}
	} else {
		// TODO: abandon settlement
	}
	createNewProducers();

	mMarket.updatePrices();
	return foundNewSettlement;
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
	for(auto product : ProductCatalog::getInstance()->getNames()) {
		auto lim = ProductCatalog::getInstance()->getProductionCap(product, *mSolarObject);
		if(lim <= 0.0f)
			continue;

		auto labourCoeff = ProductCatalog::getInstance()->getLabourRequired(product, *mSolarObject);

		if(mMarket.getPrice(product) > labourCoeff * mMarket.getPrice("Labour")) {
			if(mPopulation.getMoney() > 1000.0f) {
				mPopulation.removeMoney(1000.0f);
				auto it = mProducers.find(product);
				if(it == mProducers.end()) {
					mProducers[product] = new Producer(product, 1000.0f);
				} else {
					it->second->enhance(1000.0f);
				}
			}
		}
	}
}


