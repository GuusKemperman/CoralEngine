#include "Precomp.h"
#include "Meta/MetaProps.h"

#include "Utilities/BinarySerialization.h"

CE::MetaProps& CE::MetaProps::Add(const Name name)
{
	mProperties[name.GetHash()] = std::make_pair(name.String(), "");
	return *this;
}

CE::MetaProps& CE::MetaProps::Add(const MetaProps& other)
{
	for (const auto& [nameHash, value] : other.mProperties)
	{
		if (Has(Name{nameHash}))
		{
			continue;
		}

		mProperties[nameHash] = value;
	}

	return *this;
}

void CE::MetaProps::Remove(Name name)
{
	mProperties.erase(name.GetHash());
}

bool CE::MetaProps::Has(const Name name) const
{
	return mProperties.find(name.GetHash()) != mProperties.end();
}

void CE::MetaProps::save(cereal::BinaryOutputArchive& ar) const
{
	ar(mProperties);
}

void CE::MetaProps::load(cereal::BinaryInputArchive& ar)
{
	ar(mProperties);
}
