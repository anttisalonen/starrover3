#include <cassert>
#include <string>
#include <vector>

#include "common/SDL_utils.h"
#include "common/DriverFramework.h"
#include "common/FontConfig.h"
#include "common/Math.h"
#include "common/Entity.h"

using namespace Common;

class SpaceShip : public Entity {
	public:
		bool isAlive() const { return mAlive; }
		void setAlive(bool b) { mAlive = b; }

		float Scale = 10.0f;
		float EnginePower = 1000.0f;
		float Thrust = 0.0f;
		float SidePower = 2.0f;
		float SideThrust = 0.0f;
		Color Color;

	private:
		bool mAlive = true;
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
		std::vector<SpaceShip>& getShips();
		std::vector<LaserShot>& getShots();
		void update(float t);
		void shoot(SpaceShip& s);

	private:
		std::vector<SpaceShip> mSpaceShips;
		std::vector<LaserShot> mShots;
};

GameState::GameState()
{
	// player
	mSpaceShips.push_back(SpaceShip());
	mSpaceShips[0].Color = Color::White;
	for(int i = 0; i < 3; i++) {
		mSpaceShips.push_back(SpaceShip());
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

std::vector<LaserShot>& GameState::getShots()
{
	return mShots;
}

void GameState::shoot(SpaceShip& s)
{
	mShots.push_back(LaserShot(&s));
}

SpaceShip& GameState::getPlayerShip()
{
	assert(mSpaceShips.size() > 0);
	return mSpaceShips[0];
}

std::vector<SpaceShip>& GameState::getShips()
{
	return mSpaceShips;
}


enum class AppDriverState {
	MainMenu,
	Space
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
		bool handleSpaceKey(SDLKey key, bool down);

		TTF_Font* mFont;
		TextMap mTextMap;
		AppDriverState mState = AppDriverState::MainMenu;
		GameState mGameState;
		Vector2 mCamera;
};

AppDriver::AppDriver()
	: Driver(1280, 720, "Star Rover 3"),
	mCamera(-300.0f, -300.0f)
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
	glDisable(GL_TEXTURE_2D);
	for(const auto& ps : mGameState.getShips()) {
		glPushMatrix();
		if(ps.isAlive())
			glColor4ub(ps.Color.r, ps.Color.g, ps.Color.b, 255);
		else
			glColor4ub(50, 0, 0, 255);
		glTranslatef(ps.getPosition().x - mCamera.x, ps.getPosition().y - mCamera.y, 0.0f);
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
		glTranslatef(ls.getPosition().x - mCamera.x, ls.getPosition().y - mCamera.y, 0.0f);
		glRotatef(Math::radiansToDegrees(ls.getXYRotation()), 0.0f, 0.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex2f( 0.0f,  6.0f);
		glVertex2f( 0.0f, -6.0f);
		glEnd();
		glPopMatrix();
	}
	glLineWidth(1.0f);
}

void AppDriver::drawFrame()
{
	switch(mState) {
		case AppDriverState::MainMenu:
			drawMainMenu();
			break;

		case AppDriverState::Space:
			drawSpace();
			break;
	}
}

bool AppDriver::handleMousePress(float frameTime, Uint8 button)
{
	switch(mState) {
		case AppDriverState::MainMenu:
			if(button == SDL_BUTTON_LEFT) {
				mState = AppDriverState::Space;
			}
			break;

		case AppDriverState::Space:
			break;
	}
	return false;
}

bool AppDriver::handleKeyDown(float frameTime, SDLKey key)
{
	if(mState == AppDriverState::Space)
		return handleSpaceKey(key, true);

	else if(mState == AppDriverState::MainMenu && key == SDLK_ESCAPE)
		return true;

	return false;
}

bool AppDriver::handleKeyUp(float frameTime, SDLKey key)
{
	if(mState == AppDriverState::Space)
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
			if(down)
				mGameState.shoot(mGameState.getPlayerShip());

		default:
			break;
	}

	return false;
}

bool AppDriver::prerenderUpdate(float frameTime)
{
	if(mState == AppDriverState::Space) {
		mGameState.update(frameTime);
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

