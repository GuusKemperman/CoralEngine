#pragma once
#include "Meta/Fwd/MetaPropsFwd.h"

#include "GSON/GSONReadable.h"

namespace Engine
{
	template <typename Readable>
	MetaProps& MetaProps::Set(const Name name, const Readable& value)
	{
		ReadableGSONMember helper{};
		helper << value;
		mProperties[name.GetHash()] = std::make_pair(name.String(), helper.GetData());
		return *this;
	}

	template <typename Readable>
	std::optional<Readable> MetaProps::TryGetValue(const Name name) const
	{
		const auto it = mProperties.find(name.GetHash());

		if (it == mProperties.end())
		{
			return std::nullopt;
		}

		ReadableGSONMember helper{};
		helper.SetData(it->second.second);
		Readable tmp{};
		helper >> tmp;
		return tmp;
	}

	inline MetaProps& MetaProps::Add(const Name name)
	{
		mProperties[name.GetHash()] = std::make_pair(name.String(), "");
		return *this;
	}

	inline bool MetaProps::Has(const Name name) const
	{
		return mProperties.find(name.GetHash()) != mProperties.end();
	}
}
