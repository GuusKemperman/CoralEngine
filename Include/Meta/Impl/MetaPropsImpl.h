#pragma once
#include "Meta/Fwd/MetaPropsFwd.h"

#include "GSON/GSONReadable.h"

namespace CE
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
}
