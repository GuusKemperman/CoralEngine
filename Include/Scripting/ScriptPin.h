#pragma once
#include "ScriptIds.h"
#include "imnodes/imgui_node_editor.h"

#include "Meta/MetaAny.h"
#include "Meta/MetaTypeTraits.h"
#include "Meta/Fwd/MetaFuncFwd.h"
#include "Scripting/ScriptErrors.h"

namespace CE
{
	struct MetaFuncNamedParam;
}

namespace CE
{
	enum class ScriptPinKind : bool
	{
		Output,
		Input,
	};

	class ScriptFunc;
	class ScriptVariableTypeData;
	class ScriptNode;
	class BinaryGSONObject;

	class ScriptVariableTypeData
	{
		static constexpr std::string_view sVoidName = "None";
	public:
		ScriptVariableTypeData() = default;
		ScriptVariableTypeData(TypeTraits typeTraits, std::string_view name = {});
		ScriptVariableTypeData(const MetaFuncNamedParam& metaParam) : ScriptVariableTypeData(metaParam.mTypeTraits, metaParam.mName) {}
		ScriptVariableTypeData(std::string_view typeName, TypeForm typeForm, std::string_view name = {});
		ScriptVariableTypeData(const MetaType& type, TypeForm typeForm, std::string_view name = {});

		// May be nullptr if the type has been deleted/renamed
		const MetaType* TryGetType() const { return mType; }

		TypeForm GetTypeForm() const { return mTypeForm; }
		void SetTypeForm(TypeForm typeForm) { mTypeForm = typeForm; }

		const std::string& GetName() const { return mName.empty() && !IsFlow() ? GetTypeName() : mName; }
		void SetName(std::string_view name) { mName = std::string{ name }; }

		const std::string& GetTypeName() const { return mTypeName; }

		bool IsFlow() const;

		bool operator==(const ScriptVariableTypeData& other) const { return mTypeName == other.mTypeName && mName == other.mName && mType == other.mType && mTypeForm == other.mTypeForm; };
		bool operator!=(const ScriptVariableTypeData& other) const { return mTypeName != other.mTypeName || mName != other.mName || mType != other.mType || mTypeForm != other.mTypeForm; };

		void RefreshTypePointer();

	private:
		std::string mTypeName{};
		std::string mName{};
		const MetaType* mType{};
		TypeForm mTypeForm{ TypeForm::Value };
	};

	class ScriptPin
	{

		ScriptPin(NodeId nodeId) :
			mNodeId(nodeId) {}

	public:
		struct Flow { };
		static constexpr TypeTraits sFlow = MakeTypeTraits<Flow>();
		static constexpr std::string_view sFlowDisplayName = "Flow";

		ScriptPin(PinId id, NodeId nodeId, ScriptPinKind kind, const ScriptVariableTypeData& variableTypeInfo);

		PinId GetId() const { return mId; }
		// Use with care, dangerous.
		void SetId(PinId id) { mId = id; }

		NodeId GetNodeId() const { return mNodeId; }

		ScriptPinKind GetKind() const { return static_cast<ScriptPinKind>(mData.index()); }

		// Is nullptr if this is a flow pin,
		// or if the type has been renamed/deleted
		const MetaType* TryGetType() const { return mParamInfo.TryGetType(); }
		TypeForm GetTypeForm() const { return mParamInfo.GetTypeForm(); }
		const std::string& GetName() const { return mParamInfo.GetName(); }
		const std::string& GetTypeName() const { return mParamInfo.GetTypeName(); }
		const ScriptVariableTypeData& GetParamInfo() const { return mParamInfo; }

		void SetName(std::string_view name) { mParamInfo.SetName(name); }

		uint32 HowManyLinksCanBeConnectedToThisPin() const;

		bool IsInput() const { return GetKind() == ScriptPinKind::Input; }
		bool IsOutput() const { return GetKind() == ScriptPinKind::Output; }
		bool IsFlow() const { return mParamInfo.IsFlow(); }

		// Will return a value that is passed as input into the node for when 
		// there is no link connecting this input pin to an output pin
		// Will return nullptr if this is an output node
		MetaAny* TryGetValueIfNoInputLinked();

		const MetaAny* TryGetValueIfNoInputLinked() const;

		void SetValueIfNoInputLinked(MetaAny&& value);

		void SerializeTo(BinaryGSONObject& dest) const;
		static std::optional<ScriptPin> DeserializeFrom(const BinaryGSONObject& src, const ScriptNode& node, uint32 version);

		void CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const;

		// This is used to remap the IDs when duplicating a selection of nodes.
		void GetReferencesToIds(ScriptIdInserter inserter) { inserter = mId; inserter = mNodeId; inserter = mCachedPinWeAreLinkedWith; }

		PinId GetCachedPinWeAreLinkedWith() const { return mCachedPinWeAreLinkedWith; }
		void SetCachedPinWeAreLinkedWith(PinId id) { mCachedPinWeAreLinkedWith = id; }

		bool IsLinked() const { return mCachedPinWeAreLinkedWith != PinId::Invalid; }

		void PostDeclarationRefresh();

		glm::vec2 GetInspectWindowSize() const { ASSERT(IsInput()) return std::get<1>(mData).mInspectWindowSize; }
		void SetInspectWindowSize(glm::vec2 size) { ASSERT(IsInput()) std::get<1>(mData).mInspectWindowSize = size; }

	private:
		PinId mId{};
		NodeId mNodeId{};
		PinId mCachedPinWeAreLinkedWith{};

		ScriptVariableTypeData mParamInfo{};

		struct InputData
		{
			// If no link is attached, this value is passed into the node.
			// But if this does not hold a value, the default parameter value will be used
			//
			// When deserializing the pin, the first set the std::string to the serialized value.
			// Only on CacheDefaultValue do we load it in. This solves a lot of problems with
			// dependencies, since the serialized value may be another script, even the one
			// this pin is from.
			std::variant<std::monostate, std::string, MetaAny> mValueToUseIfNotLinked{};

			// How big of an area is needed to display the inspect UI in the editor.
			glm::vec2 mInspectWindowSize{};
		};

		using OutputData = std::monostate;

		std::variant<OutputData, InputData> mData{};


	};
}

#include "Utilities/BinarySerialization.h"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;

	inline void save(BinaryOutputArchive& ar, const CE::ScriptVariableTypeData& param)
	{
		ar(param.GetName());
		ar(param.GetTypeName());
		ar(static_cast<std::underlying_type_t<CE::TypeForm>>(param.GetTypeForm()));
	}

	inline void load(BinaryInputArchive& ar, CE::ScriptVariableTypeData& param)
	{
		std::string paramName{};
		std::string typeName{};
		std::underlying_type_t<CE::TypeForm> typeForm{};
		ar(paramName, typeName, typeForm);
		param = CE::ScriptVariableTypeData{ typeName, static_cast<CE::TypeForm>(typeForm), paramName };
	}
}