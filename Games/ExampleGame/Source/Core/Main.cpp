#include "Precomp.h"
#include "Core/Engine.h"

#include "EditorSystems/LogWindowEditorSystem.h"

int main(int argc, char* args[])
{
	Engine::EngineClass engine{ argc, args };
	engine.Run();
	return 0;
}