#pragma once
#include "Meta/Fwd/MetaToolsFwd.h"
#include "Meta/MetaAny.h"

template<typename T>
std::shared_ptr<T> CE::MakeShared(MetaAny&& any)
{
	if (!any.IsOwner()
		|| any.As<T>() == nullptr)
	{
		LOG(LogMeta, Warning, "Failed to MakeShared from any; The any was either non owning or was not of the correct type");
		return nullptr;
	}
	return std::shared_ptr<T>{ static_cast<T*>(any.Release()), InPlaceDeleter<T, true>{} };
}

template<typename T>
std::unique_ptr<T, CE::InPlaceDeleter<T, true>> CE::MakeUnique(MetaAny&& any)
{
	if (!any.IsOwner()
		|| any.As<T>() == nullptr)
	{
		LOG(LogMeta, Warning, "Failed to MakeShared from any; The any was either non owning or was not of the correct type");
		return nullptr;
	}

	return std::unique_ptr<T, InPlaceDeleter<T, true>>(static_cast<T*>(any.Release()));
}