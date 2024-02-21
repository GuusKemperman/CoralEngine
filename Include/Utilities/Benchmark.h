#pragma once

namespace Engine
{
	class World;
	class Level;

	struct BenchmarkParams
	{
		size_t mMinTotalDurationSeconds{};
		float mTickStepSize{};
	};

	struct BenchmarkResult
	{
		void Print() const;
		void ExportToCSV(std::ostream& ostream) const;

		std::chrono::nanoseconds mHighestDeltaTime{};
		std::chrono::nanoseconds mAverageDeltaTime{};
		long double mRelativeStandardDeviation{};

		std::vector<std::chrono::nanoseconds> mDeltaTimes{};
		BenchmarkParams mBenchmarkParams{};
	};

	BenchmarkResult BenchMark(World& world, BenchmarkParams params);
	BenchmarkResult BenchMark(const Level& level, BenchmarkParams params);
}