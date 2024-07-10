#include "Precomp.h"
#include "Core/UnitTests.h"

#include "Meta/MetaFunc.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Assets/Material.h"
#include "Assets/StaticMesh.h"

using namespace CE;

template<typename... Args>
static FuncResult CallScriptFuncChecked(const Name scriptName, const Name funcName, Args&&... args)
{
	const MetaType* const type = MetaManager::Get().TryGetType(scriptName);

	if (type == nullptr)
	{
		return Format("No script with the name {}", scriptName.StringView());
	}

	const MetaFunc* const func = type->TryGetFunc(funcName);

	if (func == nullptr)
	{
		return Format("Script {} has no func with the name {}", scriptName.StringView(), funcName.StringView());
	}

	return (*func)(std::forward<Args>(args)...);
}

static UnitTest::Result RunSimpleLoopTest(const Name funcName, std::function<int32(int32)> expectedNumOfIterations = [](int32 n) { return n; })
{
	const MetaType* const type = MetaManager::Get().TryGetType("UnitTestScript"_Name);

	if (type == nullptr)
	{
		LOG(LogUnitTest, Error, "Could not run test, the script we use for testing no longer exists");
		return UnitTest::Failure;
	}

	for (int32 i = 0; i < 10; i++)
	{
		FuncResult instance = type->Construct();

		if (instance.HasError())
		{
			LOG(LogUnitTest, Error, "Scripts do not produce a default constructible type - {}", instance.Error());
			return UnitTest::Failure;
		}

		FuncResult result = CallScriptFuncChecked("UnitTestScript"_Name, funcName, instance.GetReturnValue(), i);

		if (result.HasError())
		{
			LOG(LogUnitTest, Error, "Expected {} iterations, but the function returned an error - {}", i, result.Error());
			return UnitTest::Failure;
		}

		if (!result.HasReturnValue()
			|| result.GetReturnValue().As<int32>() == nullptr)
		{
			LOG(LogUnitTest, Error, "Function did not return an integer");
			return UnitTest::Failure;
		}

		int32 expected = expectedNumOfIterations(i);

		if (*result.GetReturnValue().As<int32>() != expected)
		{
			LOG(LogUnitTest, Error, "Loopbody SHOULD have ran {} times with i = {}, but ran {} times instead!", expected, i, *result.GetReturnValue().As<int32>());
			return UnitTest::Failure;
		}
	}

	return UnitTest::Success;
}

UNIT_TEST(Scripting, ParamsAndReturn)
{
	for (int32 i = 0; i < 10; i++)
	{
		FuncResult isEvenResult = CallScriptFuncChecked("UnitTestScript"_Name, "IsEven"_Name, i);
		FuncResult isOddResult = CallScriptFuncChecked("UnitTestScript"_Name, "IsOdd"_Name, i);

		if (isEvenResult.HasError())
		{
			LOG(LogUnitTest, Error, "{}", isEvenResult.Error());
			return UnitTest::Failure;
		}
		else if (isOddResult.HasError())
		{
			LOG(LogUnitTest, Error, "{}", isOddResult.Error());
			return UnitTest::Failure;
		}

		if (!isEvenResult.HasReturnValue()
			|| !isOddResult.HasReturnValue())
		{
			LOG(LogUnitTest, Error, "Expected a return value!");
			return UnitTest::Failure;
		}

		if (!isEvenResult.GetReturnValue().IsExactly<bool>()
			|| !isOddResult.GetReturnValue().IsExactly<bool>())
		{
			LOG(LogUnitTest, Error, "Expected booleans!");
			return UnitTest::Failure;
		}
	}

	return UnitTest::Success;
}

