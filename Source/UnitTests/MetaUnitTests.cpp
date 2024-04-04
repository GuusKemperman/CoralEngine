#include "Precomp.h"

#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaFuncId.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaTypeTraits.h"
#include "Core/UnitTests.h"

static_assert(CE::MakeTypeTraits<const void*>().mForm == CE::TypeForm::ConstPtr);
static_assert(CE::MakeTypeTraits<const int&>().mForm == CE::TypeForm::ConstRef);
static_assert(CE::MakeTypeTraits<int>().mForm == CE::TypeForm::Value);
static_assert(CE::MakeTypeTraits<int&&>().mForm == CE::TypeForm::RValue);

static_assert(CE::MakeFuncId<void(void*)>() == CE::MakeFuncId<void(void*)>());
static_assert(CE::MakeFuncId<void(int)>() == CE::MakeFuncId<void(int)>());

using namespace CE;

UNIT_TEST(Meta, FunctionHash)
{
	MetaManager& manager = MetaManager::Get();
	const MetaType* const intType = manager.TryGetType<int32>();
	const MetaType* const floatType = manager.TryGetType<float32>();
	const MetaType* const mat4 = manager.TryGetType<glm::mat4>();

	if (intType == nullptr
		|| floatType == nullptr
		|| mat4 == nullptr)
	{
		return UnitTest::Failure;
	}

	if (intType->GetTypeId() != CE::MakeTypeId<int32>())
	{
		return UnitTest::Failure;
	}

	if (floatType->GetTypeId() != CE::MakeTypeId<float32>())
	{
		return UnitTest::Failure;
	}

	if (mat4->GetTypeId() != CE::MakeTypeId<glm::mat4>())
	{
		return UnitTest::Failure;
	}

	const MetaFunc func1{ [](int32, float32) -> glm::mat4 { return {}; }, "func1",  MetaFunc::ExplicitParams<int32, float32>{} };

	if (func1.GetFuncId() != CE::MakeFuncId<glm::mat4(int32, float32)>())
	{
		return UnitTest::Failure;
	}

	// This could obviously be done at compile time, but thats not the point of this test
	const std::vector<MetaFuncNamedParam> params{
		{ { intType->GetTypeId() } },
		{ { floatType->GetTypeId() } }
	};

	const MetaFunc dynamicFunc
	{
		{},
		"dynamicFunc",
		MetaFuncNamedParam{ { mat4->GetTypeId() } },
		params
	};

	if (dynamicFunc.GetFuncId() == func1.GetFuncId())
	{
		return UnitTest::Success;
	}
	return UnitTest::Failure;
}
