#include "Precomp.h"
#include "Core/UnitTests.h"

#include "Core/AssetManager.h"
#include "GSON/GSONReadable.h"
#include "Core/FileIO.h"
#include "Core/ThreadPool.h"

static constexpr std::string_view sPathToTestResults = "UnitTestResults.txt";

static std::vector<CE::UnitTest>& GetTests()
{
	static std::vector<CE::UnitTest> tests{};
	return tests;
}

// Only works on windows
static std::chrono::system_clock::time_point  FileTimeToSysTime(std::filesystem::file_time_type f_tp)
{
	using namespace std::literals;
	return std::chrono::system_clock::time_point{ f_tp.time_since_epoch() - 3'234'576h };
}

static std::chrono::system_clock::time_point GetTimeOfCompilation()
{
	static auto time = []() -> std::chrono::system_clock::time_point
		{
			const std::filesystem::path ourExecutable = CE::FileIO::Get().GetPath(CE::FileIO::Directory::ThisExecutable, {});

			if (!std::filesystem::exists(ourExecutable))
			{
				LOG(LogUnitTest, Error, "Failed to determine date of compilation, {} does not exist", ourExecutable.string());
				return {};
			}

			std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(ourExecutable);
			return FileTimeToSysTime(writeTime);
		}();
	return time;
}

void CE::UnitTestManager::PostConstruct()
{
	ReadableGSONObject object{};

	std::ifstream resultFile{ FileIO::Get().GetPath(FileIO::Directory::Intermediate, std::string{sPathToTestResults}) };

	if (!resultFile.is_open())
	{
		LOG(LogUnitTest, Verbose, "Could not load result of tests, file not open");
		return;
	}

	object.LoadFrom(resultFile);

	const auto timeOfCompilation = GetTimeOfCompilation();
	[[maybe_unused]] const auto now = std::chrono::system_clock::now();


	std::vector<UnitTest>& tests = GetTests();

	for (const ReadableGSONObject& child : object.GetChildren())
	{
		auto test = std::find_if(tests.begin(), tests.end(),
			[&child](const UnitTest& test)
			{
				return test.mName == child.GetName();
			});

		if (test == tests.end())
		{
			continue;
		}

		const ReadableGSONMember* timeMember = child.TryGetGSONMember("time");
		const ReadableGSONMember* resultMember = child.TryGetGSONMember("result");
		const ReadableGSONMember* durationMember = child.TryGetGSONMember("duration");

		if (timeMember == nullptr
			|| resultMember == nullptr
			|| durationMember == nullptr)
		{
			LOG(LogUnitTest, Error, "Unit test file was corrupted");
			continue;
		}

		long long timeSinceEpoch{};
		*timeMember >> timeSinceEpoch;
		test->mTimeLastRan = std::chrono::system_clock::time_point{ std::chrono::microseconds{ timeSinceEpoch } };

		long long duration{};
		*durationMember >> duration;
		test->mLastTestDuration = std::chrono::milliseconds{ duration };

		int tmp{};
		*resultMember >> tmp;
		test->mASyncState->mResult = static_cast<UnitTest::Result>(tmp);

		test->mASyncState->mResult &= ~UnitTest::OutDated;

		// The test has been ran before, let's check if the result is up to date
		if ((test->mASyncState->mResult & UnitTest::NotRan) == 0
			&& test->mTimeLastRan < timeOfCompilation)
		{
			test->mASyncState->mResult |= UnitTest::OutDated;
		}
	}
}

CE::UnitTestManager::~UnitTestManager()
{
	ReadableGSONObject object{};

	for (UnitTest& test : GetTests())
	{
		test.CancelIfRunning();

		ReadableGSONObject& result = object.AddGSONObject(test.mName);
		result.AddGSONMember("result") << static_cast<int>(test.GetResult());
		result.AddGSONMember("time") << std::chrono::duration_cast<std::chrono::microseconds>(test.mTimeLastRan.time_since_epoch()).count();
		result.AddGSONMember("duration") << test.mLastTestDuration.count();
	}

	std::ofstream resultFile{ FileIO::Get().GetPath(FileIO::Directory::Intermediate, std::string{sPathToTestResults}) };

	if (!resultFile.is_open())
	{
		LOG(LogUnitTest, Error, "Could not save result of tests!");
		return;
	}

	object.SaveTo(resultFile);
}

void CE::UnitTestManager::RunTests(UnitTest::Result resultFlags)
{
	RunTestsAsync(resultFlags);

	for (UnitTest& test : GetTests())
	{
		test.WaitUntilFinished();
	}
}

void CE::UnitTestManager::RunTestsAsync(UnitTest::Result resultFlags)
{
	for (UnitTest& test : GetTests())
	{
		if (test.GetResult() & resultFlags)
		{
			test.RunASync();
		}
	}
}

std::span<CE::UnitTest> CE::UnitTestManager::GetAllTests()
{
	return GetTests();
}

bool CE::Internal::RegisterUnitTest(std::string_view category, std::string_view name, std::function<void()>&& function)
{
	std::vector<UnitTest>& tests = GetTests();
	UnitTest test{ category, name, std::move(function) };

	static constexpr auto sortByName =
		[](const UnitTest& lhs, const UnitTest& rhs)
		{
			if (lhs.GetCategory() == rhs.GetCategory())
			{
				return lhs.GetName() < rhs.GetName();
			}
			return lhs.GetCategory() < rhs.GetCategory();
		};

	auto whereToInsert = std::upper_bound(tests.begin(), tests.end(), test, sortByName);
	tests.insert(whereToInsert, std::move(test));

	return true;
}

void CE::UnitTest::RunASync()
{
	if (GetResult() & (WaitingForThread | Running))
	{
		return;
	}

	Clear();

	mASyncState->mResult = WaitingForThread;
	mASyncState->mPendingFuture = ThreadPool::Get().Enqueue([this]()
		{
			if (mASyncState->mResult != WaitingForThread)
			{
				return;
			}

			mASyncState->mResult = Running;

			mTimeLastRan = std::chrono::system_clock::now();
			LOG(LogUnitTests, Message, "Running {}::{}", mCategory, mName);

			try
			{
				mFunc();
				mASyncState->mResult = Success;
			}
			catch (const std::exception& e)
			{
				LOG(LogUnitTest, Error, "Unit test {} threw exception - {}", mName, e.what());
				throw Failure;
			}
			catch (Result result)
			{
				mASyncState->mResult = result;
				LOG(LogUnitTest, Error, "Unit test {} failed", mName);
			}
			catch (...)
			{
				LOG(LogUnitTest, Error, "Unit test {} threw unknown exception", mName);
				mASyncState->mResult = Failure;
			}

			LOG(LogUnitTests, Message, "Finished {}::{}", mCategory, mName);

			mLastTestDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - mTimeLastRan);
		});
}

void CE::UnitTest::WaitUntilFinished() const
{
	if (mASyncState->mPendingFuture.valid())
	{
		mASyncState->mPendingFuture.get();
	}
}

void CE::UnitTest::CancelIfRunning()
{
	if (GetResult() & WaitingForThread)
	{
		Clear();
	}
}

void CE::UnitTest::Run()
{
	RunASync();
	WaitUntilFinished();
}

void CE::UnitTest::Clear()
{
	mASyncState->mResult = NotRan;
	WaitUntilFinished();
	mTimeLastRan = {};
	mLastTestDuration = {};
}
