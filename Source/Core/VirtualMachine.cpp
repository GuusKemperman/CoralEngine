#include "Precomp.h"
#include "Core/VirtualMachine.h"

#include "Core/AssetManager.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "Scripting/ScriptTools.h"
#include "Scripting/Nodes/MetaFuncScriptNode.h"
#include "Scripting/Nodes/MetaFieldScriptNode.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/Nodes/ControlScriptNodes.h"
#include "Assets/Script.h"
#include "Meta/MetaTools.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Scripting/ScriptConfig.h"
#include "Utilities/Time.h"

void CE::VirtualMachine::PostConstruct()
{
	Recompile();
}

CE::VirtualMachine::~VirtualMachine()
{
	// I mean the meta manager will be destroyed right after us, so *technically* we can skip this step
	// But it's still polite to clean up our own mess
	DestroyAllTypesCreatedThroughScripts();

#ifdef SCRIPT_PROFILING
	std::vector<std::pair<std::string, float>> sortedTimesSpent{};
	sortedTimesSpent.reserve(mNumOfSecondsSpentEachFunction.size());

	for (const auto& [funcName, secondsSpent] : mNumOfSecondsSpentEachFunction)
	{
		sortedTimesSpent.emplace_back(funcName, secondsSpent);
	}

	std::sort(sortedTimesSpent.begin(), sortedTimesSpent.end(),
		[](const std::pair<std::string, float>& lhs, const std::pair<std::string, float>& rhs)
		{
			return lhs.second > rhs.second;
		});

	std::string output{};
	for (const auto& [funcName, timeSpent] : sortedTimesSpent)
	{
		output += Format("\n{:>64} -  {:.4f}", funcName, timeSpent);
	}
	LOG(LogScripting, Verbose, output);
#endif // SCRIPT_PROFILING
}

void CE::VirtualMachine::Recompile()
{
	[[maybe_unused]] const std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	LOG(LogScripting, Message, "Recompiling...");

	ClearCompilationResult();

	std::vector<std::pair<std::reference_wrapper<MetaType>, std::reference_wrapper<Script>>> createdTypes{};

	for (WeakAssetHandle<Script> asset : AssetManager::Get().GetAllAssets<Script>())
	{
		Script& script = const_cast<Script&>(*AssetHandle<Script>{ asset });

		MetaType* type = script.DeclareMetaType();

		if (type != nullptr)
		{
			createdTypes.emplace_back(*type, script);
		}
	}

	for (auto& [createdType, script] : createdTypes)
	{
		script.get().DeclareMetaFunctions(createdType);
	}

	for (auto& [createdType, script] : createdTypes)
	{
		script.get().PostDeclarationRefresh();

		const size_t numOfErrorsBefore = mErrorsFromLastCompilation.size();
		script.get().CollectErrors(std::back_inserter(mErrorsFromLastCompilation));
		script.get().DefineMetaType(createdType, numOfErrorsBefore != mErrorsFromLastCompilation.size());
	}

	PrintCompileErrors();

	[[maybe_unused]] const std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	[[maybe_unused]] float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1).count();
	LOG(LogScripting, Message, "Compilation completed in {} seconds", deltaTime);
	mIsCompiled = true;
}

void CE::VirtualMachine::ClearCompilationResult()
{
	DestroyAllTypesCreatedThroughScripts();
	mErrorsFromLastCompilation.clear();
	mIsCompiled = false;
}

bool CE::VirtualMachine::IsCompiled() const
{
	return mIsCompiled;
}

std::vector<std::reference_wrapper<const CE::ScriptError>> CE::VirtualMachine::GetErrors(
	const ScriptLocation& location) const
{
	std::vector<std::reference_wrapper<const ScriptError>> returnValue{};

	for (const ScriptError& error : mErrorsFromLastCompilation)
	{
		if (error.GetOrigin().IsLocationEqualToOrInsideOf(location))
		{
			returnValue.emplace_back(error);
		}
	}

	return returnValue;
}

