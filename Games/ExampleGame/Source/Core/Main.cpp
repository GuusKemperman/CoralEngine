#include "Precomp.h"
#include "Core/Engine.h"

// For the UNIT_TEST_DECLARATION macro used in the Generated file
#include "Core/UnitTests.h"

// Forces initialization of otherwise unused static variables
#include "../Intermediate/Generated/Generated.h"

int main(int argc, char* argv[])
{
	CE::Engine engine{ argc, argv, GAME_DIR };
	engine.Run("KayLevel");
	return 0;
}