#pragma once
#include "Utilities/Imgui/ImguiInspect.h"
#include "GSON/GSONBinary.h"
#include "Meta/MetaFunc.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

namespace CE
{
	// Make sure it the type receives all the functions that a data field requires
	template<typename T>
	void ReflectFieldType(MetaType& type)
	{
		ASSERT(MakeTypeId<T>() == type.GetTypeId());

#ifdef EDITOR
		type.AddFunc(&ShowInspectUI<T>, sShowInspectUIFuncName.StringView());
#endif // EDITOR

		type.AddFunc([](BinaryGSONMember& m, const T& v) { m << v; }, sSerializeMemberFuncName.StringView());
		type.AddFunc([](const BinaryGSONMember& m, T& v) { m >> v; }, sDeserializeMemberFuncName.StringView());
		
		if (type.TryGetFunc(OperatorType::equal, MakeFuncId<bool(const T&, const T&)>()) != nullptr)
		{
			return;
		}

		MetaProps& equalFuncProps = type.AddFunc([](const T& lhs, const T& rhs) {  return lhs == rhs; }, OperatorType::equal).GetProperties();

		if (type.GetProperties().Has(Props::sIsScriptableTag))
		{
			equalFuncProps.Add(Props::sIsScriptableTag);
		}
	}
}