CE::FuncResult CE::VirtualMachine::ExecuteScriptFunction(MetaFunc::DynamicArgs args,
	MetaFunc::RVOBuffer rvoBuffer,
	const ScriptFunc& func,
	const ScriptNode& firstNode,
	const FunctionEntryScriptNode* entryNode)
{
#ifdef SCRIPT_PROFILING
	struct Profiler
	{
		Profiler(float& numOfSecondsSpent) :
			mNumOfSecondsSpent(numOfSecondsSpent)
		{

		}
		~Profiler()
		{
			mNumOfSecondsSpent += mTimer.GetSecondsElapsed();
		}

		float& mNumOfSecondsSpent;
		Timer mTimer{};
	};
	Profiler profiler{ mNumOfSecondsSpentEachFunction[Format("{}::{}", func.GetNameOfScriptAsset(), func.GetName())] };
#endif // SCRIPT_PROFILING

	VMContext context{ func };

	try
	{
		if (context.mCachedValues == nullptr)
		{
			throw ScriptError{ ScriptError::StackOverflow, { context.mFunc } };
		}

		if (!func.IsStatic())
		{
			context.mThis = &args[0];
		}

		if (entryNode != nullptr)
		{
			const std::span<const ScriptPin> entryPins = entryNode->GetOutputs(func);

			// Push the arguments we received to the cache. If this is a member function,
			// skip the first argument; there is no pin for it in the entry node
			for (uint32 argNum = !func.IsStatic(), pinIndex = 0; argNum < args.size();)
			{
				ASSERT(pinIndex < entryPins.size());
				const ScriptPin& pin = entryPins[pinIndex];

				if (pin.IsFlow())
				{
					++pinIndex;
					continue;
				}

				MetaAny& arg = args[argNum];

				VMContext::CachedValue& cachedValue = FindCachedValue(context, pin.GetId());

				const MetaType* const type = arg.TryGetType();
				ASSERT(type != nullptr && "Should've been caught at compile time");

				AllocateForCache(context, cachedValue, pin, type->GetTypeInfo());

				if (pin.GetTypeForm() == TypeForm::Value)
				{
					// Copy construct
					FuncResult copyResult = type->ConstructAt(cachedValue.mData, arg);

					// TODO can be checked at compile time?
					if (copyResult.HasError())
					{
						throw ScriptError{ ScriptError::TypeCannotBeOwnedByScripts, { context.mFunc }, Format("Failed to copy construct type {} from pin {} on node {} - {} try passing by reference instead",
							type->GetName(),
							pin.GetName(),
							entryNode->GetDisplayName(),
							copyResult.Error())
						};
					}
				}
				else
				{
					cachedValue.mData = arg.GetData();
				}

				++argNum;
				++pinIndex;
			}
		}

		const ScriptNode* current = &firstNode;
		uint32 stepNum = 0;

		// We can't execute the entry node, so just skip it and move on to the next one
		if (firstNode.GetType() == ScriptNodeType::FunctionEntry)
		{
			goto nextNode;
		}

		for (; stepNum < sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp; stepNum++)
		{
			// We have reached the end of the function and must return the value
			if (current->GetType() == ScriptNodeType::FunctionReturn)
			{
				for (const ScriptPin& pinToOutputTo : current->GetInputs(func))
				{
					if (pinToOutputTo.IsFlow())
					{
						continue;
					}

					MetaAny ret = GetValueToPassAsInput(context, pinToOutputTo);

					if (pinToOutputTo.GetTypeForm() == TypeForm::Value
						&& !ret.IsOwner())
					{
						const MetaType* const returnType = ret.TryGetType();
						ASSERT(returnType != nullptr && "Shouldve been checked during script compilation");

						FuncResult copyConstructResult = rvoBuffer == nullptr ? returnType->Construct(ret) : returnType->ConstructAt(rvoBuffer, ret);

						if (copyConstructResult.HasError()) // TODO can be checked during script compilation
						{
							std::string error = Format("Could not copy return value of type {} - {}", returnType->GetName(), copyConstructResult.Error());
							PrintError({ ScriptError::FunctionCallFailed, { context.mFunc, pinToOutputTo }, error });
							return error;
						}
						return std::move(copyConstructResult.GetReturnValue());
					}

					return std::move(ret);

				}

				break;
			}

			ExecuteNode(context, *current);

		nextNode:
			current = GetNextNodeToExecute(context, *current);

			// We have reached the end of the function
			if (current == nullptr)
			{
				break;
			}
		}

		if (stepNum == sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp)
		{
			throw ScriptError{ ScriptError::ExecutionTimeOut, { context.mFunc, *current } };
		}

		if (!func.GetReturnType().has_value())
		{
			return { std::nullopt };
		}

		// No return node encountered, returning default constructed value
		const MetaType* const returnType = func.GetReturnType()->TryGetType();

		ASSERT(returnType != nullptr && "Should've been caught during script compilation");

		FuncResult defaultConstructResult = rvoBuffer == nullptr ? returnType->Construct() : returnType->ConstructAt(rvoBuffer);

		// TODO can be compile-time error
		if (defaultConstructResult.HasError())
		{
			throw ScriptError{ ScriptError::Type::ValueWasNull, { context.mFunc, *current },
				Format("No return node encountered, and the return type {} cannot be default constructed - {}",
				returnType->GetName(),
				defaultConstructResult.Error())
			};
		}

		return { std::move(defaultConstructResult.GetReturnValue()) };
	}
	catch (const ScriptError& error)
	{
		PrintError(error);
		return error.ToString(true);
	}
	catch (const std::exception& exception)
	{
		std::string msg = Format("Exception occured - {}", exception.what());
		// TODO: Use a callstack to log the exact node that caused the error
		PrintError({ ScriptError::FunctionCallFailed, ScriptLocation{ func }, msg });
		return std::move(msg);
	}
	catch (...)
	{
		std::string msg = "Unknown exception occured";
		// TODO: Use a callstack to log the exact node that caused the error
		PrintError({ ScriptError::FunctionCallFailed, ScriptLocation{ func }, msg });
		return std::move(msg);
	}
}

