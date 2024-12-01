#include "Precomp.h"
#include "Scripting/Compiler/IRBuilder.h"

#include <source_location>

#include "Meta/Impl/MetaPropsImpl.h"

namespace
{
	// Walks up the tree in reverse, only allowing access to
	// nodes that are within scope
	struct ScopeIterator
	{
		using Container = std::vector<std::unique_ptr<CE::IRNode>>;
		using UnderlyingIt = Container::reverse_iterator;

		ScopeIterator(UnderlyingIt&& it, Container& container) :
			mContainer(container),
			mIt(std::move(it))
		{
		}

		decltype(auto) operator*() const { return mIt.operator*(); }
		decltype(auto) operator->() const { return mIt.operator->(); }

		void IncrementUntilValid()
		{
			int32 balance{};

			while (true)
			{
				if (mIt == mContainer.get().rend())
				{
					return;
				}

				CE::IRNodeType type = mIt->get()->mType;
				balance += type == CE::IRNodeType::ScopePop;
				balance -= type == CE::IRNodeType::ScopePush;

				if (balance == 0)
				{
					if (type == CE::IRNodeType::ScopePush)
					{
						// Always return one past the ScopePush
						++mIt;
					}

					return;
				}

				++mIt;
			}
		}

		// Prefix increment
		ScopeIterator& operator++()
		{
			++mIt;
			IncrementUntilValid();
			return *this;
		}

		// Postfix increment
		ScopeIterator operator++(int) { ScopeIterator tmp = *this; ++(*this);	return tmp; }

		constexpr bool operator==(const ScopeIterator& b) const { return mIt == b.mIt; }
		constexpr bool operator!=(const ScopeIterator& b) const { return mIt != b.mIt; }

	private:
		std::reference_wrapper<Container> mContainer;
		UnderlyingIt mIt{};
	};

	ScopeIterator ScopeIterationRBegin(ScopeIterator::Container& container)
	{
		ScopeIterator begin{ container.rbegin(), container };
		begin.IncrementUntilValid();
		return begin;
	}

	ScopeIterator ScopeIterationREnd(ScopeIterator::Container& container)
	{
		return ScopeIterator{ container.rend(), container };
	}

	std::string GetForConditionLabel(std::string_view forName)
	{
		return CE::Format("__{}_internal_condition_label", forName);
	}

	std::string GetForUpdateLabel(std::string_view forName)
	{
		return CE::Format("__{}_internal_update_label", forName);
	}

	std::string GetForBodyLabel(std::string_view forName)
	{
		return CE::Format("__{}_internal_body_label", forName);
	}

	std::string GetLoopEndLabel(std::string_view forName)
	{
		return CE::Format("__{}_internal_end_label", forName);
	}
}

CE::IRString::IRString(ManyStrings& storage, std::string_view string) :
	mStorage(storage),
	mIndex(storage.NumOfStrings())
{
	storage.Emplace(string);
}

CE::IRString::operator std::string_view() const
{
	return mStorage.get()[mIndex];
}

CE::IRBuilder& CE::IRBuilder::Class(const IRSource& source, std::string_view name)
{
	mClasses.emplace_back(std::move(source), MakeString(name), std::make_unique<MetaProps>());
	return *this;
}

CE::IRBuilder& CE::IRBuilder::Field(const IRSource& source, 
	std::string_view typeName,
	TypeForm typeForm,
	std::string_view name,
	std::optional<std::string_view> defaultValue)
{
	IRClass* irClass = GetClassOrRaiseError(source);

	if (irClass == nullptr)
	{
		return *this;
	}

	irClass->mFields.emplace_back(source,
		MakeString(typeName),
		typeForm,
		MakeString(name),
		std::make_unique<MetaProps>(),
		defaultValue.has_value() ? std::optional{ MakeString(*defaultValue) } : std::nullopt);

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Function(const IRSource& source, std::string_view name)
{
	IRClass* irClass = GetClassOrRaiseError(source);

	if (irClass == nullptr)
	{
		return *this;
	}

	irClass->mFuncs.emplace_back(source,
		MakeString(name),
		std::make_unique<MetaProps>());

	return *this;
}

CE::IRBuilder& CE::IRBuilder::FunctionParameter(const IRSource& source,
	std::string_view typeName,
	TypeForm typeForm,
	std::string_view name, 
	std::optional<std::string_view> defaultValue)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mParameters.emplace_back(source,
		MakeString(typeName),
		typeForm,
		MakeString(name),
		defaultValue.has_value() ? std::optional{ MakeString(*defaultValue) } : std::nullopt);

	return *this;
}

