#pragma once

namespace EarlySTD
{
	struct source_location
	{
        static constexpr source_location current(const uint_least32_t line = __LINE__, const char* const file = __FILE__) noexcept
		{
            source_location _Result{};
            _Result._Line = line;
            _Result._Column = 0;
            _Result._File = file;
            _Result._Function = "Function name requires C++20 or higher";
            return _Result;
        }

        constexpr source_location() noexcept = default;

        constexpr uint_least32_t line() const noexcept {
            return _Line;
        }

		constexpr uint_least32_t column() const noexcept {
            return _Column;
        }

		constexpr const char* file_name() const noexcept {
            return _File;
        }

		constexpr const char* function_name() const noexcept {
            return _Function;
        }

        private:
            uint_least32_t _Line{};
            uint_least32_t _Column{};
            const char* _File = "";
            const char* _Function = "";
    };
}