void CE::VirtualMachine::PrintCompileErrors() const
{
	for (const ScriptError& error : mErrorsFromLastCompilation)
	{
		PrintError(error, true);
	}
}

void CE::VirtualMachine::PrintError(const ScriptError& error, bool compileError)
{
	Logger::Get().Log(error.ToString(true), compileError ? "ScriptCompileError" : "ScriptRuntimeError", Error, __FILE__, __LINE__,
#ifdef EDITOR
		[loc = error.GetOrigin()]
		{
			loc.NavigateToLocation();
		});
#else
	{});
#endif // EDITOR
}

void CE::VirtualMachine::DestroyAllTypesCreatedThroughScripts()
{
	LOG(LogScripting, Verbose, "Destroying all types created through scripts");

	MetaManager& manager = MetaManager::Get();

	std::vector<TypeId> typesToRemove{};

	for (MetaType& type : manager.EachType())
	{
		if (WasTypeCreatedByScript(type))
		{
			Internal::UnreflectComponentType(type);
			typesToRemove.push_back(type.GetTypeId());
		}
	}

	for (const TypeId typeId : typesToRemove)
	{
		[[maybe_unused]] const bool success = manager.RemoveType(typeId);
		ASSERT(success);
	}
}

CE::VirtualMachine::Stack& CE::VirtualMachine::GetStack()
{
	thread_local Stack stack{};
	return stack;
}

CE::VirtualMachine::VMContext::VMContext(const ScriptFunc& func) :
	mFunc(func)
{
	VirtualMachine& vm = VirtualMachine::Get();

	mStackPtrToFallBackTo = GetStack().mStackPtr;
	uint32 size = static_cast<uint32>(func.GetNumOfPinsIncludingRemoved() * sizeof(CachedValue));
	mCachedValues = static_cast<CachedValue*>(vm.StackAllocate(size, 8));

	if (mCachedValues != nullptr)
	{
		memset(mCachedValues, 0, size);
	}
}

CE::VirtualMachine::VMContext::~VMContext()
{
	VirtualMachine& vm = VirtualMachine::Get();

	for (uint32 i = 0; i < static_cast<uint32>(mFunc.GetNumOfPinsIncludingRemoved()); i++)
	{
		CachedValue& cachedValue = mCachedValues[i];

		// The userBit signifies ownership
		if (!vm.DoesCacheValueNeedToBeFreed(cachedValue))
		{
			continue;
		}

		const ScriptPin& pin = mFunc.GetPin(PinId{ i + 1 });
		const MetaType& metaType = *pin.TryGetType();

		vm.FreeCachedValue(cachedValue, metaType);
	}

	GetStack().mStackPtr = mStackPtrToFallBackTo;
}

void* CE::VirtualMachine::StackAllocate(uint32 numOfBytes, uint32 alignment)
{
	Stack& stack = GetStack();
	size_t sizeLeft = reinterpret_cast<uintptr>(&stack.mData.back()) - reinterpret_cast<uintptr>(stack.mStackPtr) + 1;

	void* stackPtrAsVoid = stack.mStackPtr;
	void* allocatedAt = std::align(alignment, numOfBytes, stackPtrAsVoid, sizeLeft);

	if (allocatedAt != nullptr)
	{
		stack.mStackPtr = static_cast<char*>(allocatedAt) + numOfBytes;
	}
	return allocatedAt;
}

