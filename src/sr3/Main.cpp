#include <cassert>
#include <string>
#include <vector>

#include "common/SDL_utils.h"
#include "common/DriverFramework.h"
#include "common/FontConfig.h"
#include "common/Math.h"
#include "common/Entity.h"
#include "common/Clock.h"

using namespace Common;

class SpaceShip : public Entity {
	public:
		SpaceShip(bool players);
		bool isAlive() const { return mAlive; }
		void setAlive(bool b) { mAlive = b; }
		bool isPlayer() const { return mPlayers; }
		virtual void update(float time) override;

		float Scale = 10.0f;
		float EnginePower = 1000.0f;
		float Thrust = 0.0f;
		float SidePower = 2.0f;
		float SideThrust = 0.0f;
		Color Color;

	private:
		bool mAlive = true;
		bool mPlayers;
};

SpaceShip::SpaceShip(bool players)
	: mPlayers(players)
{
	Color = mPlayers ? Color::White : Color::Red;
}

void SpaceShip::update(float time)
{
	if(isAlive()) {
		auto rot = getXYRotation();
		setAcceleration(Vector3(Thrust * EnginePower * sin(-rot),
					Thrust * EnginePower * cos(-rot), 0.0f));
		setXYRotationalVelocity(SidePower * SideThrust);
	}
	Entity::update(time);
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
	Vector3 dir(sin(-rot), cos(-rot), 0.0f);
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

class SolarObject : public Entity {
	public:
		SolarObject(float size);
		SolarObject(const SolarObject* center, float size, float orbit, float speed);
		float getSize() const { return mSize; }
		virtual void update(float time) override;

	private:
		float mSize;
		float mOrbit = 0.0f;
		float mOrbitPosition = 0.0f;
		float mSpeed = 0.0f;
		const SolarObject* mCenter = nullptr;
};

SolarObject::SolarObject(float size)
	: mSize(size * 20.0f)
{
}

SolarObject::SolarObject(const SolarObject* center, float size, float orbit, float speed)
	: mSize(size),
	mOrbit(orbit * 100000.0f),
	mOrbitPosition(rand() % 100 * 0.01f),
	mSpeed(speed * 0.01f),
	mCenter(center)
{
	update(0.0f);
}

void SolarObject::update(float time)
{
	mOrbitPosition += time * mSpeed;
	auto origo = mCenter ? mCenter->getPosition() : Vector3();
	mPosition.x = origo.x + mOrbit * sin(mOrbitPosition * PI * 2.0f);
	mPosition.y = origo.y + mOrbit * cos(mOrbitPosition * PI * 2.0f);
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
	mObjects.push_back(new SolarObject(star, 0.5f, 0.3f, 3.0f));
	mObjects.push_back(new SolarObject(star, 0.9f, 0.5f, 2.0f));
	auto p1 = new SolarObject(star, 1.0f, 1.0f, 1.0f);
	auto m1 = new SolarObject(p1, 0.4f, 0.1f, 3.0f);
	mObjects.push_back(p1);
	mObjects.push_back(m1);
	mObjects.push_back(new SolarObject(star, 0.7f, 2.0f, 0.5f));
	mObjects.push_back(new SolarObject(star, 15.0f, 4.0f, 0.25f));
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
		std::vector<SpaceShip> mCombatShips;
		std::vector<SpaceShip> mSolarShips;
		std::vector<LaserShot> mShots;
		bool mSolar = false;
		SolarSystem mSystem;
};

GameState::GameState()
{
	// player
	mCombatShips.push_back(SpaceShip(true));
	for(int i = 0; i < 3; i++) {
		mCombatShips.push_back(SpaceShip(false));
		mCombatShips[i + 1].setPosition(Vector3(rand() % 100 - 50, rand() % 100 - 50, 0.0f));
	}
	SpaceShip ss(true);
	ss.setPosition(Vector3(20000.0f, 20000.0f, 0.0f));
	mSolarShips.push_back(ss);
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


enum class AppDriverState {
	MainMenu,
	SpaceCombat,
	CombatWon,
	SolarSystem
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
		void drawMainMenu();
		void drawSpace();
		void drawCutscene();
		bool handleSpaceKey(SDLKey key, bool down);
		bool checkCombat();

		TTF_Font* mFont;
		TextMap mTextMap;
		AppDriverState mState = AppDriverState::MainMenu;
		GameState mGameState;
		Vector2 mCamera;
		SteadyTimer mCheckCombatTimer;
		CutsceneText mText;
		float mZoomSpeed = 0.0f;
		float mZoom = 1.0f;
		const float MaxZoomLevel = 0.001f;
};

AppDriver::AppDriver()
	: Driver(1280, 720, "Star Rover 3"),
	mCamera(-300.0f, -300.0f),
	mCheckCombatTimer(0.5f)
{
	mFont = TTF_OpenFont("share/DejaVuSans.ttf", 36);
	assert(mFont);
}

bool AppDriver::init()
{
	SDL_utils::setupOrthoScreen(getScreenWidth(), getScreenHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	return true;
}

void AppDriver::drawMainMenu()
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

void AppDriver::drawSpace()
{
	auto width = getScreenWidth();
	auto height = getScreenHeight();
	glDisable(GL_TEXTURE_2D);
	Vector3 trdiff(width * 0.5f - mCamera.x * mZoom, height * 0.5f - mCamera.y * mZoom, 0.0f);

	for(const auto& ps : mGameState.getShips()) {
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
		glVertex2f( 0.0f,  1.0f);
		glVertex2f(-0.7f, -1.0f);
		glVertex2f( 0.7f, -1.0f);
		glEnd();
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
		glVertex2f( 0.0f,  6.0f);
		glVertex2f( 0.0f, -6.0f);
		glEnd();
		glPopMatrix();
	}
	glLineWidth(1.0f);

	if(mGameState.isSolar()) {
		for(const auto& so : mGameState.getSolarSystem().getObjects()) {
			glPushMatrix();
			glColor4ub(255, 128, 128, 255);
			auto tr = so->getPosition() * mZoom * 1.0f + trdiff;
			glTranslatef(tr.x, tr.y, 0.0f);
			float s = so->getSize();
			float points = clamp(16.0f, s * 8.0f, 128.0f);
			s = s * mZoom * 500.0f;
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.0f,  0.0f);
			for(int i = 0; i < points + 1; i++) {
				glVertex2f(sin(2.0f * PI * i / points) * s, cos(2.0f * PI * i / points) * s);
			}
			glEnd();
			glPopMatrix();
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
			drawMainMenu();
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
	switch(mState) {
		case AppDriverState::MainMenu:
			if(button == SDL_BUTTON_LEFT) {
				mState = AppDriverState::SpaceCombat;
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
	switch(mState) {
		case AppDriverState::SpaceCombat:
		case AppDriverState::SolarSystem:
			return handleSpaceKey(key, true);

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

		default:
			break;
	}

	return false;
}

bool AppDriver::prerenderUpdate(float frameTime)
{
	if(mState == AppDriverState::SpaceCombat || mState == AppDriverState::SolarSystem) {
		mZoom = clamp(MaxZoomLevel, mZoom + 4.0f * mZoom * frameTime * mZoomSpeed, 100.0f);

		mGameState.update(frameTime);

		if(mState == AppDriverState::SpaceCombat) {
			if(mCheckCombatTimer.check(frameTime)) {
				checkCombat();
			}
		}

		auto& ps = mGameState.getPlayerShip();
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

