#pragma once
#include "GSON.h"

#include <ostream>
#include <sstream>

namespace Engine
{
	class ReadableGSONMember :
		public GSONMemberBase<ReadableGSONMember>
	{
	public:
		using GSONMemberBase::GSONMemberBase;

		template<typename Readable>
		void operator<<(const Readable& value)
		{
			std::stringstream tmp{};
			tmp << value;
			mData = tmp.str();
		}

		template<typename Readable>
		void operator>>(Readable& out) const
		{
			std::stringstream tmp{ mData };
			tmp >> out;
		}

		void operator<<(const std::string& value)
		{
			mData = value;
		}

		void operator>>(std::string& out) const
		{
			out = mData;
		}

		friend class ReadableGSONObject;
	};

	class ReadableGSONObject :
		public GSONObjectBase<ReadableGSONObject, ReadableGSONMember>
	{
	public:
		using GSONObjectBase::GSONObjectBase;

		void SaveTo(std::ostream& ostream) const;
		void LoadFrom(std::istream& ostream);
	};
}