#include "Precomp.h"
#include "GSON/GSONBinary.h"

#include "Utilities/StringFunctions.h"

template<typename SizeType>
static inline bool TrySaveSizeAsType(std::ostream& ostream, const size_t size)
{
	if (size < std::numeric_limits<SizeType>::max())
	{
		const SizeType smallSize = static_cast<SizeType>(size);
		ostream.write(reinterpret_cast<const char*>(&smallSize), sizeof(smallSize));
		return true;
	}
	else
	{
		constexpr SizeType max = std::numeric_limits<SizeType>::max();
		ostream.write(reinterpret_cast<const char*>(&max), sizeof(max));
		return false;
	}
}

static inline void SaveSmallSize(std::ostream& ostream, const size_t size)
{
	if (TrySaveSizeAsType<uint8>(ostream, size)
		|| TrySaveSizeAsType<uint16>(ostream, size)
		|| TrySaveSizeAsType<size_t>(ostream, size))
	{

	}
}

template<typename SizeType>
static inline bool TryLoadSizeAsType(std::istream& istream, size_t& size)
{
	SizeType smallSize{};
	istream.read(reinterpret_cast<char*>(&smallSize), sizeof(smallSize));
	size = static_cast<size_t>(smallSize);

	return smallSize != std::numeric_limits<SizeType>::max()
		&& istream.gcount() == sizeof(smallSize);
}

static inline std::optional<size_t> LoadSmallSize(std::istream& istream)
{
	size_t size{};

	if (!TryLoadSizeAsType<uint8>(istream, size)
		&& !TryLoadSizeAsType<uint16>(istream, size)
		&& !TryLoadSizeAsType<size_t>(istream, size))
	{
		LOG(LogCore, Error, "Invalid save, serialized size invalid.");
		return std::nullopt;
	}
	return size;
}

void Engine::BinaryGSONObject::SaveToBinary(std::ostream& ostream) const
{
	SaveSmallSize(ostream, mName.size());
	ostream.write(mName.c_str(), mName.size());

	SaveSmallSize(ostream, mChildren.size());

	for (const BinaryGSONObject& child : mChildren)
	{
		child.SaveToBinary(ostream);
	}

	SaveSmallSize(ostream, mMembers.size());

	for (const BinaryGSONMember& member : mMembers)
	{
		SaveSmallSize(ostream, member.mName.size());
		ostream.write(member.mName.c_str(), member.mName.size());

		SaveSmallSize(ostream, member.mData.size());
		ostream.write(member.mData.c_str(), member.mData.size());
	}
}

bool Engine::BinaryGSONObject::LoadFromBinary(std::istream& istream)
{
	const std::optional<size_t> myNameSize = LoadSmallSize(istream);

	if (!myNameSize.has_value())
	{
		return false;
	}

	mName.resize(*myNameSize);
	istream.read(mName.data(), *myNameSize);

	if (istream.gcount() != static_cast<std::streamsize>(*myNameSize))
	{
		LOG(LogCore, Error, "Invalid GSONObject: expected name of {} length, but found only {} bytes.", *myNameSize, istream.gcount());
		return false;
	}

	const std::optional<size_t> numOfChildren = LoadSmallSize(istream);

	if (!numOfChildren.has_value())
	{
		return false;
	}

	mChildren.reserve(*numOfChildren);

	for (size_t i = 0; i < *numOfChildren; i++)
	{
		if (!AddGSONObject({}).LoadFromBinary(istream))
		{
			return false;
		}
	}

	const std::optional<size_t> numOfMembers = LoadSmallSize(istream);

	if (!numOfMembers.has_value())
	{
		return false;
	}

	mMembers.reserve(*numOfMembers);

	for (size_t i = 0; i < *numOfMembers; i++)
	{
		BinaryGSONMember& newMember = AddGSONMember({});

		const std::optional<size_t> memberNameSize = LoadSmallSize(istream);

		if (!memberNameSize.has_value())
		{
			return false;
		}

		newMember.mName.resize(*memberNameSize);
		istream.read(newMember.mName.data(), *memberNameSize);
		if (istream.gcount() != static_cast<std::streamsize>(*memberNameSize))
		{
			LOG(LogCore, Error, "Invalid GSONObject: expected name of {} length, but found only {} bytes.", *memberNameSize, istream.gcount());
			return false;
		}

		const std::optional<size_t> memberDataSize = LoadSmallSize(istream);

		if (!memberDataSize.has_value())
		{
			return false;
		}

		newMember.mData.resize(*memberDataSize);
		istream.read(newMember.mData.data(), *memberDataSize);
		if (istream.gcount() != static_cast<std::streamsize>(*memberDataSize))
		{
			LOG(LogCore, Error, "Invalid GSONObject: expected name of {} length, but found only {} bytes.", *memberDataSize, istream.gcount());
			return false;
		}
	}

	return true;
}