const CE::ScriptNode* CE::VirtualMachine::GetNextNodeToExecute(VMContext& context, const ScriptNode& start)
{
	ASSERT(!start.IsPure(context.mFunc)
		&& !start.GetOutputs(context.mFunc).empty()
		&& start.GetOutputs(context.mFunc)[0].IsFlow());

	uint32 indexOfOutputFlowPin = 0;
	const ScriptNode* current = &start;

	for (size_t i = 0; i < sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp; i++)
	{
		uint32 indexOfInputFlowPin{};

		{
			ASSERT(indexOfOutputFlowPin < current->GetOutputs(context.mFunc).size());

			const ScriptPin& outputFlowPin = current->GetOutputs(context.mFunc)[indexOfOutputFlowPin];

			const PinId linkedToId = outputFlowPin.GetCachedPinWeAreLinkedWith();

			if (linkedToId == PinId::Invalid) // Not linked to anything
			{
				if (context.mControlNodesToFallBackTo.empty())
				{
					return nullptr;
				}

				std::variant<VMContext::WhileLoopInfo, VMContext::ForLoopInfo> loopInfo = context.mControlNodesToFallBackTo.back();

				if (std::holds_alternative<VMContext::WhileLoopInfo>(loopInfo))
				{
					current = &std::get<VMContext::WhileLoopInfo>(loopInfo).mNode.get();
				}
				else
				{
					ASSERT(std::holds_alternative<VMContext::ForLoopInfo>(loopInfo));
					current = &std::get<VMContext::ForLoopInfo>(loopInfo).mNode.get();
				}

				// We didn't enter through any pins, we just went teleported back to the loop node
				indexOfInputFlowPin = std::numeric_limits<uint32>::max();
			}
			else
			{
				current = nullptr;

				const ScriptPin& linkedTo = context.mFunc.GetPin(linkedToId);
				current = &context.mFunc.GetNode(linkedTo.GetNodeId());

				indexOfInputFlowPin = linkedToId.Get() - current->GetIdOfFirstPin().Get();

				ASSERT(&current->GetInputs(context.mFunc)[indexOfInputFlowPin] == &linkedTo);
			}
		}

		ASSERT(current != nullptr);

		switch (current->GetType())
		{
		case ScriptNodeType::ForLoop:
		{
			std::span<const ScriptPin> inputs = current->GetInputs(context.mFunc);
			std::span<const ScriptPin> outputs = current->GetOutputs(context.mFunc);

			//ASSERT(inputs.size() == 4
			//	&& inputs[ForLoopScriptNode::sIndexOfEntryPin].IsFlow() // Entry
			//	&& inputs[ForLoopScriptNode::sIndexOfStartPin].GetTypeTraits() == MakeTypeTraits<int32>() // Start
			//	&& inputs[ForLoopScriptNode::sIndexOfEndPin].GetTypeTraits() == MakeTypeTraits<const int32&>() // End
			//	&& inputs[ForLoopScriptNode::sIndexOfBreakPin].IsFlow() // Break
			//	&& outputs.size() == 3
			//	&& outputs[ForLoopScriptNode::sIndexOfLoopPin].IsFlow() // Loop body
			//	&& outputs[ForLoopScriptNode::sIndexOfIndexPin].GetTypeTraits() == MakeTypeTraits<int32>() // Current index
			//	&& outputs[ForLoopScriptNode::sIndexOfExitPin].IsFlow()); // Exit

			const ScriptPin& indexPin = outputs[ForLoopScriptNode::sIndexOfIndexPin];
			VMContext::CachedValue& cachedIndex = FindCachedValue(context, indexPin.GetId());

			if (indexOfInputFlowPin == ForLoopScriptNode::sIndexOfEntryPin) // for (int i = startValue;
			{
				BeginLoop(context, VMContext::ForLoopInfo{ static_cast<const ForLoopScriptNode&>(*current) });
				AllocateForCache(context, cachedIndex, indexPin, MakeTypeInfo<int32>());

				const ScriptPin& startPin = inputs[ForLoopScriptNode::sIndexOfStartPin];
				*static_cast<int32*>(cachedIndex.mData) = GetValueToPassAsInput<int32>(context, startPin);
			}

			int32& index = *static_cast<int32*>(cachedIndex.mData);

			if (indexOfInputFlowPin != ForLoopScriptNode::sIndexOfEntryPin // Don't increment if we just started the for loop
				&& indexOfInputFlowPin != ForLoopScriptNode::sIndexOfBreakPin) // Don't increment if we hit the break pin
			{
				// We're updating a value in the cache, any nodes that are linked to the index will be recalculated as needed
				++context.mNumOfImpureNodesExecutedAtTimeOfCaching;
				++index;
				cachedIndex.mNumOfImpureNodesExecutedAtTimeOfCaching = context.mNumOfImpureNodesExecutedAtTimeOfCaching;
			}

			const ScriptPin& endPin = inputs[ForLoopScriptNode::sIndexOfEndPin];
			int32 end = GetValueToPassAsInput<int32>(context, endPin);

			if (index >= end // If we've finished iterating
				|| indexOfInputFlowPin == ForLoopScriptNode::sIndexOfBreakPin) // If break pin hit 
			{
				EndLoop(context, *current);
				indexOfOutputFlowPin = ForLoopScriptNode::sIndexOfExitPin; // Exit loop
			}
			else
			{
				indexOfOutputFlowPin = ForLoopScriptNode::sIndexOfLoopPin; // Continue loop body
			}

			break;
		}
		case ScriptNodeType::WhileLoop:
		{
			std::span<const ScriptPin> inputs = current->GetInputs(context.mFunc);

			//ASSERT(inputs.size() == 3
			//	&& inputs[WhileLoopScriptNode::sIndexOfEntryPin].IsFlow() // Entry
			//	&& inputs[WhileLoopScriptNode::sIndexOfConditionPin].GetTypeTraits() == MakeTypeTraits<const bool&>() // Condition
			//	&& inputs[WhileLoopScriptNode::sIndexOfBreakPin].IsFlow() // Break
			//	&& current->GetOutputs(context.mFunc).size() == 2
			//	&& current->GetOutputs(context.mFunc)[WhileLoopScriptNode::sIndexOfLoopPin].IsFlow() // Loop body
			//	&& current->GetOutputs(context.mFunc)[WhileLoopScriptNode::sIndexOfExitPin].IsFlow()); // Exit

			const ScriptPin& conditionPin = inputs[WhileLoopScriptNode::sIndexOfConditionPin];

			bool condition = GetValueToPassAsInput<bool>(context, conditionPin);

			if (indexOfInputFlowPin == WhileLoopScriptNode::sIndexOfEntryPin)
			{
				BeginLoop(context, VMContext::WhileLoopInfo{ static_cast<const WhileLoopScriptNode&>(*current) });
			}

			if (indexOfInputFlowPin == WhileLoopScriptNode::sIndexOfBreakPin // If break pin hit
				|| !condition) // Or if the condition is false
			{
				EndLoop(context, *current);
				indexOfOutputFlowPin = WhileLoopScriptNode::sIndexOfExitPin; // Exit loop
			}
			else
			{
				indexOfOutputFlowPin = WhileLoopScriptNode::sIndexOfLoopPin; // Continue to loop body
			}

			break;
		}
		case ScriptNodeType::Branch:
		{
			std::span<const ScriptPin> inputs = current->GetInputs(context.mFunc);

			//ASSERT(inputs.size() == 2
			//	&& inputs[BranchScriptNode::sIndexOfEntryPin].IsFlow()
			//	&& inputs[BranchScriptNode::sIndexOfConditionPin].GetTypeTraits() == MakeTypeTraits<const bool&>()
			//	&& current->GetOutputs(context.mFunc).size() == 2
			//	&& current->GetOutputs(context.mFunc)[BranchScriptNode::sIndexOfIfPin].IsFlow()
			//	&& current->GetOutputs(context.mFunc)[BranchScriptNode::sIndexOfElsePin].IsFlow());

			const ScriptPin& conditionPin = inputs[BranchScriptNode::sIndexOfConditionPin];

			if (GetValueToPassAsInput<bool>(context, conditionPin))
			{
				indexOfOutputFlowPin = BranchScriptNode::sIndexOfIfPin;
			}
			else
			{
				indexOfOutputFlowPin = BranchScriptNode::sIndexOfElsePin;
			}

			break;
		}
		case ScriptNodeType::Rerout:
		{
			indexOfOutputFlowPin = 0;
			break;
		}
		default:
		{
			return current;
		}
		}
	}

	throw ScriptError{ ScriptError::ExecutionTimeOut, { context.mFunc, start } };
}

