#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <cfloat>

#include "common/SDL_utils.h"
#include "common/DriverFramework.h"
#include "common/FontConfig.h"
#include "common/Math.h"
#include "common/Entity.h"
#include "common/Vehicle.h"
#include "common/Steering.h"
#include "common/Clock.h"

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
		unsigned int capacityLeft() const { return mCapacityLeft; }
		const std::map<std::string, unsigned int>& getStorage() const { return mStorage; }

	private:
		unsigned int mMaxCapacity;
		unsigned int mCapacityLeft;
		std::map<std::string, unsigned int> mStorage;
};

Storage::Storage(unsigned int maxCapacity)
	: mMaxCapacity(maxCapacity),
	mCapacityLeft(maxCapacity)
{
	assert(mMaxCapacity < 1000000);
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
		mCapacityLeft += num;
		return num;
	} else {
		auto val = it->second;
		it->second = 0;
		mCapacityLeft += val;
		return val;
	}
}


class Trader {
	public:
		Trader(float money, unsigned int storage);
		float getMoney() const { return mMoney; }
		float removeMoney(float val);
		unsigned int buy(const std::string& product, unsigned int number, float price, Trader& buyer);
		unsigned int sell(const std::string& product, unsigned int number, float price, Trader& seller);
		unsigned int addToStorage(const std::string& product, unsigned int number);
		unsigned int storageLeft() const;
		const std::map<std::string, unsigned int>& getStorage() const { return mStorage.getStorage(); }

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

