#pragma once
#include "Core/EngineSubsystem.h"

// To borrow the CONCAT macro
#include <future>

#include "Meta/MetaReflect.h"

namespace CE
{
	class UnitTest
	{
		friend class UnitTestManager;
	public:
		UnitTest(std::string_view category, std::string_view name, std::function<void()>&& result) :
			mCategory(category),
			mName(name),
			mFunc(std::move(result))
		{
		}

		enum Result
		{
			NotRan = 1,
			Failure = 1 << 1,
			Success = 1 << 2,

			// The test was run on an older version
			OutDated = 1 << 3,
			WaitingForThread = 1 << 4,
			Running = 1 << 5,
			OutDatedAndFailed = Failure | OutDated,
			OutDatedAndSuccess = Success | OutDated,
			All = NotRan | Failure | Success | OutDated
		};

		void RunASync();
		void WaitUntilFinished() const;
		void CancelIfRunning();

		void Run();

		void Clear();


		Result GetResult() const { return static_cast<Result>(mASyncState->mResult.load()); }

		std::string_view GetCategory() const { return mCategory; }
		std::string_view GetName() const { return mName; }
		std::chrono::system_clock::time_point GetTimeLastRan() const { return mTimeLastRan; }
		std::chrono::milliseconds GetLastTestDuration() const { return mLastTestDuration; }

	private:
		struct ASyncState
		{
			std::atomic<int> mResult = NotRan;
			std::future<void> mPendingFuture{};
		};
		std::unique_ptr<ASyncState> mASyncState = std::make_unique<ASyncState>();

		std::string_view mCategory{};
		std::string_view mName{};
		std::function<void()> mFunc{};
		std::chrono::system_clock::time_point mTimeLastRan{};
		std::chrono::milliseconds mLastTestDuration{};
	};

	class UnitTestManager final :
		public EngineSubsystem<UnitTestManager>
	{
		friend EngineSubsystem;
		void PostConstruct() override;
		~UnitTestManager();
	
	public:
		void RunTests(UnitTest::Result resultFlags);

		void RunTestsAsync(UnitTest::Result resultFlags);

		std::span<UnitTest> GetAllTests();
	};

	namespace Internal
	{
		bool RegisterUnitTest(std::string_view name, std::string_view category, std::function<void()>&& function);
	}
}

#define INIT_DUMMY_VAR(Category, TestName)												\
[[maybe_unused]] inline bool CONCAT(__sTestDummyVariable, CONCAT(Category, TestName)) = \
CE::Internal::RegisterUnitTest(#Category,												\
	#TestName,																			\
	&(TestName));	

#define UNIT_TEST_DECLARATION(Category, TestName)	\
void TestName();					\
INIT_DUMMY_VAR(Category, TestName)					\

#define UNIT_TEST(Category, TestName)		\
UNIT_TEST_DECLARATION(Category, TestName)	\
void TestName()

#define TEST_FAILURE(FormatString, ...) LOG(UnitTests, Error, FormatString, ##__VA_ARGS__); throw CE::UnitTest::Failure
#define TEST_ASSERT(Condition) if (!(Condition)) { TEST_FAILURE("{} evaluated to false", #Condition); } static_assert(true)
#define TEST_EQUAL(Lhs, Rhs) TEST_ASSERT((Lhs) == (Rhs))
#define TEST_NOT_EQUAL(Lhs, Rhs) TEST_ASSERT((Lhs) != (Rhs))
#define TEST_NOT_NULL(Ptr) TEST_ASSERT((Ptr) != nullptr)