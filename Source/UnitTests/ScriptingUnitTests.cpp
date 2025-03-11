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

static void RunSimpleLoopTest(const Name funcName, std::function<int32(int32)> expectedNumOfIterations = [](int32 n) { return n; })
{
	const MetaType* const type = MetaManager::Get().TryGetType("UnitTestScript"_Name);

	TEST_NOT_NULL(type);

	for (int32 i = 0; i < 10; i++)
	{
		FuncResult instance = type->Construct();

		if (instance.HasError())
		{
			TEST_FAILURE("Scripts do not produce a default constructible type - {}", instance.Error());
		}

		FuncResult result = CallScriptFuncChecked("UnitTestScript"_Name, funcName, instance.GetReturnValue(), i);

		if (result.HasError())
		{
			TEST_FAILURE("Expected {} iterations, but the function returned an error - {}", i, result.Error());
		}

		if (!result.HasReturnValue()
			|| result.GetReturnValue().As<int32>() == nullptr)
		{
			TEST_FAILURE("Function did not return an integer");
		}

		int32 expected = expectedNumOfIterations(i);

		if (*result.GetReturnValue().As<int32>() != expected)
		{
			TEST_FAILURE("Loopbody SHOULD have ran {} times with i = {}, but ran {} times instead!", expected, i, *result.GetReturnValue().As<int32>());
		}
	}
}

UNIT_TEST(Scripting, ParamsAndReturn)
{
	for (int32 i = 0; i < 10; i++)
	{
		FuncResult isEvenResult = CallScriptFuncChecked("UnitTestScript"_Name, "IsEven"_Name, i);
		FuncResult isOddResult = CallScriptFuncChecked("UnitTestScript"_Name, "IsOdd"_Name, i);

		if (isEvenResult.HasError())
		{
			TEST_FAILURE("{}", isEvenResult.Error());
		}
		else if (isOddResult.HasError())
		{
			TEST_FAILURE("{}", isOddResult.Error());
		}

		if (!isEvenResult.HasReturnValue()
			|| !isOddResult.HasReturnValue())
		{
			TEST_FAILURE("Expected a return value!");
		}

		if (!isEvenResult.GetReturnValue().IsExactly<bool>()
			|| !isOddResult.GetReturnValue().IsExactly<bool>())
		{
			TEST_FAILURE("Expected booleans!");
		}
	}
}

UNIT_TEST(Scripting, NonStaticFunctions)
{
	const MetaType* const type = MetaManager::Get().TryGetType("UnitTestScript"_Name);

	if (type == nullptr)
	{
		TEST_FAILURE("Could not run test, the script we use for testing no longer exists");
	}

	FuncResult instance1 = type->Construct();
	FuncResult instance2 = type->Construct();

	if (instance1.HasError()
		|| instance2.HasError())
	{
		TEST_FAILURE("Scripts do not produce a default constructible type - {}", instance1.Error());
	}

	for (int i = 1; i < 3; i++)
	{
		float expectedValue = static_cast<float>(i) * 20.525f;

		FuncResult setResult = CallScriptFuncChecked("UnitTestScript"_Name, "SetFloat"_Name, instance1.GetReturnValue(), expectedValue);

		if (setResult.HasError())
		{
			TEST_FAILURE("Function call failed: {}", setResult.Error());
		}

		FuncResult getResult1 = CallScriptFuncChecked("UnitTestScript"_Name, "GetFloat"_Name, instance1.GetReturnValue());
		FuncResult getResult2 = CallScriptFuncChecked("UnitTestScript"_Name, "GetFloat"_Name, instance2.GetReturnValue());

		if (getResult1.HasError())
		{
			TEST_FAILURE("Function call failed: {}", getResult1.Error());
		}

		if (getResult2.HasError())
		{
			TEST_FAILURE("Function call failed: {}", getResult2.Error());
		}

		if (!getResult1.HasReturnValue()
			|| !getResult2.HasReturnValue())
		{
			TEST_FAILURE("Function returned void unexpectedly");
		}

		float* returnValue1 = getResult1.GetReturnValue().As<float>();
		float* returnValue2 = getResult2.GetReturnValue().As<float>();

		if (returnValue1 == nullptr
			|| returnValue2 == nullptr)
		{
			TEST_FAILURE("Function returned something that wasnt a float");
		}

		if (expectedValue != *returnValue1)
		{
			TEST_FAILURE("GetFloat returned {}, but we just set that value to {} - Expected them to match",
				*returnValue1, expectedValue);
		}

		if (*returnValue1 == *returnValue2)
		{
			TEST_FAILURE("Calling a non-static function on one instance somehow influenced the other instance as well",
				*returnValue1, expectedValue);
		}
	}
}

UNIT_TEST(Scripting, SimpleWhileLoop)
{
	RunSimpleLoopTest("SimpleWhileLoop"_Name);
}

UNIT_TEST(Scripting, SimpleForLoop)
{
	RunSimpleLoopTest("SimpleForLoop"_Name);
}

UNIT_TEST(Scripting, NestedForLoop)
{
	RunSimpleLoopTest("NestedForLoop"_Name, [](int32 n) { return n * (n * 2); });
}

UNIT_TEST(Scripting, BreakWhileLoop)
{
	RunSimpleLoopTest("BreakWhileLoop"_Name);
}

UNIT_TEST(Scripting, BreakForLoop)
{
	RunSimpleLoopTest("BreakForLoop"_Name);
}

UNIT_TEST(Scripting, BreakNestedLoop)
{
	RunSimpleLoopTest("BreakNestedLoops"_Name);
}


UNIT_TEST(Scripting, IsNullTest)
{
	auto nullCheck = [](MetaAny&& argument, bool shouldBeNull)
		{
			FuncResult result = CallScriptFuncChecked("UnitTestScript", "IsNull", argument);

			if (result.HasError())
			{
				TEST_FAILURE("Failed to call IsNull function - {}", result.Error());
			}

			TEST_ASSERT(result.HasReturnValue() && result.GetReturnValue().As<bool>() != nullptr);
			TEST_ASSERT(*result.GetReturnValue().As<bool>() == shouldBeNull);
		};

	nullCheck({ MakeTypeInfo<AssetHandle<Material>>(), nullptr }, true);
	nullCheck(MetaAny{ AssetHandle<Material>{ nullptr } }, true);
}
