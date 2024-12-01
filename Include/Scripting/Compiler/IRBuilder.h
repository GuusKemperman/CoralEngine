#pragma once
#include "Utilities/ManyStrings.h"
#include "SourceInfo.h"
#include "Meta/Fwd/MetaTypeTraitsFwd.h"

namespace CE
{
	class MetaProps;

	enum class IRNodeType
	{
		Declaration,
		Invocation,
		GetField,
		SetField,
		Branch,
		ScopePush,
		ScopePop,
		GoTo,
		Label,
	};

	struct IRString
	{
		IRString(ManyStrings& storage, std::string_view string);

		operator std::string_view() const;

		std::reference_wrapper<const ManyStrings> mStorage;
		size_t mIndex{};
	};

	struct IRNode
	{
		IRSource mSource{};
		IRNodeType mType{};
	};

	struct IRDeclarationNode : IRNode
	{
		IRString mTypeName;
		TypeForm mTypeForm{};
		IRString mName;

		std::optional<IRString> mDefaultValue{};
	};

	struct IRArgument
	{
		IRString mVariableName;
		TypeForm mTypeForm{};
	};

	struct IRInvocationNode : IRNode 
	{
		// The type that holds the func
		IRString mTypeName;
		IRString mFuncName;

		std::vector<IRArgument> mArguments{};
		std::vector<IRArgument> mReturnValues{};
	};

	struct IRGetField : IRNode
	{
		IRString mTarget;
		IRString mFieldName;
		IRString mDestinationVariable;
	};

	struct IRSetField : IRNode
	{
		IRString mTarget;
		IRString mFieldName;
		IRString mSourceVariable;
	};

	struct IRBranchNode : IRNode 
	{
		IRString mCondition;
		std::optional<IRString> mFalseLabel;
	};

	struct IRScopePushNode : IRNode {};
	struct IRScopePopNode : IRNode {};

	struct IRGoToNode : IRNode 
	{
		IRString mLabel;
	};

	struct IRLabelNode : IRNode 
	{
		IRString mName;
	};

	struct IRParameter
	{
		IRSource mSource{};
		IRString mTypeName;
		TypeForm mForm{}; 
		IRString mName;

		std::optional<IRString> mDefaultValue{};
	};

	struct IRFunc
	{
		IRSource mSource{};
		IRString mName;
		std::unique_ptr<MetaProps> mProps;

		std::vector<IRParameter> mParameters{};
		std::vector<IRParameter> mReturns{};

		std::vector<std::unique_ptr<IRNode>> mNodes{};
	};

	struct IRField
	{
		IRSource mSource{};
		IRString mTypeName;
		TypeForm mTypeForm;
		IRString mName;
		std::unique_ptr<MetaProps> mProps;

		std::optional<IRString> mDefaultValue{};
	};

	struct IRClass
	{
		IRSource mSource{};
		IRString mName;
		std::unique_ptr<MetaProps> mProps;

		std::vector<IRField> mFields{};
		std::vector<IRFunc> mFuncs{};
	};
	
	struct IRError
	{
		IRSource mSource{};
		IRString mMessage;
	};

	class IRBuilder
	{
	public:
		static constexpr std::string_view sThis = "this";

		IRBuilder& Class(const IRSource& source, std::string_view name);

		IRBuilder& Field(const IRSource& source, 
			std::string_view typeName,
			TypeForm typeForm,
			std::string_view name, 
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& Function(const IRSource& source, std::string_view name);

		IRBuilder& FunctionParameter(const IRSource& source, 
			std::string_view typeName,
			TypeForm typeForm,
			std::string_view name,
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& FunctionReturn(const IRSource& source,
			std::string_view typeName, 
			TypeForm typeForm,
			std::string_view name);

		IRBuilder& DeclareVariable(const IRSource& source, 
			std::string_view typeName,
			TypeForm typeForm,
			std::string_view name, 
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& Scope(const IRSource& source);
		IRBuilder& EndScope(const IRSource& source);

		IRBuilder& Invoke(const IRSource& source, std::string_view typeName, std::string_view funcName);

		IRBuilder& InvokeArgument(const IRSource& source, TypeForm forwardAs, std::string_view argumentName);

		IRBuilder& InvokeResult(const IRSource& source, TypeForm forwardAs, std::string_view resultName);

		IRBuilder& GetField(const IRSource& source, 
			std::string_view target, 
			std::string_view fieldName,
			std::string_view destinationVariable);

		IRBuilder& SetField(const IRSource& source,
			std::string_view target,
			std::string_view fieldName,
			std::string_view sourceVariable);

		IRBuilder& If(const IRSource& source, std::string_view conditionVariable);

		IRBuilder& Else(const IRSource& source);

		IRBuilder& EndIf(const IRSource& source);

		IRBuilder& GoTo(const IRSource& source, std::string_view label);

		IRBuilder& Label(const IRSource& source, std::string_view label);

		IRBuilder& While(const IRSource& source, std::string_view loopName, std::string_view condition);

		IRBuilder& EndWhile(const IRSource& source, std::string_view loopName);

		IRBuilder& For(const IRSource& source, std::string_view loopName);

		IRBuilder& ForCondition(const IRSource& source, std::string_view loopName);

		IRBuilder& ForUpdate(const IRSource& source, std::string_view loopName);

		IRBuilder& ForBody(const IRSource& source, std::string_view loopName);

		IRBuilder& ForEnd(const IRSource& source, std::string_view loopName);

		IRBuilder& Break(const IRSource& source, std::string_view loopName);

		IRBuilder& Error(const IRSource& source, std::string_view message);

	private:
		IRClass* GetClassOrRaiseError(const IRSource& source);
		IRFunc* GetFuncOrRaiseError(const IRSource& source);

		IRString MakeString(std::string_view str);

		ManyStrings mStringsStorage{};
		std::vector<IRClass> mClasses{};
		std::vector<IRError> mErrors{};
	};
}
