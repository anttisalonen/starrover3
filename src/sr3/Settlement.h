#ifndef SR3_SETTLEMENT_H
#define SR3_SETTLEMENT_H

#include <string>
#include <map>
#include <vector>

class SolarObject;

#include "Constants.h"

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

class Trader {
	public:
		Trader(float money, unsigned int storage);
		float getMoney() const;
		void addMoney(float val);
		float removeMoney(float val);
		unsigned int buy(const std::string& product, unsigned int number, float price, Trader& buyer);
		unsigned int sell(const std::string& product, unsigned int number, float price, Trader& seller);
		unsigned int addToStorage(const std::string& product, unsigned int number);
		unsigned int removeFromStorage(const std::string& product, unsigned int number);
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

class Population {
	public:
		Population(unsigned int num, float money, const SolarObject* obj);
		bool update(Market& m);
		float getMoney() const;
		void addMoney(float val);
		void removeMoney(float m);
		unsigned int getNum() const;
		void removePop(unsigned int num);
		void addPop(unsigned int num);

	private:
		bool consume(Market& m);
		void work(Market& m);
		unsigned int calculateConsumption(float coeff) const;

		unsigned int mNum;
		Trader mTrader;
		const SolarObject* mSolarObject;
};

class Settlement;

class Producer {
	public:
		Producer(const std::string& prod, unsigned int money);
		void enhance(float money);
		float deenhance();
		unsigned int produce(Market& m, const Settlement& settlement);
		const std::string& getProduct() const { return mProduct; }
		unsigned int getLevel() const { return mLevel; }
		float getProductionPrice(const Market& m, const SolarObject& obj) const;

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
		// NOTE: do not expose non-const Trader to ensure all buy/sell goes through the market.
		const Trader& getTrader() const { return mMarket.getTrader(); }
		bool update();
		unsigned int getPopulation() const;
		Population* getPopulationObj() { return &mPopulation; }
		float getPopulationMoney() const;
		const std::map<std::string, Producer*>& getProducers() const { return mProducers; }
		float getHappiness() const { return mHappiness; }
		const SolarObject* getSolarObject() const { return mSolarObject; }

	private:
		void createNewProducers();

		Market mMarket;
		Population mPopulation;
		std::map<std::string, Producer*> mProducers;
		const SolarObject* mSolarObject;
		float mHappiness = 1.0f;
};

#endif


