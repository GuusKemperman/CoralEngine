#pragma once
#include "imnodes/imgui_node_editor.h"

#include "Utilities/Expected.h"
#include "Utilities/IterableRange.h"
#include "Scripting/ScriptPin.h"
#include "Scripting/ScriptLink.h"
#include "Scripting/ScriptIds.h"
#include "Meta/MetaTypeTraits.h"
#include "Meta/MetaFunc.h"
#include "Scripting/ScriptConfig.h"

namespace CE
{
	class ScriptEvent;
}

namespace CE
{
	class MetaType;
	class MetaFunc;
	class MetaField;
	struct MetaFuncNamedParam;
	class BinaryGSONObject;
	class Script;
	class FunctionEntryScriptNode;
	class ScriptNode;


#ifdef REMOVE_FROM_SCRIPTS_ENABLED
	namespace Internal
	{
		bool IsNull(const ScriptLink& link);
		bool IsNull(const ScriptPin& link);
		bool IsNull(const ScriptNode& node);
	}

	// SKips all the elements that are zeroed out.
	template<typename UnderlyingIt, typename ContainerType>
	struct SkipZeroIterator
	{
	public:
		using value_type = typename UnderlyingIt::value_type;

		SkipZeroIterator(UnderlyingIt&& it, ContainerType& container) :
			mIt(std::move(it)),
			mContainer(container)
		{}

		decltype(auto) operator*() const { return mIt.operator*(); }
		decltype(auto) operator->() const { return mIt.operator->(); }

		// Prefix increment
		SkipZeroIterator& operator++()
		{
			do
			{
				++mIt;
			} while (mIt != mContainer.get().end() && Internal::IsNull(*mIt));
			return *this;
		}

		// Postfix increment
		SkipZeroIterator operator++(int) { SkipZeroIterator tmp = *this; ++(*this);	return tmp; }

		constexpr bool operator==(const SkipZeroIterator& b) const { return mIt == b.mIt; }
		constexpr bool operator!=(const SkipZeroIterator& b) const { return mIt != b.mIt; }

	private:
		UnderlyingIt mIt;
		std::reference_wrapper<ContainerType> mContainer;
	};

	namespace Internal
	{
		using NodesConstIterator = SkipZeroIterator<std::vector<std::reference_wrapper<ScriptNode>>::const_iterator, const std::vector<std::reference_wrapper<ScriptNode>>>;
		using NodesIterator = SkipZeroIterator<std::vector<std::reference_wrapper<ScriptNode>>::iterator, std::vector<std::reference_wrapper<ScriptNode>>>;

		using LinksConstIterator = SkipZeroIterator<std::vector<ScriptLink>::const_iterator, const std::vector<ScriptLink>>;
		using LinksIterator = SkipZeroIterator<std::vector<ScriptLink>::iterator, std::vector<ScriptLink>>;

		using PinConstIterator = SkipZeroIterator<std::vector<ScriptPin>::const_iterator, const std::vector<ScriptPin>>;
		using PinIterator = SkipZeroIterator<std::vector<ScriptPin>::iterator, std::vector<ScriptPin>>;
	}
#else
	namespace Internal
	{
		using NodesConstIterator = std::vector<std::reference_wrapper<ScriptNode>>::const_iterator;
		using NodesIterator = std::vector<std::reference_wrapper<ScriptNode>>::iterator;

		using LinksConstIterator = std::vector<ScriptLink>::const_iterator;
		using LinksIterator = std::vector<ScriptLink>::iterator;

		using PinConstIterator = std::vector<ScriptPin>::const_iterator;
		using PinIterator = std::vector<ScriptPin>::iterator;
	}
#endif


	class ScriptFunc
	{
	public:
		ScriptFunc(const Script& script, std::string_view name);
		ScriptFunc(const Script& script, const ScriptEvent& event);

		ScriptFunc(const ScriptFunc&) = delete;
		ScriptFunc(ScriptFunc&&) noexcept = default;

		ScriptFunc& operator=(const ScriptFunc&) = delete;
		ScriptFunc& operator=(ScriptFunc&&) noexcept = default;

		~ScriptFunc();

		const std::string& GetName() const { return mName; }
		void SetName(const std::string_view name) { mName = name; }

		const std::string& GetNameOfScriptAsset() const { return mNameOfScriptAsset; }
		TypeId GetTypeIdOfScriptAsset() const { return mTypeIdOfScript; }