CE::VirtualMachine::VMContext::CachedValue* CE::VirtualMachine::ExecuteNode(VMContext& context, const ScriptNode& node)
{
	std::span<const ScriptPin> inputs = node.GetInputs(context.mFunc);
	const size_t numOfInputPins = inputs.size();

	// Raii object that manages the lifetime
	struct InputDeleter
	{
		~InputDeleter()
		{
			for (size_t i = 0; i < mSize; i++)
			{
				mInputForms[i].~MetaAny();
			}
		}

		MetaAny* mInputForms;
		size_t mSize{};
	};

	// Stack allocation
	MetaAny* inputValues = static_cast<MetaAny*>(ENGINE_ALLOCA(numOfInputPins * sizeof(MetaAny)));
	TypeForm* inputForms = static_cast<TypeForm*>(ENGINE_ALLOCA(numOfInputPins * sizeof(TypeForm)));

	InputDeleter inputDeleter{ inputValues };
	ASSERT(inputValues != nullptr && inputForms != nullptr);
	// Collect the inputs that this node needs

	for (size_t i = 0; i < numOfInputPins; i++)
	{
		const ScriptPin& param = inputs[i];

		if (param.IsFlow())
		{
			continue;
		}

		MetaAny valueToPassToPureNode = GetValueToPassAsInput(context, param);
		new (&inputValues[inputDeleter.mSize])MetaAny(std::move(valueToPassToPureNode));
		inputForms[inputDeleter.mSize] = param.GetTypeForm();
		inputDeleter.mSize++;
	}

	const std::span<const ScriptPin> nodeOutputs = node.GetOutputs(context.mFunc);
	ASSERT(nodeOutputs.size() >= 1);

	// If the node returns void, all of these are nullptr.
	const ScriptPin* outputPin = &nodeOutputs[0];
	VMContext::CachedValue* returnAddress{};
	const MetaType* returnType{};

	// If the node is impure
	if (outputPin->IsFlow())
	{
		++context.mNumOfImpureNodesExecutedAtTimeOfCaching;

		if (nodeOutputs.size() == 1)
		{
			// This is an impure node with no return values
			outputPin = nullptr;
		}
		else
		{
			ASSERT(nodeOutputs.size() == 2);
			outputPin++;

			ASSERT(!outputPin->IsFlow());
			ASSERT(outputPin->TryGetType() != nullptr);
		}
	}

	if (outputPin != nullptr)
	{
		returnAddress = &FindCachedValue(context, outputPin->GetId());
		returnType = outputPin->TryGetType();
		ASSERT(returnType != nullptr);
		AllocateForCache(context, *returnAddress, *outputPin, returnType->GetTypeInfo());
	}

	FuncResult result{};

	switch (node.GetType())
	{
	default:
	case ScriptNodeType::Comment: // How would we even execute a comment node
	case ScriptNodeType::FunctionReturn: // This node can never be executed
	case ScriptNodeType::FunctionEntry: // This node can never be executed
	case ScriptNodeType::Branch: // This node can never be executed
	{
		throw ScriptError{ ScriptError::CompilerBug, { context.mFunc, *outputPin } };
	}
	case ScriptNodeType::Setter:
	case ScriptNodeType::Getter:
	{
		ASSERT(inputDeleter.mSize != 0 && "Getting or setting a field always require a target");
		ASSERT(returnAddress != nullptr && "Does not return void; memory should have been allocated");

		const MetaField* const metaMember = static_cast<const NodeInvolvingField&>(node).TryGetOriginalField();
		ASSERT(metaMember != nullptr && "Should've been caught during script-compilation");

		ASSERT(&metaMember->GetType() == returnType);

		MetaAny& refToTarget = inputValues[0];
		ASSERT(metaMember->GetOuterType().IsBaseClassOf(refToTarget.GetTypeId()) && "Invalid link, should've been caught during script compilation");

		MetaAny refToMemberInsideTarget = metaMember->MakeRef(refToTarget);

		if (node.GetType() == ScriptNodeType::Setter)
		{
			ASSERT(inputDeleter.mSize == 2 && "setting a field always require two arguments");

			MetaAny& valueToSetItTo = inputValues[1];

			const MetaType* const typeOfArg = valueToSetItTo.TryGetType();
			ASSERT(typeOfArg != nullptr && "Should've been caught during script-compilation");

			// TODO Check if it has the copy assign operator at script-compile time
			// We can't use this setResult, because the assignment operator returns a reference,
			// and we want to cache this value. If the object gets destroyed, the reference
			// will be dangling. We make a copy further along the line, and cache that instead.
			FuncResult setResult = typeOfArg->Assign(refToMemberInsideTarget, valueToSetItTo);

			if (setResult.HasError())
			{
				throw ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, node }, setResult.Error() };
			}
		}
		else if (static_cast<const GetterScriptNode&>(node).DoesNodeReturnCopy())
		{
			// Make a copy of the value when getting or setting
			// TODO Check if it has the copy-constructor at script-compile time
			result = returnType->ConstructAt(returnAddress->mData, refToMemberInsideTarget);
			break;
		}

		result = std::move(refToMemberInsideTarget);
		break;
	}
	case ScriptNodeType::FunctionCall:
	{
		const MetaFuncScriptNode& metaFuncNode = static_cast<const MetaFuncScriptNode&>(node);

		const MetaFunc* const originalFunc = metaFuncNode.TryGetOriginalFunc();
		ASSERT(originalFunc != nullptr && "Should've been caught during script-compilation");

		// We assume the types match, because that was checked during script-compilation
		// so we only have to check if something was null

		const std::vector<MetaFuncNamedParam>& params = originalFunc->GetParameters();

		for (uint32 i = 0; i < inputDeleter.mSize; i++)
		{
			if (inputValues[i] == nullptr
				&& !CanFormBeNullable(params[i].mTypeTraits.mForm))
			{
				throw ScriptError{ ScriptError::ValueWasNull, { context.mFunc, node } };
			}
		}

		result = originalFunc->InvokeUnchecked({ inputValues, inputDeleter.mSize }, { inputForms, inputDeleter.mSize }, returnAddress == nullptr ? nullptr : returnAddress->mData);
		break;
	}
	}

	if (result.HasError())
	{
		throw ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, node }, result.Error() };
	}

	ASSERT(returnAddress == nullptr == !result.HasReturnValue() && "No memory was allocated, but the result holds a return value, or Memory was allocated, but the result holds no return value");

	if (returnAddress != nullptr)
	{
		// Some functions may return a reference or pointer.
		returnAddress->mData = result.GetReturnValue().GetData();
		return returnAddress;
	}

	return nullptr;
}

