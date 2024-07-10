#pragma once
#include "Meta/MetaTypeId.h"
#include "ScriptErrors.h"
#include "Scripting/ScriptPin.h"

namespace CE
{
	class Script;
	class BinaryGSONObject;
	class BinaryGSONMember;
	class MetaAny;

	class ScriptField
	{
	public:
		ScriptField(const Script& owningScript, std::string_view name);

		ScriptField(ScriptField&& other) noexcept;
		ScriptField(const ScriptField&) = delete;

		ScriptField& operator=(ScriptField&& other) noexcept;
		ScriptField& operator=(const ScriptField&) = delete;

		~ScriptField();

		const MetaType* TryGetType() const { return mTypeData.TryGetType(); }
		const std::string& GetTypeName() const { return mTypeData.GetTypeName(); }
		const std::string& GetName() const { return mTypeData.GetName(); }

		void SetName(const std::string_view name) { mTypeData.SetName(name); }
		void SetType(const MetaType& type);

		const std::string& GetNameOfScriptAsset() const { return mNameOfScriptAsset; }

		static bool CanTypeBeUsedForFields(const MetaType& type);

		void SerializeTo(BinaryGSONObject& object) const;
		static std::optional<ScriptField> DeserializeFrom(const BinaryGSONObject& object, const Script& owningScript, uint32 version);

		void CollectErrors(ScriptErrorInserter inserter, const Script& owningScript) const;

		MetaAny& GetDefaultValue();
		const MetaAny& GetDefaultValue() const;

		bool WasValueDefaultConstructed() const;

	private:
		ScriptVariableTypeData mTypeData{};

		std::string mNameOfScriptAsset{};

		// Is initially a string containing the serialized default value.
		// When the value is needed, it is deserialized into the string.
		std::variant<std::string, std::unique_ptr<MetaAny>> mDefaultValue{};
	};
}
