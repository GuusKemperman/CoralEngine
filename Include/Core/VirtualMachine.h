#pragma once
#include "Core/EngineSubsystem.h"

#include "Scripting/ScriptErrors.h"
#include "Utilities/Expected.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaFunc.h"

namespace Engine
{
	class Script;
	class ScriptField;
	class ScriptFunc;
	class ScriptNode; 
	class FunctionEntryScriptNode; 
	class WhileLoopScriptNode;
	class ForLoopScriptNode;
	class ScriptPin;
	class ScriptLink;
	class MetaAny;

	class VirtualMachine :
		public EngineSubsystem<VirtualMachine>
	{
		friend EngineSubsystem;
		void PostConstruct() override;
		~VirtualMachine();


	public:
		/*
		Will iterate over all script assets and create metatypes out of them.
		
		If this function was called before, it will first remove all MetaTypes that were created before. Any references to them will become dangling.
		*/
		void Recompile();

		/*
		Will destroy all types created by the compilation process and clear any errors. 
		This is also automatically called when calling Recompile.
		*/
		void ClearCompilationResult();
		
		// Returns the errors that were found during the most recent compilation.
		std::vector<std::reference_wrapper<const ScriptError>> GetErrors(const ScriptLocation& location) const;

		static void PrintError(const ScriptError& error, bool compileError = false);

		/*
		Should not be called directly, but scripts compile into MetaTypes,
		a ScriptFunc compiles into a function on this metaType. These can be called.
		This function can only be called if the script function compiled succesfully.

		FirstNode:
			will be the entryNode on impure functions and the return node on pure functions.

		entryNode:
			may be nullptr, if and only if the function is pure and does not have any parameters (besides the implicit 'this' parameters)

		Example:
			So if you have a script called "Printer" with a function "PrintHelloWorld",
			you can do the following:

			const MetaType* const printerType = MetaManager::Get().TryGetType("Printer");
			const MetaFunc* const printerFunc = printerType->TryGetFunc("PrintHelloWorld");

			FuncResult result = printerFunc->Invoke({});
		*/
		FuncResult ExecuteScriptFunction(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer, const ScriptFunc& func, const ScriptNode& firstNode, const FunctionEntryScriptNode* entryNode);


	private:
		void PrintCompileErrors() const;


		// When we recompile, we first have to clean up the result of the previous compilation
		static void DestroyAllTypesCreatedThroughScripts();

		static constexpr uint32 sMaxNumOfNodesToExecutePerFunctionBeforeGivingUp = 10'000'000;
		static constexpr uint32 sMaxStackSize = 1 << 16;
		std::array<char, sMaxStackSize> mStack{};
		char* mStackPtr = &mStack[0];

		std::vector<ScriptError> mErrorsFromLastCompilation{};

		struct VMContext
		{
			VMContext(const ScriptFunc& func);
			~VMContext();

			const ScriptFunc& mFunc;
			MetaAny* mThis{};
			uint32 mNumOfImpureNodesExecutedAtTimeOfCaching{1};

			// VirtualMachine::mStackPtr  at the time of this object's construction.
			// used to roll back the stack
			char* mStackPtrToFallBackTo{};

			struct CachedValue
			{
				void* mData{};
				uint32 mNumOfImpureNodesExecutedAtTimeOfCaching{ std::numeric_limits<uint32>::max() };
				uint32 mTypeInfoFlags{};
			};
			CachedValue* mCachedValues{};

			// If we are in a for or while loop and there is no next node,
			// we fall back to the node at the top of this stack

			struct ForLoopInfo
			{
				std::reference_wrapper<const ForLoopScriptNode> mNode;
			};

			struct WhileLoopInfo
			{
				std::reference_wrapper<const WhileLoopScriptNode> mNode;
			};

			std::vector<std::variant<WhileLoopInfo, ForLoopInfo>> mControlNodesToFallBackTo{};
		};
		friend VMContext;

		void* StackAllocate(uint32 numOfBytes, uint32 alignment);

		Expected<const ScriptNode*, ScriptError> GetNextNodeToExecute(VMContext& context, const ScriptNode& current);

		template<typename T = MetaAny>
		Expected<T, ScriptError> GetValueToPassAsInput(VMContext& context, const ScriptPin& pinToPassInputInto);

		template<>
		Expected<MetaAny, ScriptError> GetValueToPassAsInput(VMContext& context,
			const ScriptPin& pinToPassInputInto);

		// Will return true on success. May fail if stack overflow
		bool AllocateForCache(VMContext& context, VMContext::CachedValue& cachedValue, const ScriptPin& pin, TypeInfo typeInfo);

		static bool DoesCacheValueNeedToBeFreed(VMContext::CachedValue& cachedValue);

		void FreeCachedValue(VMContext::CachedValue& cachedValue, const MetaType& pinType);

		VMContext::CachedValue& FindCachedValue(VMContext& context, PinId pinId);

		// Will execute the node. If it has a return value, the value will be cached. A pointer to the cached value is then returned.
		Expected<VMContext::CachedValue*, ScriptError> ExecuteNode(VMContext& context, const ScriptNode& node);

		VMContext::WhileLoopInfo& BeginLoop(VMContext& context, VMContext::WhileLoopInfo&& loopInfo);
		VMContext::ForLoopInfo& BeginLoop(VMContext& context, VMContext::ForLoopInfo&& loopInfo);
		void EndLoop(VMContext& context, const ScriptNode& node);
	};
}
