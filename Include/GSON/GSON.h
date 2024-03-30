#pragma once

namespace CE
{
	template<typename Derived>
	class GSONMemberBase
	{
	public:
		using MemberType = Derived;

		GSONMemberBase() = default;
		GSONMemberBase(std::string name) :
			mName(std::move(name)) { }

		GSONMemberBase(const MemberType& other) :
			mName(other.mName),
			mData(other.mData) {}

		GSONMemberBase(MemberType&& other) noexcept :
			mName(std::move(other.mName)),
			mData(std::move(other.mData)) {}

		MemberType& operator=(const MemberType& other)
		{
			mName = other.mName;
			mData = other.mData;
			return *this;
		}

		MemberType& operator=(MemberType&& other) noexcept
		{
			mName = std::move(other.mName);
			mData = std::move(other.mData);
			return *this;
		}

		const std::string& GetName() const { return mName; }
		void SetName(std::string_view name) { mName = std::string{ name }; }
		void SetName(std::string&& name) { mName = std::move(name); }

		std::string_view GetData() const { return mData; }
		void SetData(std::string_view data) { mData = std::string{ data }; }
		void SetData(std::string&& data) { mData = std::move(data); }

		void Clear() { mData.clear(); }

		constexpr auto operator==(const MemberType& other) const { return mData == other.mData; }
		constexpr auto operator!=(const MemberType& other) const { return mData != other.mData; }
		constexpr auto operator<(const MemberType& other) const { return mData < other.mData; }

	protected:
		std::string mName{};
		std::string mData{};
	};

	template<typename ObjectT, typename MemberT>
	class GSONObjectBase
	{
	public:
		using MemberType = MemberT;
		using ObjectType = ObjectT;

		GSONObjectBase() = default;
		GSONObjectBase(const ObjectType& other);
		GSONObjectBase(ObjectType&& other) noexcept;
		GSONObjectBase(std::string_view name, ObjectType* parent = nullptr);

		~GSONObjectBase() = default;

		ObjectType& operator=(const ObjectType& other);
		ObjectType& operator=(ObjectType&& other) noexcept;

		const std::vector<ObjectType>& GetChildren() const { return mChildren; }
		std::vector<ObjectType>& GetChildren() { return mChildren; }

		const std::vector<MemberType>& GetGSONMembers() const { return mMembers; }
		std::vector<MemberType>& GetGSONMembers() { return mMembers; }

		const std::string& GetName() const { return mName; }
		void SetName(std::string name) { mName = std::move(name); }

		const ObjectType* GetParent() const { return mParent; }
		ObjectType* GetParent() { return mParent; }

		void Clear() { mChildren.clear(); mMembers.clear(); }
		bool IsEmpty() const { return mChildren.empty() && mMembers.empty(); }

		ObjectType* TryGetGSONObject(std::string_view name);
		const ObjectType* TryGetGSONObject(std::string_view name) const;

		ObjectType& GetGSONObject(std::string_view name);
		const ObjectType& GetGSONObject(std::string_view name) const;

		ObjectType& AddGSONObject(std::string_view objectName);
		ObjectType& GetOrAddGSONObject(std::string_view objectName);

		MemberType* TryGetGSONMember(std::string_view name);
		const MemberType* TryGetGSONMember(std::string_view name) const;

		MemberType& GetGSONMember(std::string_view name);
		const MemberType& GetGSONMember(std::string_view name) const;

		MemberType& AddGSONMember(std::string_view memberName);
		MemberType& GetOrAddGSONMember(std::string_view memberName);

		// May invalidate pointers to GSONObjects owned by this object you've been holding on to.
		void RemoveGSONObjectsWithName(std::string_view objectsName);

		// May invalidate pointers to GSONObjects owned by this object you've been holding on to.
		void RemoveGSONObject(ObjectType& object);