	unsigned int tobuy = std::max<unsigned int>(buyer.getMoney() / price, number);
	tobuy = std::max<unsigned int>(tobuy, mStorage.items(product));
	tobuy = std::max<unsigned int>(tobuy, buyer.storageLeft());
	if(tobuy) {
		float total_cost = tobuy * price;
		buyer.removeMoney(total_cost);
		assert(buyer.getMoney() >= 0.0f);
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

float Trader::removeMoney(float val)
{
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


class Market {
	public:
		Market(float money, unsigned int storage);
		float getPrice(const std::string& product) const;
		const std::map<std::string, unsigned int>& getStorage() const { return mTrader.getStorage(); }

	private:
		std::map<std::string, float> mPrices;
		Trader mTrader;
};

Market::Market(float money, unsigned int storage)
	: mTrader(money, storage)
{
	if(money) {
		mPrices["Gold"] = rand() % 100 + 50;
		mPrices["Minerals"] = rand() % 100 + 50;
		mPrices["Luxury goods"] = rand() % 100 + 50;
		mTrader.addToStorage("Gold", rand() % 30);
		mTrader.addToStorage("Minerals", rand() % 100);
		mTrader.addToStorage("Luxury goods", rand() % 50);
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


enum class SOType {
	Star,
	GasGiant,
	RockyNoAtmosphere,
	RockyOxygen,
	RockyNitrogen,
	RockyCarbonDioxide,
	RockyMethane
};

class SolarObject : public Entity {
	public:
		SolarObject(float size);
		SolarObject(const SolarObject* center, SOType type, float size, float orbit, float speed, unsigned int marketlevel);
		float getSize() const { return mSize; }
		virtual void update(float time) override;
		SOType getType() const { return mObjectType; }
		const Market* getMarket() const { return &mMarket; }

	private:
		float mSize;
		float mOrbit = 0.0f;
		float mOrbitPosition = 0.0f;
		float mSpeed = 0.0f;
		const SolarObject* mCenter = nullptr;
		SOType mObjectType = SOType::GasGiant;
		Market mMarket;
};

SolarObject::SolarObject(float size)
	: mSize(size * 20.0f),
	mObjectType(SOType::Star),
	mMarket(0, 0)
{
}

SolarObject::SolarObject(const SolarObject* center, SOType type, float size, float orbit, float speed, unsigned int marketlevel)
	: mSize(size),
	mOrbit(orbit * 50000.0f),
	mOrbitPosition(rand() % 100 * 0.01f),
	mSpeed(speed * 0.002f),
	mCenter(center),
	mObjectType(type),
	mMarket(marketlevel * 1000.0f, marketlevel * 10000)
{
	assert(marketlevel <= 10);
	update(0.0f);
}

void SolarObject::update(float time)
{
	mOrbitPosition += time * mSpeed;
	auto origo = mCenter ? mCenter->getPosition() : Vector3();
	mPosition.x = origo.x + mOrbit * sin(mOrbitPosition * PI * 2.0f);
	mPosition.y = origo.y + mOrbit * cos(mOrbitPosition * PI * 2.0f);
}

static const float SolarSystemSpeedCoefficient = 10.0f;
static const float PlanetSizeCoefficient = 500.0f;

class SpaceShipAI {
	public:
		SpaceShipAI();
		void control(SpaceShip* ss, float time);

	private:
		void handleLanding(SpaceShip* ss);
		const SolarObject* mTarget = nullptr;
		Countdown mLandedTimer;
};

class SpaceShip : public Vehicle {
	public:
		SpaceShip(bool players, const SolarSystem* s);
		bool isAlive() const { return mAlive; }
		void setAlive(bool b) { mAlive = b; }
		bool isPlayer() const { return mPlayers; }
		virtual void update(float time) override;
		const SolarSystem* getSystem() const { return mSystem; }
		bool canLand(const SolarObject& obj) const;
		bool landed() const;
		void land(const SolarObject* obj);
		void takeoff();
		const SolarObject* getLandObject() const { return mLandObject; }

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
		const SolarSystem* mSystem;
		Trader mTrader;
		const SolarObject* mLandObject = nullptr;
};

SpaceShip::SpaceShip(bool players, const SolarSystem* s)
	: Vehicle(1.0f, 10000000.0f, 10000000.0f, true),
	mPlayers(players),
	mSystem(s),
	mTrader(100.0f, 20)
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
		if(mSystem)
			th *= SolarSystemSpeedCoefficient;
		setAcceleration(Vector3(th * EnginePower * cos(rot),
					th * EnginePower * sin(rot), 0.0f));
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
	if(dist > std::max(0.5f, obj.getSize()) * PlanetSizeCoefficient + 100.0f ||
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

	private:
		std::vector<SolarObject*> mObjects;
};

SolarSystem::SolarSystem()
{
	auto star = new SolarObject(1.0f);
	mObjects.push_back(star);
	mObjects.push_back(new SolarObject(star, SOType::RockyNoAtmosphere, 0.5f, 0.4f, 3.0f, 0));
	mObjects.push_back(new SolarObject(star, SOType::RockyCarbonDioxide, 0.9f, 0.7f, 2.0f, 1));
	auto p1 = new SolarObject(star, SOType::RockyOxygen, 1.0f, 1.0f, 1.0f, 9);
	auto m1 = new SolarObject(p1, SOType::RockyNoAtmosphere, 0.4f, 0.1f, 3.0f, 3);
	mObjects.push_back(p1);
	mObjects.push_back(m1);
	mObjects.push_back(new SolarObject(star, SOType::RockyCarbonDioxide, 0.7f, 2.0f, 0.5f, 6));
	auto p2 = new SolarObject(star, SOType::GasGiant, 15.0f, 4.0f, 0.25f, 0);
	auto m2 = new SolarObject(p2, SOType::RockyNoAtmosphere, 0.2f, 0.3f, 3.0f, 1);
	auto m3 = new SolarObject(p2, SOType::RockyNoAtmosphere, 0.2f, 0.4f, 3.0f, 1);
	auto m4 = new SolarObject(p2, SOType::RockyNoAtmosphere, 0.2f, 0.5f, 3.0f, 0);
	auto m5 = new SolarObject(p2, SOType::RockyNoAtmosphere, 0.2f, 0.6f, 3.0f, 0);
	mObjects.push_back(p2);
	mObjects.push_back(m2);
	mObjects.push_back(m3);
	mObjects.push_back(m4);
	mObjects.push_back(m5);
	auto p3 = new SolarObject(star, SOType::GasGiant, 10.0f, 8.0f, 0.25f, 0);
	auto m6 = new SolarObject(p3, SOType::RockyNoAtmosphere, 0.2f, 0.3f, 2.0f, 0);
	auto m7 = new SolarObject(p3, SOType::RockyNoAtmosphere, 0.2f, 0.4f, 2.0f, 0);
	auto m8 = new SolarObject(p3, SOType::RockyNoAtmosphere, 0.2f, 0.5f, 2.0f, 1);
	auto m9 = new SolarObject(p3, SOType::RockyNoAtmosphere, 0.2f, 0.6f, 2.0f, 0);
	mObjects.push_back(p3);
	mObjects.push_back(m6);
	mObjects.push_back(m7);
	mObjects.push_back(m8);
	mObjects.push_back(m9);
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
					handleLanding(ss);
					mTarget = nullptr;
				}
			} else {
				const auto& objs = ss->getSystem()->getObjects();
				assert(objs.size() > 0);
				int index = rand() % objs.size();
				mTarget = objs[index];
			}
		}
	}
}

void SpaceShipAI::handleLanding(SpaceShip* ss)
{
	assert(mTarget);
}

class GameState {
	public:
		GameState();
		SpaceShip& getPlayerShip();
		const SpaceShip& getPlayerShip() const;
		const std::vector<SpaceShip>& getShips() const;
		std::vector<SpaceShip>& getShips();
		std::vector<LaserShot>& getShots();
		const SolarSystem& getSolarSystem() const { return mSystem; }
		bool isSolar() { return mSolar; }
		void update(float t);
		void endCombat();
		void shoot(SpaceShip& s);

