#pragma once
#include "Core/EngineSubsystem.h"

// To borrow the CONCAT macro
#include "Meta/MetaReflect.h"

namespace Engine
{
	struct UnitTest
	{
		enum Result
		{
			NotRan = 1,
			Failure = 1 << 1,
			Success = 1 << 2,

			// The test was run on an older version
			OutDated = 1 << 3,
			OutDatedAndFailed = Failure | OutDated,
			OutDatedAndSuccess = Success | OutDated,
			All = NotRan | Failure | Success | OutDated
		};

		UnitTest(std::string&& category, std::string&& name, std::function<Result()>&& result) :
			mCategory(std::move(category)),
			mName(std::move(name)),
			mFunc(std::move(result))
		{
		}

		void operator()();

		void Clear();

		std::string mCategory{};
		std::string mName{};
		std::function<Result()> mFunc{};
		int mResult = NotRan;
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

		Span<UnitTest> GetAllTests();
	};

	namespace Internal
	{
		bool RegisterUnitTest(std::string_view name, std::string_view category, std::function<UnitTest::Result()>&& function);
	}
}

#define UNIT_TEST(Category, TestName)																																			\
Engine::UnitTest::Result TestName()

#define UNIT_TEST_DECLARATION(Category, TestName)																																			\
Engine::UnitTest::Result TestName();																																		\
[[maybe_unused]] static inline const bool CONCAT(__sTestDummyVariable, CONCAT(Category, TestName)) = Engine::Internal::RegisterUnitTest(#Category, #TestName, &(TestName));		\

#define TEST_ASSERT(Condition) if (!(Condition)) { LOG(UnitTests, Error, "{} evaluated to false", #Condition); return UnitTest::Failure; }; 

