#include "Precomp.h"
#include "Meta/MetaManager.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/StringFunctions.h"

namespace CE::Internal
{
	// Has to be a function to ensure it is constructed before the other static variables
	// start calling RegisterReflectFunc
	static std::vector<std::pair<TypeId, std::function<MetaType()>>>& GetTypesReflectedAtStartUp()
	{
		static std::vector<std::pair<TypeId, std::function<MetaType()>>> v{};
		return v;
	}
	
	void RegisterReflectFunc(const TypeId typeId, MetaType(*reflectFunc)())
	{
		GetTypesReflectedAtStartUp().emplace_back(typeId, reflectFunc);
	}
}

namespace CE
{
	static std::unordered_map<TypeId, MetaType> mTypeByTypeId{};
	static std::unordered_map<Name::HashType, std::reference_wrapper<MetaType>> mTypeByName{};
}

void CE::MetaManager::PostConstruct()
{
	for (const auto& [typeId, func] : Internal::GetTypesReflectedAtStartUp())
	{
		if (TryGetType(typeId) != nullptr)
		{
			continue;
		}

		[[maybe_unused]] MetaType& newType = AddType(func());
		ASSERT(newType.GetTypeId() == typeId && "You have reflected a different type, check what you passed into MetaType constructor");
	}
}

CE::MetaType& CE::MetaManager::AddType(MetaType&& type)
{
	if (MetaType* existingType = TryGetType(type.GetTypeId()); existingType != nullptr)
	{
		if (type.GetName() == existingType->GetName())
		{
			LOG(LogMeta, Warning, "Tried to add {} twice", existingType->GetName());
		}
		else
		{
			LOG(LogMeta, Error, "TypeId clash!! Both {} and {} use typeId {}! Rename either one of these to resolve this.", existingType->GetName(), type.GetName(), type.GetTypeId());
		}
		return *existingType;
	}

	if (MetaType* existingType = TryGetType(type.GetName()); existingType != nullptr)
	{
		LOG(LogMeta, Error, "There is already a type with the name {}", type.GetName());
		return *existingType;
	}

	auto typeInsertResult = mTypeByTypeId.emplace(type.GetTypeId(), std::move(type));
	ASSERT(typeInsertResult.second);

	MetaType& returnValue = typeInsertResult.first->second;

	[[maybe_unused]] const auto nameInsertResult = mTypeByName.emplace(Name::HashString(returnValue.GetName()), returnValue);
	ASSERT(nameInsertResult.second);

	const MetaProps& props = returnValue.GetProperties();
	const std::optional<std::string> oldNames = props.TryGetValue<std::string>(Props::sOldNames);

	if (oldNames.has_value())
	{
		for (const std::string_view oldName : StringFunctions::SplitString(*oldNames, ","))
		{
			const auto oldNameInsertResult = mTypeByName.emplace(Name::HashString(oldName), returnValue);
			ASSERT(oldNameInsertResult.second);
		}
	}

	return returnValue;
}

CE::MetaType* CE::MetaManager::TryGetType(const Name typeName)
{
	const auto it = mTypeByName.find(typeName.GetHash());
	return it == mTypeByName.end() ? nullptr : &it->second.get();
}

CE::MetaType* CE::MetaManager::TryGetType(const TypeId typeId)
{
	const auto it = mTypeByTypeId.find(typeId);
	return it == mTypeByTypeId.end() ? nullptr : &it->second;
}

CE::MetaManager::EachTypeT CE::MetaManager::EachType()
{
	return { mTypeByTypeId.begin(), mTypeByTypeId.end() };
}

static bool EraseFromBoth(std::unordered_map<CE::TypeId, CE::MetaType>::iterator it)
{
	if (it == CE::mTypeByTypeId.end())
	{
		return false;
	}

	for (auto nameIt = CE::mTypeByName.begin(); nameIt != CE::mTypeByName.end();)
	{
		if (nameIt->second.get() == it->second)
		{
			nameIt = CE::mTypeByName.erase(nameIt);
		}
		else
		{
			++nameIt;
		}
	}
	CE::mTypeByTypeId.erase(it);

	return true;
};

bool CE::MetaManager::RemoveType(const TypeId typeId)
{
	const auto it = mTypeByTypeId.find(typeId);
	return EraseFromBoth(it);
}

bool CE::MetaManager::RemoveType(const Name typeName)
{
	const auto it = mTypeByTypeId.find(typeName.GetHash());
	return EraseFromBoth(it);
}

bool CE::Internal::DoesTypeExist(const TypeId typeId)
{
	return MetaManager::Get().TryGetType(typeId) != nullptr;
}