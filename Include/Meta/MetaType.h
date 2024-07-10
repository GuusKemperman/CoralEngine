#pragma once
#include "Meta/MetaFunc.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaField.h"
#include "Meta/MetaManager.h"

namespace Engine
{
	class MetaProps;
	class MetaManager;

	/*
	The runtime representation of a class, it contains information
	about its members and functions. This class is used throughout
	the engine for inspecting components, serialization and scripting.

	An instance of type can be retrieved through Engine::MetaManager::TryGetType.
	*/
	class MetaType
	{
	public:
		/*
		Use this constructor only if your class has no C++ equivalent.
		*/
		MetaType(TypeInfo typeInfo, std::string_view name);

		template<typename TypeT>
		struct T {};

		template<typename BaseType>
		struct Base {};

		template<typename... Args>
		struct Ctor {};

		/*
		The main way of creating a type.

		Args can be a combination of MetaType::Base and MetaType::Ctor, See
		the example below.

		There is no limit to the number of base classes and constructors
		you can pass.

		A compile time error will be given if you've provided a class
		that is not a base or a constructor that does not exists.

		Example:
			struct Component {};

			struct TransformComponent : Component
			{
				TransformComponent(entt::entity owner);
				TransformComponent(entt::entity owner, glm::vec3 pos);
			};

			MetaType transformType(MetaType::T<TransformComponent>{},
								"Transform",
								MetaType::Base<Component>{},
								MetaType::Ctor<entt::entity>{},
								MetaType::Ctor<entt::entity, glm::vec3>{});

		Note:
			The following functions are automatically registered,
			because of how commonly they are used:
				- default constructor
				- copy constructor
				- copy assignment
				- move constructor
				- move assignment
				- destructor
		*/
		template<typename TypeT, typename ...Args>
		MetaType(T<TypeT>, std::string_view name, Args&& ... args);

		MetaType(MetaType&& other) noexcept;
		MetaType(const MetaType&) = delete;

		~MetaType();

		MetaType& operator=(const MetaType&) = delete;
		MetaType& operator=(MetaType&&) = delete;

		bool operator==(const MetaType& other) const { return GetTypeId() == other.GetTypeId(); }
		bool operator!=(const MetaType& other) const { return GetTypeId() != other.GetTypeId(); }
		auto operator<(const MetaType& other) const { return GetTypeId() < other.GetTypeId(); }

		TypeInfo GetTypeInfo() const { return mTypeInfo; }
		void SetTypeInfo(TypeInfo typeInfo) { mTypeInfo = typeInfo; }

		/*
		Example:
			typevec2.AddField(&vec2::x, "X");
			typevec2.AddField(&vec2::y, "Y");
			typevec2.AddField(&vec2::z, "Z");
		*/
		template<typename... Args>
		MetaField& AddField(Args&& ...args);

		/*
		Constructs a MetaFunc in place using the provided Args.

		Example:
			typeFloat.AddFunc(&sinf, "Sin");

			// For more examples, take a look at the comments documenting the MetaFunc class
		*/
		template<typename FuncPtr, typename... Args>
		MetaFunc& AddFunc(FuncPtr&& funcPtr, MetaFunc::NameOrTypeInit nameOrType, Args&& ...args);

		/*
		Add a baseclass. This function does not check if you ACTUALLY derive
		from the baseclass on the C++ side. When reflecting C++ classes,
		it is safer to pass a MetaType::Base<YourBaseClassHere> to the constructor
		of MetaType. See the documentation surrounding the constructor of MetaType.
		 */
		void AddBaseClass(const MetaType& baseClass);

		/*
		Add a baseclass, the base type will be reflected if it has not been already.
		Using this function requires including MetaManager.h.
		*/
		template<typename TypeT>
		void AddBaseClass();

		bool IsExactly(const uint32 typeId) const { return GetTypeId() == typeId; }

		template<typename TypeT>
		bool IsExactly() const { return IsExactly(MakeTypeId<TypeT>()); }

		bool IsDerivedFrom(TypeId baseClassTypeId) const;

		template<typename TypeT>
		bool IsDerivedFrom() const { return IsDerivedFrom(MakeTypeId<TypeT>()); }

		bool IsBaseClassOf(TypeId derivedClassTypeId) const;

		template<typename TypeT>
		bool IsBaseClassOf() const { return IsBaseClassOf(MakeTypeId<TypeT>()); }

