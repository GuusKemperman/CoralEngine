#include "Precomp.h"
#include "Core/UnitTests.h"

#include "Core/AssetManager.h"
#include "GSON/GSONReadable.h"
#include "Core/FileIO.h"

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
	std::sort(tests.begin(), tests.end(),
		[](const UnitTest& lhs, const UnitTest& rhs)
		{
			return lhs.mCategory < rhs.mCategory;
		});

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
		test->mResult = static_cast<UnitTest::Result>(tmp);

		test->mResult &= ~UnitTest::OutDated;

		// The test has been ran before, let's check if the result is up to date
		if ((test->mResult & UnitTest::NotRan) == 0
			&& test->mTimeLastRan < timeOfCompilation)
		{
			test->mResult |= UnitTest::OutDated;
		}
	}
}

CE::UnitTestManager::~UnitTestManager()
{
	ReadableGSONObject object{};

	for (const UnitTest& test : GetTests())
	{
		ReadableGSONObject& result = object.AddGSONObject(test.mName);
		result.AddGSONMember("result") << static_cast<int>(test.mResult);
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
	for (UnitTest& test : GetTests())
	{
		if (test.mResult & resultFlags)
		{
			test();
		}
	}
}

std::span<CE::UnitTest> CE::UnitTestManager::GetAllTests()
{
	return GetTests();
}

bool CE::Internal::RegisterUnitTest(std::string_view category, std::string_view name, std::function<UnitTest::Result()>&& function)
{
	GetTests().emplace_back(std::string{category}, std::string{name}, std::move(function));
	return true;
}

void CE::UnitTest::operator()()
{
	mTimeLastRan = std::chrono::system_clock::now();
	LOG(LogUnitTests, Message, "Running {}::{}", mCategory, mName);
	mResult = mFunc();

	if (mResult == Failure)
	{
		LOG(LogUnitTest, Error, "Unit test {} failed", mName);
	}

	LOG(LogUnitTests, Message, "Finished {}::{}", mCategory, mName);

	mLastTestDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - mTimeLastRan);
}

void CE::UnitTest::Clear()
{
	mResult = NotRan;
	mTimeLastRan = {};
	mLastTestDuration = {};
}