CE::IRBuilder& CE::IRBuilder::FunctionReturn(const IRSource& source, 
	std::string_view typeName, 
	TypeForm typeForm, 
	std::string_view name)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mReturns.emplace_back(source,
		MakeString(typeName),
		typeForm,
		MakeString(name));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::DeclareVariable(const IRSource& source, 
	std::string_view typeName,
	TypeForm typeForm,
	std::string_view name, 
	std::optional<std::string_view> defaultValue)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRDeclarationNode>(
		IRNode{ source, IRNodeType::Declaration, },
		MakeString(typeName),
		typeForm,
		MakeString(name),
		defaultValue.has_value() ? std::optional{ MakeString(*defaultValue) } : std::nullopt));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Scope(const IRSource& source)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRScopePushNode>(
		IRNode{ source, IRNodeType::ScopePush, }));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::EndScope(const IRSource& source)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRScopePushNode>(
		IRNode{ source, IRNodeType::ScopePop, }));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Invoke(const IRSource& source, std::string_view typeName, std::string_view funcName)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRInvocationNode>(
		IRNode{ source, IRNodeType::Invocation, },
		MakeString(typeName),
		MakeString(funcName)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::InvokeArgument(const IRSource& source, 
	TypeForm forwardAs, 
	std::string_view argumentName)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr
		|| irFunc->mNodes.empty()
		|| irFunc->mNodes.back()->mType != IRNodeType::Invocation)
	{
		Error(source, Format("Could not pass {} non-invocable object", argumentName));
		return *this;
	}

	IRInvocationNode& irInvokeNode = static_cast<IRInvocationNode&>(*irFunc->mNodes.back());
	irInvokeNode.mArguments.emplace_back(MakeString(argumentName), forwardAs);
	return *this;
}

CE::IRBuilder& CE::IRBuilder::InvokeResult(const IRSource& source, 
	TypeForm forwardAs,
	std::string_view resultName)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr
		|| irFunc->mNodes.empty()
		|| irFunc->mNodes.back()->mType != IRNodeType::Invocation)
	{
		Error(source, Format("Could not store {} from non-invocable object", resultName));
		return *this;
	}

	IRInvocationNode& irInvokeNode = static_cast<IRInvocationNode&>(*irFunc->mNodes.back());
	irInvokeNode.mReturnValues.emplace_back(MakeString(resultName), forwardAs);
	return *this;
}

CE::IRBuilder& CE::IRBuilder::GetField(const IRSource& source,
	std::string_view target,
	std::string_view fieldName,
	std::string_view destinationVariable)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRGetField>(
		IRNode{ source, IRNodeType::GetField, },
		MakeString(target),
		MakeString(fieldName),
		MakeString(destinationVariable)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::SetField(const IRSource& source,
	std::string_view target,
	std::string_view fieldName,
	std::string_view sourceVariable)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRSetField>(
		IRNode{ source, IRNodeType::SetField, },
		MakeString(target),
		MakeString(fieldName),
		MakeString(sourceVariable)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::If(const IRSource& source, std::string_view conditionVariable)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRBranchNode>(
		IRNode{ source, IRNodeType::Branch, },
		MakeString(conditionVariable)));

	Scope(source);

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Else(const IRSource& source)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	auto matchingBranch = std::find_if(ScopeIterationRBegin(irFunc->mNodes), ScopeIterationREnd(irFunc->mNodes),
		[](std::unique_ptr<IRNode>& node)
		{
			return node->mType == IRNodeType::Branch;
		});

	if (matchingBranch == ScopeIterationREnd(irFunc->mNodes))
	{
		Error(source, Format("Cannot use else statement without corresponding if statement"));
		return *this;
	}

	IRBranchNode& irBranchNode = static_cast<IRBranchNode&>(**matchingBranch);

	if (irBranchNode.mFalseLabel.has_value())
	{
		Error(source, Format("Cannot use multiple else statements in a single if statement"));
		return *this;
	}

	std::string labelName = Format("__internal_branch_true_label_{}", irFunc->mNodes.size());
	irBranchNode.mFalseLabel = MakeString(labelName);

	EndScope(source);
	Label(source, labelName);
	Scope(source);

	return *this;
}