		template<typename... Args>
		FuncResult Construct(Args&&... args) const;

		/*
		Will use placement new at the provided address.

		The address must point to a buffer of atleast MetaType::GetSize(). The address
		must be aligned to MetaType::GetAlignment().

		The MetaAny is non-owning; it is not responsible for freeing the buffer
		or calling the destructor, you need to do this yourself.
		*/
		template<typename... Args>
		FuncResult ConstructAt(void* atAdress, Args&&... args) const;

		// Calls the destructor.
		// Note that an owning MetaAny will call the destructor automatically;
		// This function is used by containers of MetaAny's, such as MetaArray,
		// to call the destructor of a non-owning MetaAny. You'll never
		// need to call this function manually unless you're making your own
		// custom container for metatypes.
		void Destruct(void* ptrToObjectOfThisType, bool freeBuffer) const;

		/*
		Returns each field, including those from baseclasses.
		*/
		std::vector<std::reference_wrapper<const MetaField>> EachField() const;

		/*
		Returns each field, including those from baseclasses.
		*/
		std::vector<std::reference_wrapper<MetaField>> EachField();

		/*
		Returns each function, including those from baseclasses
		*/
		std::vector<std::reference_wrapper<const MetaFunc>> EachFunc() const;

		/*
		Returns each function, including those from baseclasses
		*/
		std::vector<std::reference_wrapper<MetaFunc>> EachFunc();

		/*
		Get the base classes of this type. This function will not recurse, it will only return
		the classes that are directly a base.
		*/
		const std::vector<std::reference_wrapper<const MetaType>>& GetDirectBaseClasses() const { return mDirectBaseClasses; }

		/*
		Get the derived classes of this type. This function will not recurse, it will only return
		the classes that are directly a child.
		*/
		const std::vector<std::reference_wrapper<const MetaType>>& GetDirectDerivedClasses() const { return mDirectDerivedClasses; }

		/*
		Get the reflected fields of this type. This function is not recursive, and will not
		return the fields of any of it's baseclasses.
		*/
		std::vector<MetaField>& GetDirectFields() { return mFields; }

		/*
		Get the reflected fields of this type. This function is not recursive, and will not
		return the fields of any of it's baseclasses.
		*/
		const std::vector<MetaField>& GetDirectFields() const { return mFields; }

		/*
		Get the reflected functions of this type. This function is not recursive, and will not
		return the functions in any of it's baseclasses.
		*/
		std::vector<std::reference_wrapper<MetaFunc>> GetDirectFuncs();

		/*
		Get the reflected functions of this type. This function is not recursive, and will not
		return the functions in any of it's baseclasses.
		*/
		std::vector<std::reference_wrapper<const MetaFunc>>  GetDirectFuncs() const;

		const std::string& GetName() const { return mName; }
		uint32 GetTypeId() const { return mTypeInfo.mTypeId; }

		MetaField* TryGetField(Name name);
		const MetaField* TryGetField(Name name) const;

		MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType);
		const MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType) const;

		/*
		Will call the correct overload based on the provided arguments.

		Example:
			// The arguments can be a MetaAny:
			MetaAny booleanAny{ true };

			// Or they can be directly passed to the function
			type.CallFunction(OperatorType::equal, booleanAny, true);
		 */
		template<typename... Args>
		FuncResult CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Args&&... args) const;


		/*
		Will call the correct overload based on the provided arguments.
		Uses return value optimization to store the return value in the
		provided buffer.

		Example:
			// The arguments can be a MetaAny:
			MetaAny booleanAny{ true };

			// Or they can be directly passed to the function
			type.CallFunction(OperatorType::equal, booleanAny, true);
		 */
		template<typename... Args>
		FuncResult CallFunctionWithRVO(const std::variant<Name, OperatorType>& funcNameOrType, MetaFunc::RVOBuffer rvoBuffer, Args&&... args) const;

		/*
		Will call the correct overload based on the provided arguments. In general, the
		above templated function is easier to use and will likely suit all your needs.

		args.size() must be formOfArgs.size(). formOfArgs[N] represents how
		args[N] was passed in. This allows us to check if, for example, a const
		reference was passed to a function whose parameter is a mutable reference.
		 */
		FuncResult CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Span<MetaAny> args, Span<const TypeForm> formOfArgs, MetaFunc::RVOBuffer rvoBuffer = nullptr) const;

		/*
		Get a function with a specific set of parameters and return type. See MakeFuncId.
		*/
		MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, uint32 funcId);

		/*
		Get a function with a specific set of parameters and return type. See MakeFuncId.
		*/
		const MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, uint32 funcId) const;

		const MetaProps& GetProperties() const { ASSERT(mProperties != nullptr); return *mProperties; }
		MetaProps& GetProperties() { ASSERT(mProperties != nullptr); return *mProperties; }

		/*
		Removes all function with this name, if any exist. Returns the number of functions removed.
		*/
		size_t RemoveFunc(const std::variant<Name, OperatorType>& nameOrType);

		/*
		Removes all function with this name and func id, if any exist. Returns the number of functions removed.
		*/
		size_t RemoveFunc(const std::variant<Name, OperatorType>& nameOrType, uint32 funcId);

		/*
		Will allocate a buffer of the required size and alignment.
		*/
		void* Malloc(uint32 amountOfObjects = 1) const;
		static void Free(void* buffer);

		uint32 GetSize() const { return mTypeInfo.GetSize(); }
		uint32 GetAlignment() const { return mTypeInfo.GetAlign(); }

		bool IsConstructible(const std::vector<TypeTraits>& parameters) const { return TryGetConstructor(parameters) != nullptr; }
		bool IsDefaultConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsDefaultConstructible; }
		bool IsMoveConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsMoveConstructible; }
		bool IsCopyConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsCopyConstructible; }
		bool IsMoveAssignable() const { return mTypeInfo.mFlags & TypeInfo::IsMoveAssignable; }
		bool IsCopyAssignable() const { return mTypeInfo.mFlags & TypeInfo::IsCopyAssignable; }

		const MetaFunc* TryGetConstructor(const std::vector<TypeTraits>& parameters) const;
		const MetaFunc* TryGetDefaultConstructor() const;
		const MetaFunc* TryGetCopyConstructor() const;
		const MetaFunc* TryGetMoveConstructor() const;
		const MetaFunc* TryGetCopyAssign() const;
		const MetaFunc* TryGetMoveAssign() const;
		const MetaFunc& GetDestructor() const;

	private:
		template<typename FieldType, typename Self>
		static std::vector<std::reference_wrapper<FieldType>> EachField(Self& self);

		template<typename FuncType, typename Self>
		static std::vector<std::reference_wrapper<FuncType>> EachFunc(Self& self);

		template<typename FuncType, typename Self>
		static std::vector<std::reference_wrapper<FuncType>> GetDirectFuncs(Self& self);

		template<typename TypeT, typename BaseT>
		void AddFromArg(Base<BaseT>) { AddBaseClass<BaseT>(); }

		template<typename TypeT, typename... Args>
		void AddFromArg(Ctor<Args...>);

		template<typename... Args>
		FuncResult ConstructInternalGeneric(bool isOwner, void* address, Args&&... args) const;

		template<typename... Args>
		FuncResult ConstructInternal(bool isOwner, void* address, Args&&... args) const { return ConstructInternalGeneric(isOwner, address, std::forward<Args>(args)...); }

		FuncResult ConstructInternal(bool isOwner, void* address) const;

		template<typename TypeT>
		FuncResult ConstructInternal(bool isOwner, void* address, const TypeT& args) const;

		template<typename TypeT>
		FuncResult ConstructInternal(bool isOwner, void* address, TypeT& args) const;

		template<typename TypeT, std::enable_if_t<std::is_rvalue_reference_v<TypeT>, bool> = true>
		FuncResult ConstructInternal(bool isOwner, void* address, TypeT&& args) const;

		FuncResult ConstructInternal(bool isOwner, void* address, const MetaAny& args) const;

		FuncResult ConstructInternal(bool isOwner, void* address, MetaAny& args) const;

		FuncResult ConstructInternal(bool isOwner, void* address, MetaAny&& args) const;

		struct FuncKey
		{
			FuncKey() = default;
			FuncKey(Name::HashType hashType);
			FuncKey(OperatorType operatorType);
			FuncKey(const MetaFunc::NameOrType& nameOrType);
			FuncKey(const std::variant<Name, OperatorType>& nameOrType);
			FuncKey(const std::variant<std::string_view, OperatorType>& nameOrType);

			OperatorType mOperatorType{};
			Name::HashType mNameHash{};

			bool operator==(const FuncKey& other) const { return mOperatorType == other.mOperatorType && mNameHash == other.mNameHash; };
			bool operator!=(const FuncKey& other) const { return mOperatorType != other.mOperatorType || mNameHash != other.mNameHash; };
		};

		struct FuncHasher
		{
			uint64 operator()(const FuncKey& k) const
			{
				return (static_cast<uint64>(k.mOperatorType) << 32) | static_cast<uint64>(k.mNameHash);
			}
		};

		TypeInfo mTypeInfo{};

		std::unordered_multimap<FuncKey, MetaFunc, FuncHasher> mFunctions{};
		std::vector<MetaField> mFields{};

		// Mutable because this is double-linked; if we add a baseclass, we must add
		// ourselves to the baseclass.mDirectDerivedClasses, but we are not allowed
		// to modify the base name, properties, etc.
		mutable std::vector<std::reference_wrapper<const MetaType>> mDirectBaseClasses{};
		mutable std::vector<std::reference_wrapper<const MetaType>> mDirectDerivedClasses{};

		std::string mName;

		std::unique_ptr<MetaProps> mProperties;
	};

	template <typename TypeT, typename ... Args>
	MetaType::MetaType(T<TypeT>, const std::string_view name, Args&&... args) :
		MetaType(MakeTypeInfo<TypeT>(), name)
	{
		(
			[&]
			{
				AddFromArg<TypeT>(args);
			}
		(), ...);

		if constexpr (std::is_default_constructible_v<TypeT>)
		{
			AddFromArg<TypeT>(Ctor{});
			ASSERT(TryGetDefaultConstructor() != nullptr);
		}

		if constexpr (std::is_copy_constructible_v<TypeT>)
		{
			AddFromArg<TypeT>(Ctor<const TypeT&>{});
			ASSERT(TryGetCopyConstructor() != nullptr);
		}

		if constexpr (std::is_move_constructible_v<TypeT>)
		{
			AddFromArg<TypeT>(Ctor<TypeT&&>{});
			ASSERT(TryGetMoveConstructor() != nullptr);
		}

		if constexpr (std::is_move_assignable_v<TypeT>)
		{
			AddFunc(
				[](TypeT& lhs, TypeT&& rhs) -> TypeT&
				{
					lhs = std::move(rhs);
					return lhs;
				}, OperatorType::assign, MetaFunc::ExplicitParams<TypeT&, TypeT&&>{});
			ASSERT(TryGetMoveAssign() != nullptr);
		}

		if constexpr (std::is_copy_assignable_v<TypeT>)
		{
			AddFunc(
				[](TypeT& lhs, const TypeT& rhs) -> TypeT&
				{
					lhs = rhs;
					return lhs;
				}, OperatorType::assign, MetaFunc::ExplicitParams<TypeT&, const TypeT&>{});
			ASSERT(TryGetCopyAssign() != nullptr);
		}

		AddFunc(
			[](void* addr)
			{
				static_cast<TypeT*>(addr)->~TypeT();
			}, OperatorType::destructor, MetaFunc::ExplicitParams<void*>{});
	}

	template<typename FuncPtr, typename... Args>
	MetaFunc& MetaType::AddFunc(FuncPtr&& funcPtr, const MetaFunc::NameOrTypeInit nameOrType, Args&& ...args)
	{
		auto result = mFunctions.emplace(std::piecewise_construct,
			std::forward_as_tuple(nameOrType),
			std::forward_as_tuple(std::forward<FuncPtr>(funcPtr), nameOrType, std::forward<Args>(args)...));

		return result->second;
	}

	template<typename ...Args>
	MetaField& MetaType::AddField([[maybe_unused]] Args&& ... args)
	{
		MetaField& returnValue = mFields.emplace_back(*this, std::forward<Args>(args)...);

#ifdef ASSERTS_ENABLED
		auto it = std::find_if(mFields.begin(), mFields.end() - 1,
			[&returnValue](const MetaField& field)
			{
				return field.GetName() == returnValue.GetName();
			});
		 ASSERT_LOG(it == mFields.end() - 1, "{}::{} has already been added", GetName(), returnValue.GetName());
#endif // ASSERTS_ENABLED

		return returnValue;
	}

	template<typename TypeT, typename... Args>
	void MetaType::AddFromArg(Ctor<Args...>)
	{
		AddFunc(
			[](MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer buffer) -> FuncResult
			{
				ASSERT(buffer != nullptr && "The address provided to a constructor may never be nullptr");

				// Unpack needs to know the argument that it
				// needs to forward.
				[[maybe_unused]] size_t argIndex = args.size();

				new (buffer) TypeT(Internal::Unpack<Args>(args, argIndex)...);
				return std::nullopt;
			},
			OperatorType::constructor,
			MetaFunc::Return{ MakeTypeTraits<void>() },
			MetaFunc::Params{ MakeTypeTraits<Args>()... }
		);
	}

	template <typename ... Args>
	FuncResult MetaType::CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Args&&... args) const
	{
		return CallFunctionWithRVO(funcNameOrType, nullptr, std::forward<Args>(args)...);
	}

	template <typename ... Args>
	FuncResult MetaType::CallFunctionWithRVO(const std::variant<Name, OperatorType>& funcNameOrType,
		MetaFunc::RVOBuffer rvoBuffer, Args&&... args) const
	{
		static constexpr uint32 numOfArgs = sizeof...(Args);
		std::pair<std::array<MetaAny, numOfArgs>, std::array<TypeForm, numOfArgs>> packedArgs = MetaFunc::Pack(std::forward<Args>(args)...);
		return CallFunction(funcNameOrType, Span<MetaAny>{packedArgs.first}, Span<const TypeForm>{packedArgs.second}, rvoBuffer);
	}

	template <typename ... Args>
	FuncResult MetaType::Construct(Args&&... args) const
	{
		void* buffer = Malloc();

		FuncResult constructResult = ConstructInternal(true, buffer, std::forward<Args>(args)...);

		if (constructResult.HasError())
		{
			Free(buffer);
		}
		return constructResult;
	}

	template <typename ... Args>
	FuncResult MetaType::ConstructAt(void* atAddress, Args&&... args) const
	{
		ASSERT(atAddress != nullptr);
		ASSERT(reinterpret_cast<uintptr>(atAddress) % GetAlignment() == 0 && "Address was not aligned");

		return ConstructInternal(false, atAddress, std::forward<Args>(args)...);
	}

	template <typename ... Args>
	FuncResult MetaType::ConstructInternalGeneric(bool isOwner, void* address, Args&&... args) const
	{
		FuncResult result = CallFunctionWithRVO(OperatorType::constructor, address, std::forward<Args>(args)...);

		if (result.HasError())
		{
			return result;
		}

		return MetaAny{ *this, address, isOwner };
	}

	template <typename TypeT>
	FuncResult MetaType::ConstructInternal(bool isOwner, void* address, const TypeT& args) const
	{
		if (MakeTypeId<TypeT>() != GetTypeId())
		{
			return ConstructInternalGeneric(isOwner, address, args);
		}

		if constexpr (std::is_copy_constructible_v<TypeT>)
		{
			return MetaAny{ *this, new (address) TypeT(args), isOwner };
		}
		return { "Type is not copy-constructible" };
	}

	template <typename TypeT>
	FuncResult MetaType::ConstructInternal(bool isOwner, void* address, TypeT& args) const
	{
		if (MakeTypeId<TypeT>() != GetTypeId())
		{
			return ConstructInternalGeneric(isOwner, address, args);
		}

		if constexpr (std::is_copy_constructible_v<TypeT>)
		{
			return MetaAny{ *this, new (address) TypeT(args), isOwner };
		}
		return { "Type is not copy-constructible" };
	}

	template <typename TypeT, std::enable_if_t<std::is_rvalue_reference_v<TypeT>, bool>>
	FuncResult MetaType::ConstructInternal(bool isOwner, void* address, TypeT&& args) const 
	{
		if (MakeTypeId<TypeT>() != GetTypeId())
		{
			return ConstructInternalGeneric(isOwner, address, std::move(args));
		}

		if constexpr (std::is_move_constructible_v<TypeT>)
		{
			return MetaAny{ *this, new (address) TypeT(std::move(args)), isOwner };
		}
		return { "Type is not copy-constructible" };
	}

	template<typename TypeT>
	void MetaType::AddBaseClass()
	{
		const MetaType& base = MetaManager::Get().GetType<TypeT>();
		AddBaseClass(base);
	}
}
