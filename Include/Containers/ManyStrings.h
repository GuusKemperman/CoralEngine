#pragma once
#include <vector>

namespace CE
{
	class ManyStrings
	{
	public:
		void Emplace(std::string_view data)
		{
			const size_t dataSize = data.size() + 1;

			if (mEndIndices.empty())
			{
				mEndIndices.push_back(dataSize);
			}
			else
			{
				mEndIndices.push_back(mEndIndices.back() + dataSize);
			}

			const size_t oldBufferSize = mBuffer.size();
			const size_t newBufferSize = oldBufferSize + dataSize;

			mBuffer.resize(newBufferSize);
			memcpy(mBuffer.data() + oldBufferSize, data.data(), dataSize - 1);
		}

		std::string_view operator[](const size_t index) const
		{
			const size_t startIndex = index == 0 ? 0 : mEndIndices[index - 1];
			const size_t endIndex = mEndIndices[index];
			return { &mBuffer[startIndex], endIndex - startIndex - 1 };
		}

		void Clear()
		{
			mBuffer.clear();
			mEndIndices.clear();
		}

		const char* Data() const
		{
			return mBuffer.data();
		}

		size_t NumOfStrings() const { return mEndIndices.size(); }
		size_t SizeInBytes() const { return mBuffer.size(); }

	private:
		std::vector<char> mBuffer{};
		std::vector<size_t> mEndIndices{};
	};
}