#pragma once

namespace Engine
{
	class MetaType;

	template<typename Filter>
	class MetaTypeFilter
	{
	public:
		MetaTypeFilter() = default;
		MetaTypeFilter(const MetaType* type);

		MetaTypeFilter& operator=(const MetaType* type);

		bool operator==(nullptr_t) const { return mValue == nullptr; };
		bool operator==(const MetaTypeFilter& other) const { return mValue == other.mValue; };
		bool operator!=(const MetaTypeFilter& other) const { return mValue != other.mValue; };

		operator const MetaType* () const { return Get(); }

		static bool IsTypeValid(const MetaType& type);

		const MetaType* Get() const { return mValue; }

	private:
		const MetaType* mValue{};
	};

	template<class Archive, typename Filter>
	void save(Archive& ar, const MetaTypeFilter<Filter>& value);

	template<class Archive, typename Filter>
	void load(Archive& ar, MetaTypeFilter<Filter>& value);
}