	private:
		void spawnSolarShip();

		std::vector<SpaceShip> mCombatShips;
		std::vector<SpaceShip> mSolarShips;
		std::vector<LaserShot> mShots;
		bool mSolar = false;
		SolarSystem mSystem;
		SteadyTimer mSpawnSolarShipTimer;
};

GameState::GameState()
	: mSpawnSolarShipTimer(0.8f)
{
	// player
	mCombatShips.push_back(SpaceShip(true, nullptr));
	for(int i = 0; i < 3; i++) {
		mCombatShips.push_back(SpaceShip(false, nullptr));
		mCombatShips[i + 1].setPosition(Vector3(rand() % 100 - 50, rand() % 100 - 50, 0.0f));
	}
	SpaceShip ss(true, &mSystem);
	ss.setPosition(Vector3(20000.0f, 20000.0f, 0.0f));
	mSolarShips.push_back(ss);
	for(int i = 0; i < 5; i++)
		spawnSolarShip();
}

void GameState::update(float t)
{
	if(!mSolar) {
		for(auto& ps : mCombatShips) {
			for(auto it = mShots.begin(); it != mShots.end(); ) {
				if(it->testHit(&ps)) {
					if(ps.isAlive()) {
						it = mShots.erase(it);
						ps.setAlive(false);
					} else {
						++it;
					}
				} else {
					++it;
				}
			}
			ps.update(t);
		}

		for(auto& ls : mShots) {
			ls.update(t);
		}
	} else {
		mSystem.update(t);
		for(auto& ps : mSolarShips) {
			ps.update(t);
		}

		if(mSpawnSolarShipTimer.check(t)) {
			if(mSolarShips.size() < 20 && (rand() % 3) == 0) {
				spawnSolarShip();
			}
		}
	}
}

void GameState::endCombat()
{
	assert(!mSolar);
	mCombatShips.clear();
	mShots.clear();
	mSolar = true;
}

std::vector<LaserShot>& GameState::getShots()
{
	return mShots;
}

void GameState::shoot(SpaceShip& s)
{
	mShots.push_back(LaserShot(&s));
}

const SpaceShip& GameState::getPlayerShip() const
{
	if(!mSolar) {
		assert(mCombatShips.size() > 0);
		return mCombatShips[0];
	} else {
		assert(mSolarShips.size() > 0);
		return mSolarShips[0];
	}
}

SpaceShip& GameState::getPlayerShip()
{
	if(!mSolar) {
		assert(mCombatShips.size() > 0);
		return mCombatShips[0];
	} else {
		assert(mSolarShips.size() > 0);
		return mSolarShips[0];
	}
}

const std::vector<SpaceShip>& GameState::getShips() const
{
	return mSolar ? mSolarShips : mCombatShips;
}

std::vector<SpaceShip>& GameState::getShips()
{
	return mSolar ? mSolarShips : mCombatShips;
}

void GameState::spawnSolarShip()
{
	SpaceShip ss(false, &mSystem);
	const auto& objs = mSystem.getObjects();
	assert(objs.size() > 0);
	int index = rand() % objs.size();
	const auto& obj = objs[index];
	ss.setPosition(obj->getPosition());
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

	private:
		void drawMenu();
		void drawMarket();
		void drawSpace();
		void drawCutscene();
		bool handleSpaceKey(SDLKey key, bool down);
		bool checkCombat();

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
		SolarObject* mLandTarget = nullptr;
};

AppDriver::AppDriver()
	: Driver(1280, 720, "Star Rover 3"),
	mCamera(-300.0f, -300.0f),
	mCheckCombatTimer(0.5f)
{
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

void AppDriver::drawMarket()
{
	std::vector<std::string> text;
	const auto& ps = mGameState.getPlayerShip();
	assert(ps.landed());
	const auto* obj = ps.getLandObject();
	assert(obj);
	const auto* m = obj->getMarket();
	assert(m);
	const auto& stor = m->getStorage();
	char buf[256];
	snprintf(buf, 255, "%-20s %-10s %-10s", "Product", "Quantity", "Price");
	text.push_back(std::string(buf));
	for(const auto& st : stor) {
		snprintf(buf, 255, "%-20s %-10d %-10.2f", st.first.c_str(), st.second, m->getPrice(st.first));
		text.push_back(std::string(buf));
	}

	float i = getScreenHeight() * 0.9f;
	for(auto& t : text) {
		SDL_utils::drawText(mTextMap, mMonoFont, Vector3(0.0f, 0.0f, 0.0f), 1.0f,
				getScreenWidth(), getScreenHeight(),
				getScreenWidth() * 0.5f, i, FontConfig(t.c_str(), Color::White, 1.0f),
				true, true);
		i -= 100.0f;
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
		if(ps.landed())
			continue;
		glPushMatrix();
		if(ps.isAlive())
			glColor4ub(ps.Color.r, ps.Color.g, ps.Color.b, 255);
		else
			glColor4ub(50, 0, 0, 255);
		auto tr = ps.getPosition() * mZoom + trdiff;
		glTranslatef(tr.x, tr.y, 0.0f);
		glRotatef(Math::radiansToDegrees(ps.getXYRotation()), 0.0f, 0.0f, 1.0f);
		float sc = ps.Scale * pow(mZoom, 0.1f);
		glScalef(sc, sc, 1.0f);
		glBegin(GL_TRIANGLES);
		glVertex2f( 1.0f,  0.0f);
		glVertex2f(-1.0f, -0.7f);
		glVertex2f(-1.0f,  0.7f);
		glEnd();

		// thrusters
		glLineWidth(2.0f);
		glColor3f(0.5f, 0.5f, 1.0f);
		if(ps.Thrust) {
			glBegin(GL_LINES);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(-ps.Thrust * 2.0f, 0.0f);
			glEnd();
		}
		if(ps.SideThrust) {
			glBegin(GL_LINES);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(0.0f, -ps.SideThrust * 1.0f);
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
	auto& ps = mGameState.getPlayerShip();
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
				ps.takeoff();
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
	auto& ps = mGameState.getPlayerShip();
	switch(mState) {
		case AppDriverState::SpaceCombat:
		case AppDriverState::SolarSystem:
			return handleSpaceKey(key, true);

		case AppDriverState::Landed:
			switch(key) {
				case SDLK_SPACE:
				case SDLK_RETURN:
					mState = AppDriverState::SolarSystem;
					ps.takeoff();
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

bool AppDriver::handleSpaceKey(SDLKey key, bool down)
{
	float acc = 0.0f;
	float side = 0.0f;
	auto& ps = mGameState.getPlayerShip();

	switch(key) {
		case SDLK_w:
			if(down)
				acc = 1.0f;
			else
				acc = 0.0f;
			ps.Thrust = acc;
			break;

		case SDLK_s:
			if(down)
				acc = -1.0f;
			else
				acc = 0.0f;
			ps.Thrust = acc;
			break;

		case SDLK_a:
			if(down)
				side = 1.0f;
			else
				side = 0.0f;
			ps.SideThrust = side;
			break;

		case SDLK_d:
			if(down)
				side = -1.0f;
			else
				side = 0.0f;
			ps.SideThrust = side;
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
				ps.land(mLandTarget);
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

		auto& ps = mGameState.getPlayerShip();
		if(mState == AppDriverState::SpaceCombat) {
			if(mCheckCombatTimer.check(frameTime)) {
				checkCombat();
			}
		} else {
			mLandTarget = nullptr;
			auto mindist = FLT_MAX;
			for(const auto& so : mGameState.getSolarSystem().getObjects()) {
				auto dist = Entity::distanceBetween(ps, *so);
				if(dist < mindist) {
					mLandTarget = so;
					mindist = dist;
				}
			}

			if(!ps.canLand(*mLandTarget)) {
				mLandTarget = nullptr;
			}
		}

		mCamera.x = ps.getPosition().x;
		mCamera.y = ps.getPosition().y;
	}
	return false;
}

bool AppDriver::checkCombat()
{
	int numOpponents = 0;
	int numNearbyOpponents = 0;
	const auto& ps = mGameState.getPlayerShip();
	for(const auto& ss : mGameState.getShips()) {
		if(ss.isPlayer())
			continue;

		if(ss.isAlive()) {
			numOpponents++;
			if(Entity::distanceBetween(ps, ss) < 500.0f)
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

