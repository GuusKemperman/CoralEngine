#pragma once

namespace CE
{
	class World;
	class Level;

	struct BenchmarkParams
	{
		size_t mMinTotalDurationSeconds = 30;
		float mTickStepSize = 1.0f / 60.0f;
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
	BenchmarkResult BenchMark(std::string_view levelName, BenchmarkParams params);
}