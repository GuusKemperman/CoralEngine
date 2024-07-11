#include "Precomp.h"
#include "Utilities/ManyStrings.h"

bool CE::ManyStrings::operator==(const ManyStrings& other) const
{
	// Early out if the size of either buffer does not match
	return mEndIndices.size() == other.mEndIndices.size()
		&& mBuffer.size() == other.mBuffer.size()
		&& mEndIndices == other.mEndIndices
		&& mBuffer == other.mBuffer;
}

bool CE::ManyStrings::operator!=(const ManyStrings& other) const
{
	// Early out if the size of either buffer does not match
	return mEndIndices.size() != other.mEndIndices.size()
		|| mBuffer.size() != other.mBuffer.size()
		|| mEndIndices != other.mEndIndices
		|| mBuffer != other.mBuffer;
}

void CE::ManyStrings::Emplace(std::string_view data)
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

std::string_view CE::ManyStrings::operator[](const size_t index) const
{
	const size_t startIndex = index == 0 ? 0 : mEndIndices[index - 1];
	const size_t endIndex = mEndIndices[index];
	return { &mBuffer[startIndex], endIndex - startIndex - 1 };
}

void CE::ManyStrings::Clear()
{
	mBuffer.clear();
	mEndIndices.clear();
}

const char* CE::ManyStrings::Data() const
{
	return mBuffer.data();
}

char* CE::ManyStrings::Data()
{
	return mBuffer.data();
}

size_t CE::ManyStrings::NumOfStrings() const
{
	return mEndIndices.size();
}

size_t CE::ManyStrings::SizeInBytes() const
{
	return mBuffer.size();
}
