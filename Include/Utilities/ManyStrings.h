#pragma once
#include <vector>

namespace CE
{
	class ManyStrings
	{
	public:
		bool operator==(const ManyStrings& other) const;
		bool operator!=(const ManyStrings& other) const;

		void Emplace(std::string_view data);

		std::string_view operator[](const size_t index) const;

		void Clear();

		const char* Data() const;

		size_t NumOfStrings() const;
		size_t SizeInBytes() const;

	private:
		std::vector<char> mBuffer{};
		std::vector<size_t> mEndIndices{};
	};
}