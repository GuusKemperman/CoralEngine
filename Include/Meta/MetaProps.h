#pragma once
#include "GSON/GSONReadable.h"

namespace Engine
{
	class ReadableGSONMember;

	/*
	Short for MetaProperties. 

	MetaTypes, MetaFuncs or MetaFields can have properties. 
	Each property is a key-value pair, and allows for some
	additional customization. See CommonMetaProperties for 
	some examples.
	*/
	class MetaProps
	{
	public:
		MetaProps& Add(const Name name)
		{
			mProperties[name.GetHash()] = std::make_pair(name.String(), "");
			return *this;
		}

		/*
		Readable can be anything that can be passed into std::cout.
		The value you set here can be retrieved using TryGetValue
		
		If there is no property with this name, one is added.
		*/
		template<typename Readable>
		MetaProps& Set(const Name name, const Readable& value)
		{
			ReadableGSONMember helper{};
			helper << value;
			mProperties[name.GetHash()] = std::make_pair(name.String(), helper.GetData());
			return *this;
		}

		/*
		Checks if there is a property with this name
		*/
		bool Has(const Name name) const
		{
			return mProperties.find(name.GetHash()) != mProperties.end();
		}

		template<typename Readable>
		std::optional<Readable> TryGetValue(const Name name) const
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

	private:
		std::unordered_map<Name::HashType, std::pair<std::string, std::string>> mProperties{};
	};
}
