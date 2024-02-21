#pragma once
#include "GSON.h"

#include "Utilities/BinarySerialization.h"

namespace Engine
{
	class BinaryGSONMember :
		public GSONMemberBase<BinaryGSONMember>
	{
	public:
		using GSONMemberBase::GSONMemberBase;

		template<typename T>
		void SerializeIntoName(const T& value)
		{
			mName = ToBinary(value);
		}

		template<typename T>
		void DeserializeFromName(T& out)
		{
			FromBinary(mName, out);
		}

		template<typename T>
		void operator<<(const T& value)
		{
			mData = ToBinary(value);
		}

		template<typename T>
		void operator>>(T& out) const
		{
			FromBinary(mData, out);
		}

		friend class BinaryGSONObject;
	};

	class BinaryGSONObject :
		public GSONObjectBase<BinaryGSONObject, BinaryGSONMember>
	{
	public:
		using GSONObjectBase::GSONObjectBase;

		void SaveToBinary(std::ostream& ostream) const;
		void SaveToHex(std::ostream& ostream) const;

		// Returns true on success
		bool LoadFromBinary(std::istream& ostream);

		void LoadFromHex(std::istream& ostream);
	};
}