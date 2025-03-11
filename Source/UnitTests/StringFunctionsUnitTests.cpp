#include "Precomp.h"
#include "Utilities/StringFunctions.h"

#include "Core/UnitTests.h"

using namespace CE;

UNIT_TEST(StringFunctions, EqualStreams)
{
	std::istringstream streamA{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };
	std::istringstream streamB{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };

	TEST_ASSERT(StringFunctions::AreStreamsEqual(streamA, streamB));
}

UNIT_TEST(StringFunctions, NotEqualStreams)
{
	std::istringstream streamA{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };
	std::istringstream streamB{ "aOjkjij4lorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };

	TEST_ASSERT(!StringFunctions::AreStreamsEqual(streamA, streamB));
}

UNIT_TEST(StringFunctions, DifferentLengthStreams)
{
	std::istringstream streamA{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };
	std::istringstream streamB{ "aOjkjij4lorueLKJGi4308jv" };

	TEST_ASSERT(!StringFunctions::AreStreamsEqual(streamA, streamB));
}

UNIT_TEST(StringFunctions, LongStreams)
{
	std::istringstream streamA{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };
	std::istringstream streamB{ "FOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\nFOjoemkljorueLKJGi4308jvioJ-2\nOIJfd8\\\t\r\n" };

	TEST_ASSERT(StringFunctions::AreStreamsEqual(streamA, streamB));
}
