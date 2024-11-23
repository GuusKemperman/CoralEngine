#pragma once
#include "Utilities/ManyStrings.h"

namespace CE
{
	class MetaProps;

	enum class IRNodeType
	{
		Declaration,
		Invocation,
		Branch,
		ScopePush,
		ScopePop,
		GoTo,
		Label,
	};

	using Source = void*;

	struct IRNode
	{
		Source mSource{};
		IRNodeType mType{};
	};

	struct IRDeclarationNode : IRNode
	{
		std::string_view mTypeName{};
		std::string_view mName{};

		std::optional<std::string_view> mDefaultValue{};
	};

	struct IRInvocationNode : IRNode 
	{
		// The type that holds the func
		std::string_view mTypeName{};
		std::string_view mFuncName{};

		std::vector<std::string_view> mArguments{};
		std::vector<std::string_view> mReturnValues{};
	};

	struct IRBranchNode : IRNode 
	{
		std::string_view mCondition{};
		std::string_view mTrueLabel{};
		std::string_view mFalseLabel{};
	};

	struct IRScopePushNode : IRNode {};
	struct IRScopePopNode : IRNode {};

	struct IRGoToNode : IRNode 
	{
		std::string_view mLabel{};
	};

	struct IRLabelNode : IRNode 
	{
		std::string_view mName{};
	};

	struct IRParameter
	{
		Source mSource{};
		std::string_view mName{};
		std::string_view mTypeName{};

		std::optional<std::string_view> mDefaultValue{};
	};

	struct IRFunc
	{
		Source mSource{};
		std::string_view mName{};
		std::string_view mTypeName{};

		std::vector<std::unique_ptr<IRNode>> mNodes{};
		std::unique_ptr<MetaProps> mProps;
	};

	struct IRField
	{
		Source mSource{};
		std::string_view mName{};
		std::string_view mTypeName{};

		std::optional<std::string_view> mDefaultValue{};
		std::unique_ptr<MetaProps> mProps;
	};

	struct IRClass
	{
		std::string_view mName{};

		std::vector<IRField> mFields{};
		std::vector<IRFunc> mFuncs{};

		std::unique_ptr<MetaProps> mProps;
	};
	
	class IRBuilder
	{
	public:
		IRBuilder& Class(Source source, std::string_view name);

		IRBuilder& Field(Source source, std::string_view typeName, std::string_view name);
		IRBuilder& Field(Source source, std::string_view typeName, std::string_view name, std::string_view defaultValue);

		IRBuilder& Function(Source source, std::string_view name);

		IRBuilder& FunctionParameter(Source source, std::string_view typeName, std::string_view name);
		IRBuilder& FunctionParameter(Source source, std::string_view typeName, std::string_view name, std::string_view defaultValue);

		IRBuilder& FunctionReturn(Source source, std::string_view typeName);

		IRBuilder& DeclareVariable(Source source, std::string_view typeName, std::string_view name);
		IRBuilder& DeclareVariable(Source source, std::string_view typeName, std::string_view name, std::string_view defaultValue);

		IRBuilder& Scope(Source source);
		IRBuilder& EndScope(Source source);

		IRBuilder& Invoke(Source source, std::string_view typeName, std::string_view funcName);

		IRBuilder& InvokeArgument(Source source, std::string_view argumentName);

		IRBuilder& InvokeResult(Source source, std::string_view resultName);
		
		IRBuilder& If(Source source, std::string_view conditionVariable);

		IRBuilder& Else(Source source);

		IRBuilder& EndIf(Source source);

		IRBuilder& GoTo(Source source, std::string_view label);

		IRBuilder& Label(Source source, std::string_view label);

		IRBuilder& While(Source source, std::string_view name, std::string_view condition)
		{
			Label(source, "UniqueWhileName");
			If(source, condition);
		}

		IRBuilder& EndWhile(Source source);

		IRBuilder& For(Source source, std::string_view name);

		IRBuilder& ForCondition(Source source);

		IRBuilder& ForUpdate(Source source);

		IRBuilder& ForBody(Source source);

		IRBuilder& ForEnd(Source source);

		IRBuilder& Break(Source source, std::string_view from);

		std::list<IRClass> mClasses{};
		CE::ManyStrings mStringsStorage{};
	};
}