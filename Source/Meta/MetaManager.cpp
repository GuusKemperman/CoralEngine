#include "Precomp.h"
#include "Meta/MetaManager.h"

#include "Meta/MetaType.h"

namespace Engine::Internal
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

namespace Engine
{
	static std::unordered_map<TypeId, MetaType> mTypeByTypeId{};
	static std::unordered_map<Name::HashType, std::reference_wrapper<MetaType>> mTypeByName{};
}

void Engine::MetaManager::PostConstruct()
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

Engine::MetaType& Engine::MetaManager::AddType(MetaType&& type)
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

	[[maybe_unused]] const auto nameInsertResult = mTypeByName.emplace(std::make_pair(Name::HashString(returnValue.GetName()), std::ref(returnValue)));
	ASSERT(nameInsertResult.second);

	return returnValue;
}

Engine::MetaType* Engine::MetaManager::TryGetType(const Name typeName)
{
	const auto it = mTypeByName.find(typeName.GetHash());
	return it == mTypeByName.end() ? nullptr : &it->second.get();
}

Engine::MetaType* Engine::MetaManager::TryGetType(const TypeId typeId)
{
	const auto it = mTypeByTypeId.find(typeId);
	return it == mTypeByTypeId.end() ? nullptr : &it->second;
}

Engine::MetaManager::EachTypeT Engine::MetaManager::EachType()
{
	return { mTypeByName.begin(), mTypeByName.end() };
}

bool Engine::MetaManager::RemoveType(const TypeId typeId)
{
	const auto it = mTypeByTypeId.find(typeId);

	if (it == mTypeByTypeId.end())
	{
		return false;
	}

	const bool erasedFromBoth = mTypeByName.erase(Name::HashString(it->second.GetName())) && mTypeByTypeId.erase(typeId);

	if (!erasedFromBoth)
	{
		LOG_TRIVIAL(LogMeta, Error, "Somehow type a type was present in mTypeByTypeId but not in mTypeByTypeName");
	}

	return erasedFromBoth;
}

bool Engine::MetaManager::RemoveType(const Name typeName)
{
	const auto it = mTypeByTypeId.find(typeName.GetHash());

	if (it == mTypeByTypeId.end())
	{
		return false;
	}

	const bool erasedFromBoth = mTypeByName.erase(typeName.GetHash()) && mTypeByTypeId.erase(it->second.GetTypeId());

	if (!erasedFromBoth)
	{
		LOG_TRIVIAL(LogMeta, Error, "Somehow type a type was present in mTypeByTypeName but not in mTypeByTypeId");
	}

	return erasedFromBoth;
}

bool Engine::Internal::DoesTypeExist(const TypeId typeId)
{
	return MetaManager::Get().TryGetType(typeId) != nullptr;
}