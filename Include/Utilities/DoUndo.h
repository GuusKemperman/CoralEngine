#pragma once
#include <vector>
#include <memory>

namespace CE
{
	namespace DoUndo
	{
		class Action
		{
		public:
			virtual ~Action() = default;

			virtual void Do() = 0;
			virtual void Undo() = 0;
		};

		template<typename T = Action>
		class DoUndoStackBase
		{
		public:
			DoUndoStackBase() = default;

			DoUndoStackBase(DoUndoStackBase&&) noexcept = default;
			DoUndoStackBase(const DoUndoStackBase&) = delete;

			DoUndoStackBase& operator=(DoUndoStackBase&&) noexcept = default;
			DoUndoStackBase& operator=(const DoUndoStackBase&) = delete;

			template<typename ActionType = T, typename ...Args>
			ActionType& Do(Args&& ...args)
			{
				ClearRedo();

				auto action = std::make_unique<ActionType>(std::forward<Args>(args)...);
				ActionType& returnValue = *action;

				mActionsTaken.push_back(std::move(action));
				mNumOfActionsDone++;

				DoTopActionAgain();

				LOG(LogEditor, Verbose, "Added {} to DoUndoStack", typeid(ActionType).name());

				return returnValue;
			}

			void DoTopActionAgain()
			{
				T* mostRecent = PeekTop();

				if (mostRecent != nullptr)
				{
					mostRecent->Do();
					mTimeLastActionAdded = std::chrono::high_resolution_clock::now();
				}
			}

			bool CanUndo() const { return PeekTop() != nullptr; }
			bool CanRedo() const { return mNumOfActionsDone < mActionsTaken.size(); }

			void Undo()
			{
				T* mostRecent = PeekTop();

				if (mostRecent != nullptr)
				{
					LOG(LogEditor, Verbose, "Undoing action");
					mNumOfActionsDone--;
					mostRecent->Undo();
				}
			}

			void Redo()
			{
				if (CanRedo())
				{
					LOG(LogEditor, Verbose, "Redoing action");
					mNumOfActionsDone++;
					DoTopActionAgain();
				}
			}

			void Clear()
			{
				mActionsTaken.clear();
				mNumOfActionsDone = 0;
			}

			void ClearRedo()
			{
				mActionsTaken.resize(mNumOfActionsDone);
			}

			const T* PeekTop() const
			{
				if (mNumOfActionsDone == 0)
				{
					return nullptr;
				}
				ASSERT(mNumOfActionsDone <= mActionsTaken.size());
				
				return mActionsTaken[mNumOfActionsDone - 1].get();
			}

			T* PeekTop()
			{
				return const_cast<T*>(const_cast<const DoUndoStackBase<T>*>(this)->PeekTop());
			}

			static inline constexpr float sJustNowTreshold = 1.0f;

			float NumOfSecondsSinceLastActionAdded() const
			{
				const auto now = std::chrono::high_resolution_clock::now();
				return (std::chrono::duration_cast<std::chrono::duration<float>>(now - mTimeLastActionAdded)).count();
			}

			T* WhatDidWeJustDo()
			{
				T* top = PeekTop();

				if (top != nullptr
					&& NumOfSecondsSinceLastActionAdded() <= sJustNowTreshold)
				{
					return top;
				}
				return nullptr;
			}

			size_t GetNumOfActionsDone() const { return mNumOfActionsDone; }

			// Some of these actions may have been undone already!
			std::span<std::unique_ptr<T>> GetAllStoredActions() { return mActionsTaken; }
			std::span<const std::unique_ptr<T>> GetAllStoredActions() const { return mActionsTaken; }

		private:
			std::vector<std::unique_ptr<T>> mActionsTaken{};
			std::chrono::high_resolution_clock::time_point mTimeLastActionAdded{};
			size_t mNumOfActionsDone{};
		};

		using DoUndoStack = DoUndoStackBase<Action>;
	}
}