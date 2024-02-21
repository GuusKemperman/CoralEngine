#include "Precomp.h"
#include "Utilities/Benchmark.h"

#include "World/World.h"
#include "Assets/Level.h"

using namespace std::chrono;

Engine::BenchmarkResult Engine::BenchMark(World& world, const BenchmarkParams params)
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

        result.mHighestDeltaTime = std::max(result.mHighestDeltaTime, dt);

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

Engine::BenchmarkResult Engine::BenchMark(const Level& level, const BenchmarkParams params)
{
    World world = level.CreateWorld(true);
    return BenchMark(world, params);
}

void Engine::BenchmarkResult::Print() const
{
    std::cout << Format("AvgDt (ms): {:.4}, Standard deviation (%) : {:.4},  HighestDt (ms): {:.4}, NumOfTicksCompleted: {}",
        static_cast<long double>(mAverageDeltaTime.count()) / 1e6,
        mRelativeStandardDeviation,
        static_cast<long double>(mHighestDeltaTime.count()) / 1e6,
		mDeltaTimes.size()) << std::endl;
}

void Engine::BenchmarkResult::ExportToCSV(std::ostream& ostream) const
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
