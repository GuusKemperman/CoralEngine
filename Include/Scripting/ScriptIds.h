#pragma once
#include "imnodes/imgui_node_editor.h"

namespace Engine
{
	namespace Internal
	{
		template<typename AxType, size_t Shift>
		struct IdBase
		{
		private:
			struct InvalidT {};
		public:
			static constexpr InvalidT Invalid{};
			using ValueType = uint32;

			static constexpr size_t sShift = Shift;

			constexpr IdBase() = default;
			constexpr IdBase(InvalidT) {};

			constexpr IdBase(AxType axId) :
				mValue(static_cast<ValueType>(axId.Get() >> Shift))
			{
				ASSERT(static_cast<AxType>(*this) == axId && "Too high of a an Id");
			}

			constexpr IdBase(ValueType value) :
				mValue(value)
			{
				ASSERT(IdBase(static_cast<AxType>(*this)).mValue == value && "Too high of a an Id");
			}

			constexpr operator AxType() const { return { static_cast<uintptr>(mValue) << Shift }; };

			constexpr bool operator==(InvalidT) { return !IsValid(); }

			constexpr bool operator==(IdBase<AxType, Shift> other) const { return other.Get() == Get(); }
			constexpr bool operator!=(IdBase<AxType, Shift> other) const { return other.Get() != Get(); }
			constexpr bool operator<(IdBase<AxType, Shift> other) const { return other.Get() < Get(); }
			constexpr bool operator>(IdBase<AxType, Shift> other) const { return other.Get() > Get(); }

			constexpr bool IsValid() const { return mValue != 0; }

			constexpr ValueType Get() const { return mValue; }

		private:
			ValueType mValue{};
		};
	}

	struct NodeId :
		Internal::IdBase<ax::NodeEditor::NodeId, ((sizeof(uintptr) * 8) / 3) * 2>
	{
		using IdBase::IdBase;
	};

	struct LinkId :
		Internal::IdBase<ax::NodeEditor::LinkId, (sizeof(uintptr) * 8) / 3>
	{
		using IdBase::IdBase;
	};

	struct PinId :
		Internal::IdBase<ax::NodeEditor::PinId, 0>
	{
		using IdBase::IdBase;
	};

	using ScriptIdInserter = std::back_insert_iterator<std::vector<std::variant<std::reference_wrapper<LinkId>, std::reference_wrapper<PinId>, std::reference_wrapper<NodeId>>>>;
}

template<>
struct std::hash<Engine::NodeId>
{
	std::size_t operator()(const Engine::NodeId& k) const
	{
		return static_cast<size_t>(k.Get()) << Engine::NodeId::sShift;;
	}
};

template<>
struct std::hash<Engine::LinkId>
{
	std::size_t operator()(const Engine::LinkId& k) const
	{
		return static_cast<size_t>(k.Get()) << Engine::LinkId::sShift;;
	}
};

template<>
struct std::hash<Engine::PinId>
{
	std::size_t operator()(const Engine::PinId& k) const
	{
		return static_cast<size_t>(k.Get()) << Engine::PinId::sShift;;
	}
};

