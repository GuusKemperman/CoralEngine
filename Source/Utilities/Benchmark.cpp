#include "Precomp.h"
#include "Utilities/Benchmark.h"

#include "World/World.h"
#include "Assets/Level.h"
#include "Core/AssetManager.h"

using namespace std::chrono;

CE::BenchmarkResult CE::BenchMark(World& world, const BenchmarkParams params)
{
    BenchmarkResult result{};
    result.mBenchmarkParams = params;

    result.mDeltaTimes.reserve(2'048);

    // First tick to warm the caches
    world.Tick(params.mTickStepSize);

    const auto startTime = high_resolution_clock::now();

    // Create a duration using the seconds
    std::chrono::seconds duration(params.mMinTotalDurationSeconds);

    // Add the duration to the current time point
    const auto endTime = startTime + duration;

    auto now = high_resolution_clock::now();

    std::chrono::nanoseconds totalTimeElapsed{};

    while (now < endTime)
    {
        world.Tick(params.mTickStepSize);

        auto newNow = high_resolution_clock::now();
        auto dt = newNow - now;

        totalTimeElapsed += dt;

        result.mDeltaTimes.emplace_back(dt);

        // std::max does not work for prospero with these types
    	result.mHighestDeltaTime = result.mHighestDeltaTime > dt ? result.mHighestDeltaTime : dt;

        now = newNow;
    }

    long long numOfSteps = static_cast<long long>(result.mDeltaTimes.size());
    result.mAverageDeltaTime = totalTimeElapsed / numOfSteps;


    const long double floatingAvgDeltaTimeFloating = static_cast<long double>(result.mAverageDeltaTime.count());
    long double totalSqrdDistances{};
	for (const nanoseconds deltaTime : result.mDeltaTimes)
    {
        const long double distToMean = floatingAvgDeltaTimeFloating - static_cast<long double>(deltaTime.count());
        const long double sqrdDistToMean = distToMean * distToMean;
        totalSqrdDistances += sqrdDistToMean;
    }
    long double avgSqrdDistance = totalSqrdDistances / static_cast<long double>(numOfSteps);

    const long double standardDeviation = std::sqrt(avgSqrdDistance);

    result.mRelativeStandardDeviation = (standardDeviation / floatingAvgDeltaTimeFloating) * 100.0;

    return result;
}

CE::BenchmarkResult CE::BenchMark(std::string_view levelName, BenchmarkParams params)
{
    AssetHandle<Level> level = AssetManager::Get().TryGetAsset<Level>(levelName);

    if (level == nullptr)
    {
        LOG(LogCore, Error, "Cannot benchmark {}, as it does not exist", levelName);
        return {};
    }

	World world = level->CreateWorld(true);
    return BenchMark(world, params);
}

void CE::BenchmarkResult::Print() const
{
    std::cout << Format("AvgDt (ms): {:.4}, Standard deviation (%) : {:.4},  HighestDt (ms): {:.4}, NumOfTicksCompleted: {}",
        static_cast<long double>(mAverageDeltaTime.count()) / 1e6,
        mRelativeStandardDeviation,
        static_cast<long double>(mHighestDeltaTime.count()) / 1e6,
		mDeltaTimes.size()) << std::endl;
}

void CE::BenchmarkResult::ExportToCSV(std::ostream& ostream) const
{
    if (mDeltaTimes.empty())
    {
        return;
    }

    for (size_t i = 0; i < mDeltaTimes.size(); i++)
    {
        ostream << mDeltaTimes[i].count() << ",\n";
    }
    ostream << std::endl;
}
