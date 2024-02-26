#pragma once

namespace Engine
{
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
		MetaProps& Add(Name name);

		/*
		Readable can be anything that can be passed into std::cout.
		The value you set here can be retrieved using TryGetValue

		If there is no property with this name, one is added.
		*/
		template<typename Readable>
		MetaProps& Set(Name name, const Readable& value);

		/*
		Checks if there is a property with this name
		*/
		bool Has(Name name) const;

		template<typename Readable>
		std::optional<Readable> TryGetValue(Name name) const;

	private:
		std::unordered_map<Name::HashType, std::pair<std::string, std::string>> mProperties{};
	};
}