CE::VirtualMachine::VMContext::WhileLoopInfo& CE::VirtualMachine::BeginLoop(VMContext& context, VMContext::WhileLoopInfo&& loopInfo)
{
	return std::get<VMContext::WhileLoopInfo>(context.mControlNodesToFallBackTo.emplace_back(std::move(loopInfo)));
}

CE::VirtualMachine::VMContext::ForLoopInfo& CE::VirtualMachine::BeginLoop(VMContext& context, VMContext::ForLoopInfo&& loopInfo)
{
	return std::get<VMContext::ForLoopInfo>(context.mControlNodesToFallBackTo.emplace_back(std::move(loopInfo)));
}

void CE::VirtualMachine::EndLoop(VMContext& context, const ScriptNode& node)
{
	auto nodeToFallBackTo = std::find_if(context.mControlNodesToFallBackTo.crbegin(), context.mControlNodesToFallBackTo.crend(),
		[&node](const std::variant<VMContext::WhileLoopInfo, VMContext::ForLoopInfo>& loopInfo)
		{
			if (std::holds_alternative<VMContext::WhileLoopInfo>(loopInfo))
			{
				return &node == &std::get<VMContext::WhileLoopInfo>(loopInfo).mNode.get();
			}
			ASSERT(std::holds_alternative<VMContext::ForLoopInfo>(loopInfo));
			return &node == &std::get<VMContext::ForLoopInfo>(loopInfo).mNode.get();
		});

	if (nodeToFallBackTo == context.mControlNodesToFallBackTo.crend())
	{
		PrintError(ScriptError{ ScriptError::LinkNotAllowed, { context.mFunc, node }, "Attempted to break out of a loop that was not running", });
	}
	else
	{
		++nodeToFallBackTo;
		context.mControlNodesToFallBackTo.erase(nodeToFallBackTo.base(), context.mControlNodesToFallBackTo.end());
	}
}

