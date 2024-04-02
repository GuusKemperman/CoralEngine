#pragma once
#include "ScriptFunc.h"
#include "ScriptPin.h"
#include "Scripting/ScriptErrors.h"
#include "Scripting/ScriptIds.h"
#include "Meta/MetaFunc.h"
#include "Scripting/ScriptConfig.h"

namespace CE
{
	class ScriptIdGenerator;
	class ScriptFunc;
	class BinaryGSONObject;

	enum class ScriptNodeType : uint32
	{
		Branch,
		Comment,
		FunctionCall,
		Setter,
		Getter,
		FunctionEntry,
		FunctionReturn,
		/* There used to be another entry here  */
		ForLoop = 8,
		WhileLoop,
		Rerout
	};

	class ScriptNode
	{
	protected:
		ScriptNode(ScriptNodeType type);

	public:
		ScriptNode(ScriptNodeType type, ScriptFunc& scriptFunc);

		ScriptNode(ScriptNode&&) noexcept = default;
		ScriptNode(const ScriptNode&) = delete;

		ScriptNode& operator=(ScriptNode&&) noexcept = default;
		ScriptNode& operator=(const ScriptNode&) = delete;

		virtual ~ScriptNode() = default;

		NodeId GetId() const { return mId; }

		// Use with care, dangerous.
		void SetId(NodeId id) { mId = id; }

		ScriptNodeType GetType() const { return mType; }

		glm::vec2 GetPosition() const { return mPosition; }
		void SetPosition(glm::vec2 position) { mPosition = position; }

		Span<const ScriptPin> GetInputs(const ScriptFunc& scriptFunc) const;
		Span<ScriptPin> GetInputs(ScriptFunc& scriptFunc);

		Span<const ScriptPin> GetOutputs(const ScriptFunc& scriptFunc) const;
		Span<ScriptPin> GetOutputs(ScriptFunc& scriptFunc);

		Span<const ScriptPin> GetPins(const ScriptFunc& scriptFunc) const;
		Span<ScriptPin> GetPins(ScriptFunc& scriptFunc);

		PinId GetIdOfFirstPin() const { return mFirstPinId; }
		bool IsPure(const ScriptFunc& scriptFunc) const;

		std::string GetDisplayName() const { return Format("{} ({})", GetTitle(), GetSubTitle()); }

		struct InputsOutputs
		{
			std::vector<ScriptVariableTypeData> mInputs{};
			std::vector<ScriptVariableTypeData> mOutputs{};
		};

		void SetPins(ScriptFunc& scriptFunc,
			InputsOutputs&& inputsOutputs);

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		void ClearPins(ScriptFunc& scriptFunc);
#endif // REMOVE_FROM_SCRIPTS_ENABLED

		// This is used to remap the IDs when duplicating a selection of nodes.
		void GetReferencesToIds(ScriptIdInserter inserter);

		static std::unique_ptr<ScriptNode> DeserializeFrom(const BinaryGSONObject& from, std::back_insert_iterator<std::vector<ScriptPin>> pinInserter, uint32 version);

		//********************************//
		//		Virtual functions		  //
		//********************************//
		virtual void CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const;

		virtual std::string GetTitle() const { return ""; }
		virtual std::string GetSubTitle() const { return ""; }

		virtual glm::vec4 GetHeaderColor(const ScriptFunc&) const { return glm::vec4{ 0.4f }; }

		// Will cache some lookups to speed up execution of this node
		virtual void PostDeclarationRefresh(ScriptFunc&) {};

		virtual void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const;

	protected:
		// Return true on success
		virtual bool DeserializeVirtual(const BinaryGSONObject& from);

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		void RefreshByComparingPins(ScriptFunc& scriptFunc, const std::vector<ScriptVariableTypeData>& expectedInputs,
			const std::vector<ScriptVariableTypeData>& expectedOutputs);
#endif // REMOVE_FROM_SCRIPTS_ENABLED

		void GetErrorsByComparingPins(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc, const std::vector<ScriptVariableTypeData>& expectedInputs,
			const std::vector<ScriptVariableTypeData>& expectedOutputs) const;

	private:
		struct CompareOutOfDataResult
		{
			bool mAreInputsOutOfDate{};
			bool mAreOutputsOutOfDate{};

			std::vector<std::reference_wrapper<const ScriptPin>> mPinsThatMustBeUnlinkedBeforeWeCanRefresh{};
		};
		CompareOutOfDataResult CheckIfOutOfDateByComparingPinType(const ScriptFunc& scriptFunc, const std::vector<ScriptVariableTypeData>& expectedInputs,
			const std::vector<ScriptVariableTypeData>& expectedOutputs) const;

		NodeId mId{};

		PinId mFirstPinId{};
		uint32 mNumOfInputs{};
		uint32 mNumOfOutputs{};

		ScriptNodeType mType{};

		glm::vec2 mPosition{};
	};
}

inline CE::Span<const CE::ScriptPin> CE::ScriptNode::GetInputs(const ScriptFunc& scriptFunc) const
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) : nullptr, mNumOfInputs };
}

inline CE::Span<CE::ScriptPin> CE::ScriptNode::GetInputs(ScriptFunc& scriptFunc)
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) : nullptr, mNumOfInputs };
}

inline CE::Span<const CE::ScriptPin> CE::ScriptNode::GetOutputs(const ScriptFunc& scriptFunc) const
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) + mNumOfInputs : nullptr, mNumOfOutputs };
}

inline CE::Span<CE::ScriptPin> CE::ScriptNode::GetOutputs(ScriptFunc& scriptFunc)
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) + mNumOfInputs : nullptr , mNumOfOutputs };
}

inline CE::Span<const CE::ScriptPin> CE::ScriptNode::GetPins(const ScriptFunc& scriptFunc) const
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) : nullptr, mNumOfInputs + mNumOfOutputs };
}

inline CE::Span<CE::ScriptPin> CE::ScriptNode::GetPins(ScriptFunc& scriptFunc)
{
	return { mFirstPinId.IsValid() ? &scriptFunc.GetPin(mFirstPinId) : nullptr, mNumOfInputs + mNumOfOutputs };
}
