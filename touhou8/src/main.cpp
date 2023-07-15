#include "Game.h"

int main(int argc, char* argv[]) {
	th::Game game;

	game.Init();

	game.Run();

	game.Quit();

	return 0;
}
