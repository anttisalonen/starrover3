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

#include "SolarObject.h"
#include "Settlement.h"
#include "Constants.h"
#include "Product.h"
#include "Econ.h"


using namespace Common;


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
		void updateSettlements();
		TradeNetwork& getTradeNetwork() { return mTradeNetwork; }
		const TradeNetwork& getTradeNetwork() const { return mTradeNetwork; }

	private:
		void updateTradeNetwork();
		void foundNewSettlement(SolarObject* from);

		std::vector<SolarObject*> mObjects;
		TradeNetwork mTradeNetwork;
};


class SpaceShip;

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
	mTrader(Constants::SpaceShipCargoSpace * 5.0f, Constants::SpaceShipCargoSpace),
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
			th *= Constants::SolarSystemSpeedCoefficient;
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
	// Make landing easier for the dumb AI
	auto dist = Entity::distanceBetween(*this, obj);
	auto maxDist = mPlayers ? std::max(0.5f, obj.getSize()) * Constants::PlanetSizeCoefficient + 500.0f :
		std::max(1.0f, obj.getSize()) * Constants::PlanetSizeCoefficient + 2500.0f;
	if(dist > maxDist) {
		return false;
	} else if(mPlayers && getVelocity().length() > 10000.0f) {
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
	const auto& products = ProductCatalog::getInstance()->getNames();

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
				if(it != stor.end() && it->second > 0 &&
						m2->getPrice(prod) > 1.5f * m1->getPrice(prod) &&
						m2->getMoney() > m2->getPrice(prod)) {
					mTradeNetwork.addTradeRoute(o, o2, prod);
				}
			}
		}
	}
}

void SolarSystem::updateSettlements()
{
	for(auto& obj : mObjects) {
		if(obj->hasMarket()) {
			bool newsett = obj->updateSettlement();
			if(newsett) {
				foundNewSettlement(obj);
			}
		}
	}
	updateTradeNetwork();
}

