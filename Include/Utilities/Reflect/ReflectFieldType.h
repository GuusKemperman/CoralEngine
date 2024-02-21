#pragma once
#include "Utilities/Imgui/ImguiInspect.h"
#include "GSON/GSONBinary.h"
#include "Meta/MetaFunc.h"
#include "Meta/MetaType.h"

namespace Engine
{
	// Make sure it the type receives all the functions that a data field requires
	template<typename T>
	void ReflectFieldType(MetaType& type)
	{
#ifdef EDITOR
		type.AddFunc(&ShowInspectUI<T>, sShowInspectUIFuncName.StringView());
#endif // EDITOR

		type.AddFunc(&BinaryGSONMember::operator<<<T>, sSerializeMemberFuncName.StringView());
		type.AddFunc(&BinaryGSONMember::operator>><T>, sDeserializeMemberFuncName.StringView());
		
		if (type.TryGetFunc(OperatorType::equal, MakeFuncId<bool(const T&, const T&)>()) == nullptr)
		{
			type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{});
		}
	}
}
