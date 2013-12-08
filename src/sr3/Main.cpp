#include <unistd.h>

#include "common/SDL_utils.h"

using namespace Common;

class App {
	public:
		void go();

	private:
		SDL_Surface* mScreen;
};

void App::go()
{
	mScreen = SDL_utils::initSDL(1280, 720, "Star Rover 3");
	sleep(1);
}

int main(void)
{
	App app;
	app.go();

	return 0;
}