		/*
		The function does nothing when called; first all the script classes are declared, after which Script::DefineMetaType
		is called. See MetaFunc::DefineMetaFunc comment.
		*/
		void DeclareMetaFunc(MetaType& addToType);

		/*
		The function created during DeclareMetaFunc is passed as an argument. DefineMetaFunc is responsible for
		compiling the function and making it callable.

		The reason that this is in two steps (DeclareMetaFunc and DefineMetaFunc) rather than one function, is because
		Player::OnTick might reference something in script Enemy; if Player::OnTick compiles before script enemy, it will not be
		able to find a type called Enemy and fail to compile. That's why all the functions are first declared before we
		start compiling.
		*/
		void DefineMetaFunc(MetaFunc& func);

		ScriptNode* TryGetNode(NodeId id);
		const ScriptNode* TryGetNode(NodeId id) const;

		ScriptLink* TryGetLink(LinkId id);
		const ScriptLink* TryGetLink(LinkId id) const;

		ScriptPin* TryGetPin(PinId id);
		const ScriptPin* TryGetPin(PinId id) const;

		ScriptNode& GetNode(NodeId id);
		const ScriptNode& GetNode(NodeId id) const;

		ScriptLink& GetLink(LinkId id);
		const ScriptLink& GetLink(LinkId id) const;

		ScriptPin& GetPin(PinId id);
		const ScriptPin& GetPin(PinId id) const;

		template<typename T, typename ...Args>
		T& AddNode(Args&&... args);

		template<typename T>
		T& AddNode(std::unique_ptr<T>&& node);

		// Should only be called from ScriptNode.cpp
		Span<ScriptPin> AllocPins(NodeId calledFrom,
			std::vector<ScriptVariableTypeData>&& inputs,
			std::vector<ScriptVariableTypeData>&& outputs);

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		// Links can only be added if they can be removed as well,
		// as there is a maximum number of links a pin can have.
		ScriptLink* TryAddLink(const ScriptPin& startPin, const ScriptPin& endPin);

		void RemoveLink(LinkId linkId);

		void FreePins(PinId firstPin, uint32 amount);

		void RemoveNode(NodeId nodeId);
#endif

		bool IsPure() const { return mIsPure; }
		void SetIsPure(bool isPure) { mIsPure = isPure; }

		bool IsStatic() const { return mIsStatic; }
		void SetIsStatic(bool isStatic) { mIsStatic = isStatic; }

		void SerializeTo(BinaryGSONObject& object) const;
		static std::optional<ScriptFunc> DeserializeFrom(const BinaryGSONObject& object, const Script& owningScript, uint32 version);

		std::vector<std::reference_wrapper<ScriptLink>> GetAllLinksConnectedToPin(PinId id);
		std::vector<std::reference_wrapper<const ScriptLink>> GetAllLinksConnectedToPin(PinId id) const;

		IterableRange<Internal::NodesIterator> GetNodes() { return GetIterableRange<Internal::NodesIterator>(mNodes); }
		IterableRange<Internal::NodesConstIterator> GetNodes() const { return GetIterableRange<Internal::NodesConstIterator>(mNodes); }

		IterableRange<Internal::LinksIterator> GetLinks() { return GetIterableRange<Internal::LinksIterator>(mLinks); }
		IterableRange<Internal::LinksConstIterator> GetLinks() const { return GetIterableRange<Internal::LinksConstIterator>(mLinks); }

		IterableRange<Internal::PinIterator> GetPins() { return GetIterableRange<Internal::PinIterator>(mPins); }
		IterableRange<Internal::PinConstIterator> GetPins() const { return GetIterableRange<Internal::PinConstIterator>(mPins); }

		std::vector<ScriptVariableTypeData> GetParameters(bool ignoreObjectInstanceIfMemberFunction) const;
		void SetParameters(std::vector<ScriptVariableTypeData>&& params) { mParams = std::move(params); }
		void SetParameters(const std::vector<ScriptVariableTypeData>& params) { mParams = params; }

		const std::optional<ScriptVariableTypeData>& GetReturnType() const { return mReturns; }
		void SetReturnType(std::optional<ScriptVariableTypeData>&& returnType) { mReturns = std::move(returnType); }

		void CollectErrors(ScriptErrorInserter inserter, const Script& owningScript) const;

