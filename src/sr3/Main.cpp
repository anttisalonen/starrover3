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
		SpaceShip(bool players) : mPlayers(players) { }
		bool isAlive() const { return mAlive; }
		void setAlive(bool b) { mAlive = b; }
		bool isPlayer() const { return mPlayers; }

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

class GameState {
	public:
		GameState();
		SpaceShip& getPlayerShip();
		const SpaceShip& getPlayerShip() const;
		const std::vector<SpaceShip>& getShips() const;
		std::vector<SpaceShip>& getShips();
		std::vector<LaserShot>& getShots();
		void update(float t);
		void endCombat();
		void shoot(SpaceShip& s);

	private:
		std::vector<SpaceShip> mSpaceShips;
		std::vector<LaserShot> mShots;
};

GameState::GameState()
{
	// player
	mSpaceShips.push_back(SpaceShip(true));
	mSpaceShips[0].Color = Color::White;
	for(int i = 0; i < 3; i++) {
		mSpaceShips.push_back(SpaceShip(false));
		mSpaceShips[i + 1].Color = Color::Red;
		mSpaceShips[i + 1].setPosition(Vector3(rand() % 100 - 50, rand() % 100 - 50, 0.0f));
	}
}

void GameState::update(float t)
{
	for(auto& ps : mSpaceShips) {
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

		if(ps.isAlive()) {
			auto rot = ps.getXYRotation();
			ps.setAcceleration(Vector3(ps.Thrust * ps.EnginePower * sin(-rot),
						ps.Thrust * ps.EnginePower * cos(-rot), 0.0f));
			ps.setXYRotationalVelocity(ps.SidePower * ps.SideThrust);
		}
		ps.update(t);
	}

	for(auto& ls : mShots) {
		ls.update(t);
	}
}

void GameState::endCombat()
{
	for(auto it = mSpaceShips.begin(); it != mSpaceShips.end(); ) {
		if(it->isPlayer())
			++it;
		else
			it = mSpaceShips.erase(it);
	}
	mShots.clear();
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
	assert(mSpaceShips.size() > 0);
	return mSpaceShips[0];
}

SpaceShip& GameState::getPlayerShip()
{
	assert(mSpaceShips.size() > 0);
	return mSpaceShips[0];
}

const std::vector<SpaceShip>& GameState::getShips() const
{
	return mSpaceShips;
}

std::vector<SpaceShip>& GameState::getShips()
{
	return mSpaceShips;
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
	for(const auto& ps : mGameState.getShips()) {
		glPushMatrix();
		if(ps.isAlive())
			glColor4ub(ps.Color.r, ps.Color.g, ps.Color.b, 255);
		else
			glColor4ub(50, 0, 0, 255);
		glTranslatef(ps.getPosition().x - mCamera.x + width * 0.5f, ps.getPosition().y - mCamera.y + height * 0.5f, 0.0f);
		glRotatef(Math::radiansToDegrees(ps.getXYRotation()), 0.0f, 0.0f, 1.0f);
		glScalef(ps.Scale, ps.Scale, 1.0f);
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
		glTranslatef(ls.getPosition().x - mCamera.x + width * 0.5f, ls.getPosition().y - mCamera.y + height * 0.5f, 0.0f);
		glRotatef(Math::radiansToDegrees(ls.getXYRotation()), 0.0f, 0.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex2f( 0.0f,  6.0f);
		glVertex2f( 0.0f, -6.0f);
		glEnd();
		glPopMatrix();
	}
	glLineWidth(1.0f);
}

void AppDriver::drawCutscene()
{
	const char* text = nullptr;
	switch(mText) {
		case CutsceneText::AllEnemyShot:
			text = "You shot all enemy! Hooray!";
			break;

		case CutsceneText::EvadedEnemy:
			text = "You managed to escape from the evil enemy. You'll commence exploring the solar system now.";
			break;
	}

	SDL_utils::drawText(mTextMap, mFont, Vector3(0.0f, 0.0f, 0.0f), 1.0f,
			getScreenWidth(), getScreenHeight(),
			getScreenWidth() * 0.5f, getScreenHeight() * 0.9f, FontConfig(text, Color::White, 1.0f),
			true, true);
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
		mGameState.update(frameTime);
		auto& ps = mGameState.getPlayerShip();
		mCamera.x = ps.getPosition().x;
		mCamera.y = ps.getPosition().y;

		if(mState == AppDriverState::SpaceCombat) {
			if(mCheckCombatTimer.check(frameTime)) {
				if(checkCombat()) {
					mGameState.endCombat();
				}
			}
		}
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

