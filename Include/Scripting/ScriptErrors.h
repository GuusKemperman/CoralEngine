#pragma once
#include "Utilities/EnumString.h"
#include "Scripting/ScriptIds.h"
#include "Scripting/ScriptConfig.h"

namespace Engine
{
	class Script;
	class ScriptField;
	class ScriptFunc;
	class ScriptPin;
	class ScriptLink;
	class ScriptNode;

	struct ScriptLocation
	{
		ScriptLocation(const Script& script);
		ScriptLocation(const ScriptFunc& func);
		ScriptLocation(const ScriptField& field);
		ScriptLocation(const ScriptFunc& func, const ScriptNode& node);
		ScriptLocation(const ScriptFunc& func, const ScriptLink& link);
		ScriptLocation(const ScriptFunc& func, const ScriptPin& pin);

#ifdef EDITOR
		void NavigateToLocation() const;
#endif // EDITOR

		std::string ToString() const;

		bool IsLocationEqualToOrInsideOf(const ScriptLocation& other) const;

		enum class Type
		{
			Script,
			Func,
			Field,
			Node,
			Pin,
			Link
		};
		Type mType{};
		std::string mNameOfScript{};
		std::string mFieldOrFuncName{};

		std::variant<std::monostate, NodeId, LinkId, std::pair<PinId, NodeId>> mId{};
	};

	class ScriptError
	{
	public:
		// The entire error message can be found at the bottom of this file.
		// Might make it more clear what each error entails.
		enum Type
		{
			UnreflectedType,
			TypeCannotBeMember,
			TypeCannotBeReferencedFromScripts,
			TypeCannotBeOwnedByScripts,
			NameNotUnique,
			NodeOutOfDate,
			UnderlyingFuncNoLongerExists,
			CompilerBug,
			LinkNotAllowed,
			NotPossibleToEnterFunc,
			ExecutionTimeOut,
			FunctionCallFailed,
			ValueWasNull,
			StackOverflow
		};

		ScriptError(Type type,
			ScriptLocation&& originOfError,
			const std::optional<std::string_view>& additionalInfo = std::nullopt) :
			mType(type),
			mAdditionalInfo(additionalInfo.has_value() ? std::optional<std::string>{*additionalInfo} : std::nullopt ),
			mOrigin(std::move(originOfError)) {}

		std::string ToString(bool includeLocationInString) const;
		const ScriptLocation& GetOrigin() const { return mOrigin; }

	private:
		Type mType{};
		std::optional<std::string> mAdditionalInfo{};
		ScriptLocation mOrigin;
	};

	using ScriptErrorInserter = std::back_insert_iterator<std::vector<ScriptError>>;

	template<>
	struct EnumStringPairsImpl<ScriptError::Type>
	{
		static constexpr EnumStringPairs<ScriptError::Type, 137> value = {
			EnumStringPair<ScriptError::Type>{ ScriptError::Type::UnreflectedType, "UnreflectedType" },
			{ ScriptError::Type::TypeCannotBeMember, "TypeCannotBeMember" },
			{ ScriptError::Type::TypeCannotBeReferencedFromScripts, "This type can no longer be used by scripts" },
			{ ScriptError::Type::TypeCannotBeOwnedByScripts, "This type cannot be owned by scripts, and can maybe not even be referenced by scripts at all." },
			{ ScriptError::Type::NameNotUnique, "NameNotUnique - Classes, functions and fields must have a unique name" },
#ifdef REMOVE_FROM_SCRIPTS_ENABLED
			{ ScriptError::Type::NodeOutOfDate, "NodeOutOfDate - The underlying function now has different parameters or a different return type" },
#else
			{ ScriptError::Type::NodeOutOfDate, "NodeOutOfDate in a no-editor build - The underlying function has different parameters or a different return type, and is not refreshed. Resaving the asset will resolve the issue." },
#endif
			{ ScriptError::Type::UnderlyingFuncNoLongerExists, "The underlying function no longer exists" },
			{ ScriptError::Type::CompilerBug, "CompilerBug - An error in the compiler, or front-end that was 100% not caused by a designer. " },
			{ ScriptError::Type::LinkNotAllowed, "LinkNotAllowed - A link between two pins is invalid" },
			{ ScriptError::Type::NotPossibleToEnterFunc, "NotPossibleToEnterFunc -  There is something wrong with the entry/return nodes, there may be too many or too little of them for example" },
			{ ScriptError::Type::ExecutionTimeOut, "Execution time-out, Possibly an infinite loop. If this is not an error, try raising the maximum number of nodes that can be executed(or ask a programmer to raise it) in VirtualMachine.h" },
			{ ScriptError::Type::FunctionCallFailed, "Function call failed" },
			{ ScriptError::Type::ValueWasNull, "Value was null" },
			{ ScriptError::Type::StackOverflow, "Stack overflow - Possibly an infinite loop" },
		};
	};
}

