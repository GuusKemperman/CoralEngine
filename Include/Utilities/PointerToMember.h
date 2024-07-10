#pragma once

namespace Engine
{
	// Internal details, can be ignored
	namespace Internal
	{
		template <class T>
		struct MemberTypeHelper;

		template <class C, class T>
		struct MemberTypeHelper<T C::*> { using type = T; };

		template <class T>
		struct MemberType
			: MemberTypeHelper<typename std::remove_cvref<T>::type> {};

		template <class T>
		struct ClassTypeHelper;

		template <class C, class T>
		struct ClassTypeHelper<T C::*> { using type = C; };

		template <class T>
		struct ClassType
			: ClassTypeHelper<typename std::remove_cvref<T>::type> {};
	}

	// --------------------------------- //
	//				API					 //
	// --------------------------------- //

	// Can be used to get the type of a member, based on a pointer to a member
	// E.g., static_assert(std::same_v<float, MemberType_t<&glm::vec3::x>>);
	template <class PtrToMember>
	using MemberType_t = Internal::MemberType<PtrToMember>::type;

	// Can be used to get the class that a pointer to member can be found in
	// E.g., static_assert(std::same_v<glm::vec3, MemberType_t<&glm::vec3::x>>);
	template <class PtrToMember>
	using ClassType_t = Internal::ClassType<PtrToMember>::type;

	template<auto PointerToMember>
	uintptr GetOffset()
	{
		using Class = ClassType_t<decltype(PointerToMember)>;
		
		// We could also do it a null address, but this function violates
		// enough undefined behaviour with its reinterpret_casts as it is
		alignas(Class) const char bytes[sizeof(Class)]{};
		const Class* objectInstance = reinterpret_cast<const Class*>(bytes);

		auto memberPtr = PointerToMember;
		const void* ptrToThing = &((objectInstance)->*memberPtr);

		return reinterpret_cast<uintptr>(ptrToThing) - reinterpret_cast<uintptr>(objectInstance);
	}
}