CE::VirtualMachine::VMContext::CachedValue& CE::VirtualMachine::FindCachedValue(VMContext& context, PinId pinId)
{
	return context.mCachedValues[pinId.Get() - 1];
}

template<>
CE::MetaAny CE::VirtualMachine::GetValueToPassAsInput(VMContext& context, const ScriptPin& pinToPassInputInto)
{
	ASSERT(pinToPassInputInto.IsInput());

	// Get the output pin this pin is linked to
	const PinId otherPinId = pinToPassInputInto.GetCachedPinWeAreLinkedWith();

	std::optional<MetaAny> returnValue{};
	const TypeForm form = pinToPassInputInto.GetTypeForm();

	// No output pin? No worries, we'll just use the default value!
	if (otherPinId == PinId::Invalid)
	{
		// There is no link connected to this pin, maybe we can default to 'this'?
		if (pinToPassInputInto.GetTypeName() == context.mFunc.GetNameOfScriptAsset())
		{
			if (context.mFunc.IsStatic())
			{
				throw ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto },
					Format("Cannot call non-static functions ({}) from static functions ({})",
						context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
						context.mFunc.GetName())
				};
			}

			// An instance of the script should have been provided as an argument to this function
			if (context.mThis == nullptr)
			{
				throw ScriptError{ ScriptError::CompilerBug, { context.mFunc, pinToPassInputInto },
					"Compiler error: 'This' was unexpectedly nullptr for a non-static function. This should have been checked earlier." };
			}

			returnValue = MakeRef(*context.mThis);
		}
		else if (!DoesPinRequireLink(context.mFunc, pinToPassInputInto))
		{
			const MetaAny* valueIfNoInputLinked = pinToPassInputInto.TryGetValueIfNoInputLinked();

			if (valueIfNoInputLinked != nullptr)
			{
				ASSERT(valueIfNoInputLinked->GetData() != nullptr && "Not sure how this could have happened");
				returnValue = MakeRef(const_cast<MetaAny&>(*valueIfNoInputLinked));
			}
			else
			{
				const MetaType* const pinType = pinToPassInputInto.TryGetType();
				ASSERT(pinType != nullptr && "Shoulve been caught during script compilation");

				// Construct default
				FuncResult defaultConstructedValue = pinType->Construct();

				// TODO can be checked during script-compilation
				if (defaultConstructedValue.HasError())
				{
					throw ScriptError{ ScriptError::FunctionCallFailed, { context.mFunc, pinToPassInputInto },
						Format("Type {} of input pin {} has no link connected to it, and creating a default value failed - {}",
						pinType->GetName(),
						pinToPassInputInto.GetName(),
						defaultConstructedValue.Error())
					};
				}

				returnValue = std::move(defaultConstructedValue.GetReturnValue());
			}
		}
		else
		{
			// TODO can be checked during script-compilation
			throw ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto }, "No link was connected to pin" };
		}
	}
	else
	{
		const ScriptPin& otherPin = context.mFunc.GetPin(otherPinId);
		const ScriptNode& otherNode = context.mFunc.GetNode(otherPin.GetNodeId());

		if (otherNode.GetType() == ScriptNodeType::Rerout)
		{
			return GetValueToPassAsInput(context, otherNode.GetInputs(context.mFunc)[0]);
		}

		// If the pin type is MetaAny, we accept anything. We cannot assign that to returnValue, 
		// because typeId<MetaAny> != typeIdOfWhateverIsConnectedToOurPin
		// A bit of an exception to the rule that makes this part of the code a bit messy
		// At the time of writing, this is only used for the IsNull node
		bool returnImmediately{};

		if (pinToPassInputInto.TryGetType()->GetTypeId() == MakeTypeId<MetaAny>())
		{
			// TODO can be checked during script compilation
			if (pinToPassInputInto.GetTypeForm() == TypeForm::Value)
			{
				throw ScriptError{ ScriptError::TypeCannotBeOwnedByScripts, { context.mFunc, pinToPassInputInto }, "Pins accepting 'MetaAny' by value is not allowed; use a reference or a const reference instead" };
			}
			returnImmediately = true;
		}

		VMContext::CachedValue& cachedValue = FindCachedValue(context, otherPin.GetId());

		if (cachedValue.mNumOfImpureNodesExecutedAtTimeOfCaching == context.mNumOfImpureNodesExecutedAtTimeOfCaching // Did we run any impure nodes that could have changed the cached value
			|| !otherNode.IsPure(context.mFunc)) // Impure nodes are not executed again if the cache is 'out of date', this is perfectly valid and expected behaviour
		{
			// Yay! We can use the cached value
			if (returnImmediately)
			{
				return MetaAny{ *otherPin.TryGetType(), cachedValue.mData, false };
			}
			returnValue = MetaAny{ *otherPin.TryGetType(), cachedValue.mData, false };
		}
		else
		{
			// The cache is out of date. We have to recalculate it

			// We can't suddenly execute a non-pure node
			if (!otherNode.IsPure(context.mFunc))
			{
				throw ScriptError{ ScriptError::LinkNotAllowed, { context.mFunc, context.mFunc.GetAllLinksConnectedToPin(pinToPassInputInto.GetId())[0] },
					Format("The input pin {} in node {} is connected to the output pin {} in node {}, but node {} runs before node {}!",
					pinToPassInputInto.GetName(),
					context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
					otherNode.GetDisplayName(),
					otherNode.GetDisplayName(),
					context.mFunc.GetNode(pinToPassInputInto.GetNodeId()).GetDisplayName(),
					otherNode.GetDisplayName())
				};
			}

			// Run the node
			const VMContext::CachedValue* const nodeReturnValue = ExecuteNode(context, otherNode);

			if (nodeReturnValue == nullptr)
			{
				throw ScriptError{ ScriptError::CompilerBug, { context.mFunc, pinToPassInputInto },
					Format("The node {} unexpectedly returned void", otherNode.GetDisplayName()) };
			}

			const TypeInfo returnTypeInfo = otherPin.TryGetType()->GetTypeInfo();

			if (returnImmediately)
			{
				return MetaAny{ returnTypeInfo, nodeReturnValue->mData };
			}

			returnValue = MetaAny{ returnTypeInfo, nodeReturnValue->mData };
		}
	}

	ASSERT(returnValue.has_value());

	// Check if it's nullptr
	if (*returnValue == nullptr
		&& !CanFormBeNullable(form))
	{
		throw ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto } };
	}

	return std::move(*returnValue);
}

