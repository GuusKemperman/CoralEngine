#pragma once
#include "Utilities/ManyStrings.h"

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

	using IRSource = void*;

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
		IRString mName;

		std::optional<IRString> mDefaultValue{};
	};

	struct IRInvocationNode : IRNode 
	{
		// The type that holds the func
		IRString mTypeName;
		IRString mFuncName;

		std::vector<IRString> mArguments{};
		std::vector<IRString> mReturnValues{};
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
		IRBuilder& Class(IRSource source, std::string_view name);

		IRBuilder& Field(IRSource source, 
			std::string_view typeName, 
			std::string_view name, 
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& Function(IRSource source, std::string_view name);

		IRBuilder& FunctionParameter(IRSource source, 
			std::string_view typeName, 
			std::string_view name, 
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& FunctionReturn(IRSource source, std::string_view typeName, std::string_view name);

		IRBuilder& DeclareVariable(IRSource source, 
			std::string_view typeName, 
			std::string_view name, 
			std::optional<std::string_view> defaultValue = std::nullopt);

		IRBuilder& Scope(IRSource source);
		IRBuilder& EndScope(IRSource source);

		IRBuilder& Invoke(IRSource source, std::string_view typeName, std::string_view funcName);

		IRBuilder& InvokeArgument(IRSource source, std::string_view argumentName);

		IRBuilder& InvokeResult(IRSource source, std::string_view resultName);

		IRBuilder& GetField(IRSource source, 
			std::string_view target, 
			std::string_view fieldName,
			std::string_view destinationVariable);

		IRBuilder& SetField(IRSource source,
			std::string_view target,
			std::string_view fieldName,
			std::string_view sourceVariable);

		IRBuilder& If(IRSource source, std::string_view conditionVariable);

		IRBuilder& Else(IRSource source);

		IRBuilder& EndIf(IRSource source);

		IRBuilder& GoTo(IRSource source, std::string_view label);

		IRBuilder& Label(IRSource source, std::string_view label);

		IRBuilder& While(IRSource source, std::string_view loopName, std::string_view condition);

		IRBuilder& EndWhile(IRSource source, std::string_view loopName);

		IRBuilder& For(IRSource source, std::string_view loopName);

		IRBuilder& ForCondition(IRSource source, std::string_view loopName);

		IRBuilder& ForUpdate(IRSource source, std::string_view loopName);

		IRBuilder& ForBody(IRSource source, std::string_view loopName);

		IRBuilder& ForEnd(IRSource source, std::string_view loopName);

		IRBuilder& Break(IRSource source, std::string_view loopName);

		IRBuilder& Error(IRSource source, std::string_view message);

	private:
		IRClass* GetClassOrRaiseError(IRSource source);
		IRFunc* GetFuncOrRaiseError(IRSource source);

		IRString MakeString(std::string_view str);

		ManyStrings mStringsStorage{};
		std::vector<IRClass> mClasses{};
		std::vector<IRError> mErrors{};
	};
}