UNIT_TEST(Scripting, NonStaticFunctions)
{
	const MetaType* const type = MetaManager::Get().TryGetType("UnitTestScript"_Name);

	if (type == nullptr)
	{
		LOG(LogUnitTest, Error, "Could not run test, the script we use for testing no longer exists");
		return UnitTest::Failure;
	}

	FuncResult instance1 = type->Construct();
	FuncResult instance2 = type->Construct();

	if (instance1.HasError()
		|| instance2.HasError())
	{
		LOG(LogUnitTest, Error, "Scripts do not produce a default constructible type - {}", instance1.Error());
		return UnitTest::Failure;
	}

	for (int i = 1; i < 3; i++)
	{
		float expectedValue = static_cast<float>(i) * 20.525f;

		FuncResult setResult = CallScriptFuncChecked("UnitTestScript"_Name, "SetFloat"_Name, instance1.GetReturnValue(), expectedValue);

		if (setResult.HasError())
		{
			LOG(LogUnitTest, Error, "Function call failed: {}", setResult.Error());
			return UnitTest::Failure;
		}

		FuncResult getResult1 = CallScriptFuncChecked("UnitTestScript"_Name, "GetFloat"_Name, instance1.GetReturnValue());
		FuncResult getResult2 = CallScriptFuncChecked("UnitTestScript"_Name, "GetFloat"_Name, instance2.GetReturnValue());

		if (getResult1.HasError())
		{
			LOG(LogUnitTest, Error, "Function call failed: {}", getResult1.Error());
			return UnitTest::Failure;
		}

		if (getResult2.HasError())
		{
			LOG(LogUnitTest, Error, "Function call failed: {}", getResult2.Error());
			return UnitTest::Failure;
		}

		if (!getResult1.HasReturnValue()
			|| !getResult2.HasReturnValue())
		{
			LOG(LogUnitTest, Error, "Function returned void unexpectedly");
			return UnitTest::Failure;
		}

		float* returnValue1 = getResult1.GetReturnValue().As<float>();
		float* returnValue2 = getResult2.GetReturnValue().As<float>();

		if (returnValue1 == nullptr
			|| returnValue2 == nullptr)
		{
			LOG(LogUnitTest, Error, "Function returned something that wasnt a float");
		}

		if (expectedValue != *returnValue1)
		{
			LOG(LogUnitTest, Error, "GetFloat returned {}, but we just set that value to {} - Expected them to match",
				*returnValue1, expectedValue);
			return UnitTest::Failure;
		}

		if (*returnValue1 == *returnValue2)
		{
			LOG(LogUnitTest, Error, "Calling a non-static function on one instance somehow influenced the other instance as well",
				*returnValue1, expectedValue);
			return UnitTest::Failure;
		}
	}

	return UnitTest::Success;
}

UNIT_TEST(Scripting, SimpleWhileLoop)
{
	return RunSimpleLoopTest("SimpleWhileLoop"_Name);
}

UNIT_TEST(Scripting, SimpleForLoop)
{
	return RunSimpleLoopTest("SimpleForLoop"_Name);
}

UNIT_TEST(Scripting, NestedForLoop)
{
	return RunSimpleLoopTest("NestedForLoop"_Name, [](int32 n) { return n * (n * 2); });
}

UNIT_TEST(Scripting, BreakWhileLoop)
{
	return RunSimpleLoopTest("BreakWhileLoop"_Name);
}

UNIT_TEST(Scripting, BreakForLoop)
{
	return RunSimpleLoopTest("BreakForLoop"_Name);
}

UNIT_TEST(Scripting, BreakNestedLoop)
{
	return RunSimpleLoopTest("BreakNestedLoops"_Name);
}


UNIT_TEST(Scripting, IsNullTest)
{
	auto nullCheck = [](MetaAny&& argument, bool shouldBeNull)
		{
			FuncResult result = CallScriptFuncChecked("UnitTestScript", "IsNull", argument);

			if (result.HasError())
			{
				LOG(LogUnitTest, Error, "Failed to call IsNull function - {}", result.Error());
				return UnitTest::Failure;
			}

			TEST_ASSERT(result.HasReturnValue() && result.GetReturnValue().As<bool>() != nullptr);
			TEST_ASSERT(*result.GetReturnValue().As<bool>() == shouldBeNull);
			return UnitTest::Success;
		};

	TEST_ASSERT(nullCheck({ MakeTypeInfo<AssetHandle<Material>>(), nullptr }, true) == UnitTest::Success);
	TEST_ASSERT(nullCheck(MetaAny{ AssetHandle<Material>{ nullptr } }, true) == UnitTest::Success);
	return UnitTest::Success;
}