		/*
		Will return the entry node if this is an impure function, or the return node if this is an impure function.
		May return an error if there are too many/too little entry/return nodes.
		*/
		Expected<std::reference_wrapper<const ScriptNode>, std::string> GetFirstNode() const;

		/*
		Will return the entry node, if there is one.

		If there is more than one entry node, or if there is no entry node when there *should* be, the return value will hold
		an error.
		*/
		Expected<const FunctionEntryScriptNode*, std::string> GetEntryNode() const;

		// Returns the elements you just added
		std::tuple<Span<std::reference_wrapper<ScriptNode>>, Span<ScriptLink>, Span<ScriptPin>> AddCondensed(std::vector<std::unique_ptr<ScriptNode>>&& nodes,
			std::vector<ScriptLink>&& links,
			std::vector<ScriptPin>&& pins);

		size_t GetNumOfLinksIncludingRemoved() const { return mLinks.size(); }
		size_t GetNumOfPinsIncludingRemoved() const { return mPins.size(); }
		size_t GetNumOfNodesIncludingRemoved() const { return mNodes.size(); }

		// Some nodes need to lookup data. To speed up execution,
		// each node only does this lookup once, when this function is called.
		void PostDeclarationRefresh();

		bool IsEvent() const { return mBasedOnEvent != nullptr; }
		const ScriptEvent* TryGetEvent() const { return mBasedOnEvent; }

	private:
#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		template<typename ItType, typename VectorType>
		static IterableRange<ItType> GetIterableRange(VectorType& vec)
		{
			return { { std::find_if(vec.begin(), vec.end(), [](decltype(vec[0]) element) { return !Internal::IsNull(element); }), vec}, {vec.end(), vec} };
		}
#else
		template<typename ItType, typename VectorType>
		static IterableRange<ItType> GetIterableRange(VectorType& vec)
		{
			return { vec.begin(), vec.end() };
		}
#endif

		std::string mName{};
		std::string mNameOfScriptAsset{};
		TypeId mTypeIdOfScript{};

		// Essentially raw pointers, delete must be called for them.
		std::vector<std::reference_wrapper<ScriptNode>> mNodes{};
		std::vector<ScriptLink> mLinks{};
		std::vector<ScriptPin> mPins{};

		std::vector<ScriptVariableTypeData> mParams{};
		std::optional<ScriptVariableTypeData> mReturns{};

		const ScriptEvent* mBasedOnEvent{};

		bool mIsPure{};
		bool mIsStatic{};
	};

	template <typename T, typename ... Args>
	T& ScriptFunc::AddNode(Args&&... args)
	{
		T& node = static_cast<T&>(mNodes.emplace_back(*new T(std::forward<Args>(args)...)).get());
		ASSERT(node.GetId().Get() == mNodes.size());
		return node;
	}

	template <typename T>
	T& ScriptFunc::AddNode(std::unique_ptr<T>&& node)
	{
		T* nodePtr = node.release();
		mNodes.emplace_back(*nodePtr);
		ASSERT(node.GetId().Get() == mNodes.size());
		return *nodePtr;
	}

	inline CE::ScriptNode& CE::ScriptFunc::GetNode(NodeId id)
	{
		ASSERT(TryGetNode(id) != nullptr);
		return mNodes[id.Get() - 1];
	}

	inline const CE::ScriptNode& CE::ScriptFunc::GetNode(NodeId id) const
	{
		ASSERT(TryGetNode(id) != nullptr);
		return mNodes[id.Get() - 1];
	}

	inline CE::ScriptLink& CE::ScriptFunc::GetLink(LinkId id)
	{
		ASSERT(TryGetLink(id) != nullptr);
		return mLinks[id.Get() - 1];
	}

	inline const CE::ScriptLink& CE::ScriptFunc::GetLink(LinkId id) const
	{
		ASSERT(TryGetLink(id) != nullptr);
		return mLinks[id.Get() - 1];
	}

	inline CE::ScriptPin& CE::ScriptFunc::GetPin(PinId id)
	{
		ASSERT(TryGetPin(id) != nullptr);
		return mPins[id.Get() - 1];
	}

	inline const CE::ScriptPin& CE::ScriptFunc::GetPin(PinId id) const
	{
		ASSERT(TryGetPin(id) != nullptr);
		return mPins[id.Get() - 1];
	}
}