void SolarSystem::foundNewSettlement(SolarObject* from)
{
	SolarObject* target = nullptr;
	auto maxHappiness = 0.0f;

	// pick object with highest happiness if any
	for(auto& obj : mObjects) {
		if(obj == from)
			continue;

		if(!obj->hasSettlement())
			continue;

		auto hap = obj->getSettlementHappiness();
		if(hap > 0.4f && hap > maxHappiness) {
			target = obj;
			maxHappiness = hap;
		}
	}

	// if none found colonise a new object
	if(!target) {
		for(auto& obj : mObjects) {
			if(obj == from)
				continue;

			if(!obj->canBeColonised())
				continue;

			if(!obj->hasSettlement()) {
				target = obj;
				break;
			}
		}
	}

	if(target) {
		from->colonise(target);
	}
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
				auto velDiffNorm = velDiff / (ss->EnginePower * Constants::SolarSystemSpeedCoefficient);
				ss->SideThrust = clamp(-1.0f, velDiffNorm.y, 1.0f);
				ss->Thrust = clamp(-1.0f, velDiffNorm.x * 2.0f, 1.0f);

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
	return (p2 - p1);
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
			if(it.second) {
				landobj->getMarket()->sell(it.first, it.second, trader, Econ::Entity::Trader, landobj);
			}
		}
	}

	// choose next trade
	if(!mTradeRoute || landobj == mTradeRoute->getTo()) {
		// prefer routes from current location, search all routes otherwise
		auto& tn = ss->getSystem()->getTradeNetwork();
		auto routes = tn.getTradeRoutesFrom(landobj);
		if(routes.size() == 0) {
			const auto& routemap = tn.getTradeRoutes();
			for(const auto& it : routemap)
				routes.insert(routes.end(), it.second.begin(), it.second.end());
		}

		if(routes.size() != 0) {
			std::sort(routes.begin(), routes.end(), [this] (const boost::shared_ptr<TradeRoute>& r1,
						const boost::shared_ptr<TradeRoute>& r2) -> bool {
					return getPotentialRevenue(*r1) < getPotentialRevenue(*r2); } );
			unsigned int index = routes.size() - 1;
			if(routes.size() > 2)
				index = (routes.size() + 1) / 2 + rand() % (routes.size() / 2);
			assert(index < routes.size() && index >= routes.size() / 2);
			mTradeRoute = routes[index];
			mTarget = mTradeRoute->getFrom();
		} else {
			// no routes, wander aimlessly
			const auto& objs = ss->getSystem()->getObjects();
			assert(objs.size() > 0);
			int index = rand() % objs.size();
			if(index == 0 && objs.size() > 1)
				index++;
			mTarget = objs[index];
			mTradeRoute = boost::shared_ptr<TradeRoute>();
		}
	}

	// buy goods if planned
	if(mTradeRoute && landobj == mTradeRoute->getFrom()) {
		auto prod = mTradeRoute->getProduct();
		assert(landobj->hasMarket());
		landobj->getMarket()->buy(prod, trader.storageLeft(), trader, Econ::Entity::Trader, landobj);
		mTarget = mTradeRoute->getTo();

#if 0
		// if earned enough money, feed money back to the population of the exporting site
		if(trader.getMoney() > 1000.0f) {
			auto toDonate = trader.getMoney() - 1000.0f;
			trader.removeMoney(toDonate);
			landobj->getSettlement()->getPopulationObj()->addMoney(toDonate);
			printf("Donated %.2f to %s.\n", toDonate, landobj->getName().c_str());
		}
#endif
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
			if(getSolarSystem().getTradeNetwork().getTradeRoutes().size() * 20 < mSolarShips.size()) {
				spawnSolarShip();
			}
		}

		if(mUpdatePricesTimer.check(t)) {
			mSystem.updateSettlements();
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
			s = s * mZoom * Constants::PlanetSizeCoefficient;
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

	auto products = ProductCatalog::getInstance()->getNames();
	products.push_back("Labour");
	printf("%-12s %-16s %-16s %-16s %-12s ", "System", "Population", "Pop money", "Market money", "Happiness");
	for(auto& p : products) {
		printf("%-16s ", p.c_str());
	}
	printf("\n");

	unsigned long long totalPeople = 0;
	unsigned long long totalHappyPeople = 0;
	for(const auto& obj : mGameState.getSolarSystem().getObjects()) {
		if(!obj->hasMarket())
			continue;

		printf("%-12s %-16s %-16.2f %-16.2f %-12.2f ", obj->getName().c_str(),
				getPopulationString(*obj).c_str(),
				obj->getSettlement()->getPopulationMoney(), obj->getMarket()->getMoney(),
				obj->getSettlementHappiness());

		auto pop = obj->getSettlement()->getPopulation();
		totalPeople += pop;
		totalHappyPeople += pop * obj->getSettlementHappiness();

		const auto& m = obj->getMarket();
		for(auto& p : products) {
			auto items = m->items(p);
			auto price = m->getPrice(p);
			if(price > 10000) {
				printf("%-6.3fk ", price / 1000.0f);
			} else {
				printf("%-7.2f ", price);
			}
			if(items > 100000) {
				printf("%5dk   ", items / 1000);
			} else {
				printf("%5d    ", items);
			}
		}
		printf("\n");
	}

	if(totalPeople < 10000)
		printf("Total people: %llu\n", totalPeople);
	else if(totalPeople < 10000000)
		printf("Total people: %lluk\n", totalPeople / 1000);
	else
		printf("Total people: %lluM\n", totalPeople / 1000000);
	printf("Total happiness: %.2Lf %%\n", 100.0f * (totalHappyPeople / (long double)totalPeople));

	printf("%-16s %-16s %-16s %-16s %-16s %-16s\n", "Object", "Product", "Production", "Consumption", "Import", "Export");
	for(auto prod : products) {
		for(const auto& obj : mGameState.getSolarSystem().getObjects()) {
			if(obj->hasMarket()) {
				auto dt = Econ::Stats::getInstance()->getData(obj, prod);
				printf("%-16s %-16s %-16u %-16u %-16u %-16u\n", obj->getName().c_str(),
						prod.c_str(), dt.Production, dt.Consumption,
						dt.Import, dt.Export);
			}
		}
	}
	Econ::Stats::getInstance()->clearData();

	for(const auto& obj : mGameState.getSolarSystem().getObjects()) {
		if(obj->hasMarket()) {
			for(auto it : obj->getSettlement()->getProducers()) {
				printf("%-10s %-3u %s\n", obj->getName().c_str(), it.second->getLevel(), it.first.c_str());
			}
		}
	}

	{
		unsigned int numRoutes = 0;
		for(auto it : mGameState.getSolarSystem().getTradeNetwork().getTradeRoutes()) {
			numRoutes += it.second.size();
		}
		if(numRoutes <= 5) {
			for(auto it : mGameState.getSolarSystem().getTradeNetwork().getTradeRoutes()) {
				for(auto it2 : it.second) {
					printf("Trade route from %-20s to %-20s for %-20s\n",
							it2->getFrom()->getName().c_str(),
							it2->getTo()->getName().c_str(),
							it2->getProduct().c_str());
				}
			}
		} else {
			printf("%zd trade routes.\n", numRoutes);
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

