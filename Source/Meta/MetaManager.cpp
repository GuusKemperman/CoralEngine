#include "Precomp.h"
#include "Meta/MetaManager.h"

#include "Core/Engine.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/NameLookUp.h"
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
	static std::unordered_set<TypeId> mBannedTypes{};
}

CE::MetaManager::MetaManager(const EngineConfig& config)
{
	mBannedTypes = config.mBannedTypes;
}

void CE::MetaManager::PostConstruct()
{
	for (const auto& [typeId, func] : Internal::GetTypesReflectedAtStartUp())
	{
		if (TryGetType(typeId) != nullptr
			|| IsBanned(typeId))
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

	if (IsBanned(type.GetTypeId()))
	{
		LOG(LogMeta, Error, "Type {} was banned, but still ended up in runtime reflection system.", type.GetName());
	}

	auto typeInsertResult = mTypeByTypeId.emplace(type.GetTypeId(), std::move(type));
	ASSERT(typeInsertResult.second);

	MetaType& returnValue = typeInsertResult.first->second;

	Name::HashType currentNameHashed = Name::HashString(returnValue.GetName());

	[[maybe_unused]] const auto nameInsertResult = mTypeByName.emplace(currentNameHashed, returnValue);
	ASSERT(nameInsertResult.second);

	sNameLookUpMutex.lock();
	sNameLookUp.emplace(currentNameHashed, returnValue.GetName());

	const MetaProps& props = returnValue.GetProperties();
	const std::optional<std::string> oldNames = props.TryGetValue<std::string>(Props::sOldNames);

	if (oldNames.has_value())
	{
		for (const std::string_view oldName : StringFunctions::SplitString(*oldNames, ","))
		{
			const uint32 hash = Name::HashString(oldName);
			[[maybe_unused]] const auto oldNameInsertResult = mTypeByName.emplace(Name::HashString(oldName), returnValue);
			sNameLookUp.emplace(hash, oldName);

			ASSERT(oldNameInsertResult.second);
		}
	}

	sNameLookUpMutex.unlock();

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

bool CE::MetaManager::RemoveType(const TypeId typeId)
{
	const auto it = mTypeByTypeId.find(typeId);

	if (it == mTypeByTypeId.end())
	{
		return false;
	}

	// Multiple names can point to the same type
	for (auto nameIt = mTypeByName.begin(); nameIt != mTypeByName.end();)
	{
		if (nameIt->second.get() == it->second)
		{
			nameIt = mTypeByName.erase(nameIt);
		}
		else
		{
			++nameIt;
		}
	}
	mTypeByTypeId.erase(it);

	return true;
}

bool CE::MetaManager::IsBanned(TypeId typeId) const
{
	return mBannedTypes.contains(typeId);
}

bool CE::Internal::DoesTypeExist(const TypeId typeId)
{
	return MetaManager::Get().TryGetType(typeId) != nullptr;
}
