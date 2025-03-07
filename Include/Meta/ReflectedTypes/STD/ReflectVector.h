#pragma once
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

template<typename T>
struct Reflector<std::vector<T>>
{
	static_assert(CE::sIsReflectable<T>, "Cannot reflect a vector of a type that is not reflected");

	static CE::MetaType Reflect()
	{
		using namespace CE;

		const MetaType& basedOnType = MetaManager::Get().GetType<T>();

		MetaType arrayType{ MetaType::T<std::vector<T>>{}, Format("{} array", basedOnType.GetName()) };

		// TODO this function always add Props::sIsScriptableTag and Props::sIsScriptOwnableTag, but this really
		// shouldnt be done implicitely
		arrayType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

		arrayType.AddFunc(
			[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
			{
				const std::vector<T>& v = *static_cast<const std::vector<T>*>(args[0].GetData());
				const int32 i = *static_cast<const int32*>(args[1].GetData());

				if (i < 0
					|| i >= static_cast<int32>(v.size()))
				{
					return Format("Index out of range, index was {} while vector size was {}", i, v.size());
				}

				return MetaAny{ T{ v[i] }, rvoBuffer };
			},
			"Get (a copy)",
			MetaFunc::Return{ MakeTypeTraits<T>() }, // Return value
			MetaFunc::Params{ { MakeTypeTraits<const std::vector<T>&>(), "Array" }, { MakeTypeTraits<const int32&>(), "Index" } }).GetProperties().Add(Props::sIsScriptableTag);

		if constexpr (!std::is_same_v<bool, T>) // Cant get a reference to a boolean inside std::vector<bool>
		{
			arrayType.AddFunc(
				[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer) -> FuncResult
				{
					std::vector<T>& v = *static_cast<std::vector<T>*>(args[0].GetData());
					const int32 i = *static_cast<const int32*>(args[1].GetData());

					if (i < 0
						|| i >= static_cast<int32>(v.size()))
					{
						return Format("Index out of range, index was {} while vector size was {}", i, v.size());
					}

					return MetaAny{ v[i] };
				},
				"Get",
				MetaFunc::Return{ MakeTypeTraits<T&>() }, // Return value
				MetaFunc::Params{ { MakeTypeTraits<std::vector<T>&>(), "Array" }, { MakeTypeTraits<const int32&>(), "Index" } }).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

		}

		arrayType.AddFunc(static_cast<void(std::vector<T>::*)()>(&std::vector<T>::clear), "Clear").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc(static_cast<void(std::vector<T>::*)(const T&)>(&std::vector<T>::push_back), "Add to end").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc([](std::vector<T>& v)
			{
				if (!v.empty())
				{
					v.pop_back();
				}
				else
				{
					LOG(LogScripting, Warning, "Called PopBack on empty array");
				}
			}, "Remove from end").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc([](std::vector<T>& v, const int32& newSize)
			{
				if (newSize < 0)
				{
					LOG(LogScripting, Warning, "Cannot resize array to negative value {}, will resize to 0 instead", newSize);
				}

				v.resize(static_cast<size_t>(std::max(newSize, 0)));
			}, "Resize").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc([](const std::vector<T>& v) -> int32
			{
				return static_cast<int32>(v.size());
			}, "Get size").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc([](std::vector<T>& v)
			{
				std::reverse(v.begin(), v.end());
			}, "Reverse").GetProperties().Add(Props::sIsScriptableTag);

		arrayType.AddFunc([](std::vector<T>& v, const std::vector<T>& other)
			{
				v.insert(v.end(), other.begin(), other.end());
			}, "Combine").GetProperties().Add(Props::sIsScriptableTag);


		ReflectFieldType<std::vector<T>>(arrayType);
		return arrayType;
	}

};
