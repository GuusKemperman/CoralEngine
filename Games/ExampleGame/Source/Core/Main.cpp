#include "Precomp.h"
#include "Core/Engine.h"

#include "Core/UnitTests.h"
#include "../Intermediate/Generated/Generated.h"

int main(int argc, char* argv[])
{
	// Currently ENGINE_ASSETS_DIR is a macro,
	// but this will break if the Engine/Assets folder or the
	// game executable are moved. When shipping, we should
	// have the Engine/Assets folder be passed into the executable
	// as a command argument.
	Engine::EngineClass engine{ argc, argv, ENGINE_ASSETS_DIR };
	engine.Run();
	return 0;
}