		// May invalidate pointers to GSONMembers owned by this object you've been holding on to.
		void RemoveGSONMembersWithName(std::string_view memberName);

		// May invalidate pointers to GSONMembers owned by this object you've been holding on to.
		void RemoveGSONMember(MemberType& member);

	protected:
		std::string mName{};
		ObjectType* mParent{};

		std::vector<ObjectType> mChildren{};
		std::vector<MemberType> mMembers{};
	};

	template<typename ObjectT, typename MemberT>
	GSONObjectBase<ObjectT, MemberT>::GSONObjectBase(const ObjectType& other) :
		mName(other.mName),
		mParent(other.mParent),
		mChildren(other.mChildren),
		mMembers(other.mFields)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i].mParent = this;
		}
	}

	template<typename ObjectT, typename MemberT>
	GSONObjectBase<ObjectT, MemberT>::GSONObjectBase(ObjectType&& other) noexcept :
		mName(std::move(other.mName)),
		mParent(other.mParent),
		mChildren(std::move(other.mChildren)),
		mMembers(std::move(other.mFields))
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i].mParent = this;
		}
	}

	template<typename ObjectT, typename MemberT>
	GSONObjectBase<ObjectT, MemberT>::GSONObjectBase(const std::string_view name, ObjectType* const parent) :
		mName(std::string{ name }),
		mParent(parent)
	{
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::operator=(const ObjectType& other)
	{
		mName = other.mName;
		mChildren = other.GetChildren();
		mMembers = other.GetGSONMembers();

		for (ObjectType& child : mChildren)
		{
			child.mParent = this;
		}

		return *this;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::operator=(ObjectType&& other) noexcept
	{
		mName = std::move(other.mName);
		mChildren = std::move(other.GetChildren());
		mMembers = std::move(other.GetGSONMembers());

		for (ObjectType& child : mChildren)
		{
			child.mParent = this;
		}

		return *this;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType* GSONObjectBase<ObjectT, MemberT>::TryGetGSONObject(const std::string_view name)
	{
		return const_cast<ObjectType*>(const_cast<const GSONObjectBase<ObjectT, MemberT>&>(*this).TryGetGSONObject(name));
	}

	template<typename ObjectT, typename MemberT>
	const typename GSONObjectBase<ObjectT, MemberT>::ObjectType* GSONObjectBase<ObjectT, MemberT>::TryGetGSONObject(const std::string_view name) const
	{
		auto it = find_if(mChildren.begin(), mChildren.end(),
			[name](const ObjectType& s)
			{
				return s.mName == name;
			});

		return it != mChildren.end() ? &*it : nullptr;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::GetGSONObject(const std::string_view name)
	{
		auto* found = TryGetGSONObject(name);
		ASSERT_LOG(found != nullptr, "{} has no object with name {}", mName, name);
		return *found;
	}

	template<typename ObjectT, typename MemberT>
	const typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::GetGSONObject(const std::string_view name) const
	{
		auto* found = TryGetGSONObject(name);
		ASSERT_LOG(found != nullptr, "{} has no object with name {}", mName, name);
		return *found;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::AddGSONObject(const std::string_view objectName)
	{
		return mChildren.emplace_back(objectName, static_cast<ObjectType*>(this));
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::ObjectType& GSONObjectBase<ObjectT, MemberT>::GetOrAddGSONObject(const std::string_view objectName)
	{
		ObjectType* const existingObject = TryGetGSONObject(objectName);
		return existingObject == nullptr ? AddGSONObject(objectName) : *existingObject;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::MemberType* GSONObjectBase<ObjectT, MemberT>::TryGetGSONMember(const std::string_view name)
	{
		return const_cast<MemberType*>(const_cast<const GSONObjectBase<ObjectT, MemberT>&>(*this).TryGetGSONMember(name));
	}

	template<typename ObjectT, typename MemberT>
	const typename GSONObjectBase<ObjectT, MemberT>::MemberType* GSONObjectBase<ObjectT, MemberT>::TryGetGSONMember(const std::string_view name) const
	{
		auto it = find_if(mMembers.begin(), mMembers.end(),
			[name](const MemberType& v)
			{
				return v.GetName() == name;
			});
		return it != mMembers.end() ? &*it : nullptr;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::MemberType& GSONObjectBase<ObjectT, MemberT>::GetGSONMember(const std::string_view name)
	{
		auto* found = TryGetGSONMember(name);
		ASSERT_LOG(found != nullptr, "{} has no member with name {}", mName, name);
		return *found;
	}

	template<typename ObjectT, typename MemberT>
	const typename GSONObjectBase<ObjectT, MemberT>::MemberType& GSONObjectBase<ObjectT, MemberT>::GetGSONMember(const std::string_view name) const
	{
		auto* found = TryGetGSONMember(name);
		ASSERT_LOG(found != nullptr, "{} has no member with name {}", mName, name);
		return *found;
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::MemberType& GSONObjectBase<ObjectT, MemberT>::AddGSONMember(const std::string_view memberName)
	{
		return mMembers.emplace_back(std::string{ memberName });
	}

	template<typename ObjectT, typename MemberT>
	typename GSONObjectBase<ObjectT, MemberT>::MemberType& GSONObjectBase<ObjectT, MemberT>::GetOrAddGSONMember(const std::string_view memberName)
	{
		MemberType* existingMember = TryGetGSONMember(memberName);
		return existingMember == nullptr ? AddGSONMember(memberName) : *existingMember;
	}

	template<typename ObjectT, typename MemberT>
	void GSONObjectBase<ObjectT, MemberT>::RemoveGSONObjectsWithName(const std::string_view objectsName)
	{
		mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(),
			[objectsName](const ObjectType& child)
			{
				return child.GetName() == objectsName;
			}), mChildren.end());
	}

	template<typename ObjectT, typename MemberT>
	void GSONObjectBase<ObjectT, MemberT>::RemoveGSONObject(ObjectType& object)
	{
		const intptr objectAdress = reinterpret_cast<intptr>(&object);
		const intptr myObjectsAdress = reinterpret_cast<intptr>(mChildren.data());

		if (objectAdress < myObjectsAdress
			|| objectAdress >= myObjectsAdress + static_cast<intptr>(mChildren.size() * sizeof(object)))
		{
			LOG(LogTemp, Warning, "GSONObject was not removed, as {} is not owned by {}", static_cast<void*>(&object), static_cast<void*>(this));
			return;
		}

		const size_t objectIndex = (objectAdress - myObjectsAdress) / sizeof(object);
		mChildren.erase(mChildren.begin() + objectIndex);
	}

	template<typename ObjectT, typename MemberT>
	void GSONObjectBase<ObjectT, MemberT>::RemoveGSONMembersWithName(const std::string_view memberName)
	{
		mMembers.erase(std::remove_if(mMembers.begin(), mMembers.end(),
			[memberName](const MemberType& child)
			{
				return child.GetName() == memberName;
			}), mMembers.end());
	}

	template<typename ObjectT, typename MemberT>
	void GSONObjectBase<ObjectT, MemberT>::RemoveGSONMember(MemberType& member)
	{
		const intptr memberAdress = reinterpret_cast<intptr>(&member);
		const intptr myMembersAdress = reinterpret_cast<intptr>(mMembers.data());

		if (memberAdress < myMembersAdress
			|| memberAdress >= myMembersAdress + static_cast<intptr>(mMembers.size() * sizeof(member)))
		{
			LOG(LogTemp, Warning, "GSONMember was not removed, as {} is not owned by {}", static_cast<void*>(&member), static_cast<void*>(this));
			return;
		}

		const size_t memberIndex = (memberAdress - myMembersAdress) / sizeof(member);
		mMembers.erase(mMembers.begin() + memberIndex);
	}
}