void CE::VirtualMachine::AllocateForCache(VMContext& context, VMContext::CachedValue& cachedValue, const ScriptPin& pin, TypeInfo typeInfo)
{
	cachedValue.mNumOfImpureNodesExecutedAtTimeOfCaching = context.mNumOfImpureNodesExecutedAtTimeOfCaching;

	cachedValue.mTypeInfoFlags = typeInfo.mFlags;

	if (pin.GetTypeForm() != TypeForm::RValue
		&& pin.GetTypeForm() != TypeForm::Value)
	{
		cachedValue.mTypeInfoFlags &= ~TypeInfo::UserBit;
		return;
	}

	// Reuse the existing space if we can
	if (cachedValue.mData != nullptr)
	{
		if (DoesCacheValueNeedToBeFreed(cachedValue))
		{
			FreeCachedValue(cachedValue, *pin.TryGetType());
		}

		return;
	}

	cachedValue.mData = StackAllocate(typeInfo.GetSize(), typeInfo.GetAlign());
	cachedValue.mTypeInfoFlags |= TypeInfo::UserBit;

	if (cachedValue.mData == nullptr)
	{
		throw ScriptError{ ScriptError::StackOverflow, { context.mFunc } };
	}
}

bool CE::VirtualMachine::DoesCacheValueNeedToBeFreed(VMContext::CachedValue& cachedValue)
{
	return (cachedValue.mTypeInfoFlags & TypeInfo::UserBit) && (cachedValue.mTypeInfoFlags & TypeInfo::IsTriviallyDestructible) == 0 && cachedValue.mData != nullptr;
}

void CE::VirtualMachine::FreeCachedValue(VMContext::CachedValue& cachedValue, const MetaType& pinType)
{
	ASSERT(DoesCacheValueNeedToBeFreed(cachedValue));
	pinType.Destruct(cachedValue.mData, false);
}

template<typename T>
T CE::VirtualMachine::GetValueToPassAsInput(VMContext& context, const ScriptPin& pinToPassInputInto)
{
	ASSERT(pinToPassInputInto.IsInput() && pinToPassInputInto.TryGetType()->GetTypeId() == MakeTypeId<T>());

	MetaAny valueToPassAsInput = GetValueToPassAsInput(context, pinToPassInputInto);
	ASSERT(valueToPassAsInput.IsExactly<T>() && "Shoulve been caught at script-compilation");

	const T* const conditionValue = static_cast<T*>(valueToPassAsInput.GetData());

	if (conditionValue == nullptr)
	{
		throw ScriptError{ ScriptError::ValueWasNull, { context.mFunc, pinToPassInputInto } };
	}

	return *conditionValue;
}

