#include "Precomp.h"
#include "Core/Engine.h"

#include "../Intermediate/Generated/AllReflectAtStartUp.h"

int main(int argc, char* args[])
{
	Engine::EngineClass engine{ argc, args };
	engine.Run();
	return 0;
}