#include "Precomp.h"
#include "Core/Engine.h"

int main(int argc, char* argv[])
{
	CE::Engine engine{ argc, argv, GAME_DIR };
	engine.Run("TestLevel");

	return 0;
}