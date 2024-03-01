#include "Precomp.h"
#include "Core/Engine.h"

#include "Core/UnitTests.h"
#include "../Intermediate/Generated/Generated.h"

int main(int argc, char* args[])
{
	Engine::EngineClass engine{ argc, args };
	engine.Run();
	return 0;
}