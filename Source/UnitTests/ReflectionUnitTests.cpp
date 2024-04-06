#include "Precomp.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Core/UnitTests.h"
#include "Meta/MetaType.h"
#include "Assets/Asset.h"
#include "Systems/System.h"

using namespace CE;

namespace
{
	template<typename Type, typename... CtorArgs>
	UnitTest::Result RecurivelyCheckIfHasContructor()
	{
		const MetaType& assetType = MetaManager::Get().GetType<Type>();

		UnitTest::Result result = UnitTest::Success;

		std::function<void(const MetaType&)> checkType = [&result, &checkType](const MetaType& type)
			{
				for (const MetaType& child : type.GetDirectDerivedClasses())
				{
					if (!child.IsConstructible<CtorArgs...>())
					{
						LOG(LogUnitTests, Error, "{} is missing a contructor", child.GetName());
						result = UnitTest::Failure;
					}

					checkType(child);
				}
			};
		checkType(assetType);

		return result;
	}
}

UNIT_TEST(ReflectionUnitTests, DoesEachAssetHaveLoadInfoConstructor)
{
	return RecurivelyCheckIfHasContructor<Asset, AssetLoadInfo&>();
}

UNIT_TEST(ReflectionUnitTests, DoesEachAssetHaveStringViewConstructor)
{
	return RecurivelyCheckIfHasContructor<Asset, std::string_view>();
}

UNIT_TEST(ReflectionUnitTests, DoesEachSystemHaveDefaultConstructor)
{
	return RecurivelyCheckIfHasContructor<System>();
}