CE::IRBuilder& CE::IRBuilder::EndIf(const IRSource& source)
{
	EndScope(source);
	return *this;
}

CE::IRBuilder& CE::IRBuilder::GoTo(const IRSource& source, std::string_view label)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRGoToNode>(
		IRNode{ source, IRNodeType::GoTo, },
		MakeString(label)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Label(const IRSource& source, std::string_view label)
{
	IRFunc* irFunc = GetFuncOrRaiseError(source);

	if (irFunc == nullptr)
	{
		return *this;
	}

	irFunc->mNodes.emplace_back(std::make_unique<IRLabelNode>(
		IRNode{ source, IRNodeType::Label, },
		MakeString(label)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::While(const IRSource& source, std::string_view loopName, std::string_view condition)
{
	Label(source, loopName);
	If(source, condition);
	return *this;
}


CE::IRBuilder& CE::IRBuilder::EndWhile(const IRSource& source, std::string_view loopName)
{
	GoTo(source, loopName);
	Else(source);
	Break(source, loopName);
	EndIf(source);

	Label(source, MakeString(GetLoopEndLabel(loopName)));
	return *this;
}

CE::IRBuilder& CE::IRBuilder::For(const IRSource& source, std::string_view loopName)
{
	Scope(source);
	return *this;
}

CE::IRBuilder& CE::IRBuilder::ForCondition(const IRSource& source, std::string_view loopName)
{
	Scope(source);
	Label(source, MakeString(GetForConditionLabel(loopName)));
	Scope(source);
	return *this;
}

CE::IRBuilder& CE::IRBuilder::ForUpdate(const IRSource& source, std::string_view loopName)
{
	// Check the condition
	If(source, loopName);
	GoTo(source, MakeString(GetForBodyLabel(loopName)));
	Else(source);
	Break(source, loopName);
	EndScope(source);

	Label(source, MakeString(GetForUpdateLabel(loopName)));
	Scope(source);

	return *this;

}

CE::IRBuilder& CE::IRBuilder::ForBody(const IRSource& source, std::string_view loopName)
{
	EndScope(source);
	Label(source, MakeString(GetForBodyLabel(loopName)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::ForEnd(const IRSource& source, std::string_view loopName)
{
	GoTo(source, MakeString(GetForUpdateLabel(loopName)));
	GoTo(source, MakeString(GetForConditionLabel(loopName)));

	EndScope(source);

	Label(source, MakeString(GetLoopEndLabel(loopName)));

	return *this;
}

CE::IRBuilder& CE::IRBuilder::Break(const IRSource& source, std::string_view loopName)
{
	GoTo(source, MakeString(GetLoopEndLabel(loopName)));
	return *this;
}

CE::IRBuilder& CE::IRBuilder::Error(const IRSource& source, std::string_view message)
{
	mErrors.emplace_back(std::move(source), MakeString(message));
	return *this;
}

CE::IRClass* CE::IRBuilder::GetClassOrRaiseError(const IRSource& source)
{
	if (mClasses.empty())
	{
		Error(source, "Not inside class body");
		return nullptr;
	}
	return &mClasses.back();
}

CE::IRFunc* CE::IRBuilder::GetFuncOrRaiseError(const IRSource& source)
{
	IRClass* irClass = GetClassOrRaiseError(source);

	if (irClass == nullptr
		|| irClass->mFuncs.empty())
	{
		Error(source, "Not inside function");
		return nullptr;
	}
	return &irClass->mFuncs.back();
}

CE::IRString CE::IRBuilder::MakeString(std::string_view str)
{
	return { mStringsStorage, str };
}