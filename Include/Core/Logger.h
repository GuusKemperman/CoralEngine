#pragma once
#include "Core/EngineSubsystem.h"

#if 1

#define LOG(channel, severity, formatString, ...) Engine::Logger::Get().Log(Engine::Format("", __VA_ARGS__), #channel, severity, Engine::SourceLocation::current( __LINE__, __FILE__ ));
#else
#define LOG(...)
#endif

enum LogSeverity
{
	// To log what is currently being done, so that when the program
	// crashes, it is clear what the most recent action was that led
	// to the crash.
	Verbose,

	// General messages
	Message,

	// 
	Warning,

	// This was unexpected, there is something seriously wrong that the user should
	// be made aware off.
	Error,

	// This was completely unexpected and we cannot recover from this. The program will crash after logging this message.
	Fatal,

	NUM_OF_SEVERITIES
};

namespace Engine
{
	class ManyStrings;

	class Logger final :
		public EngineSubsystem<Logger>
	{
		friend class EngineSubsystem;
		Logger();
		~Logger();

		void PostConstruct();

	public:
		void Log(std::string_view message,
			std::string_view channel, 
			LogSeverity severity, 
			SourceLocation&& origin,
			std::function<void()>&& onMessageClick = {});

		void Clear();

		void DumpToCrashLog() const;

		LogSeverity GetCurrentSeverityLevel() const { return mCurrentLogSeverity; }
		void SetCurrentSeverityLevel(const LogSeverity severity) { mCurrentLogSeverity = severity; }

		const std::array<uint32, static_cast<size_t>(NUM_OF_SEVERITIES)>& GetNumOfEntriesPerSeverity() { return mNumOfEntriesPerSeverity; }

	private:
		friend class LogWindow;

		static inline constexpr std::array<glm::vec4, static_cast<size_t>(NUM_OF_SEVERITIES)> sDisplayColorOfLogSeverities
		{
			glm::vec4{.5f, .5f, .5f, 1.0f},
			glm::vec4{1.0f},
			glm::vec4{1.0f, 1.0f, 0.0f, 1.0f},
			glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
			glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}
		};

		static inline constexpr std::array<std::string_view, static_cast<size_t>(NUM_OF_SEVERITIES)> sDisplayNameOfLogSeverities
		{
			"Verbose",
			"Message",
			"Warning",
			"Error",
			"Fatal",
		};

		struct Channel
		{
			std::string mName{};
			bool mEnabled = true;

			bool operator<(const Channel& other) const { return mName < other.mName; }
		};

		LogSeverity mCurrentLogSeverity{};

		std::unordered_map<uint32, Channel> mChannels{};
		std::unique_ptr<ManyStrings> mEntryContents{};

		struct Entry
		{
			Entry(const Channel& channel, LogSeverity severity, SourceLocation&& origin, std::function<void()>&& onClick) :
				mChannel(channel),
				mSeverity(severity),
				mOrigin(std::move(origin)),
				mOnClick(std::move(onClick)){}
			std::reference_wrapper<const Channel> mChannel;
			LogSeverity mSeverity;
			SourceLocation mOrigin;
			std::function<void()> mOnClick;
		};
		std::vector<Entry> mEntries{};

		bool ShouldLog(const Entry& entry) const
		{
			return entry.mSeverity >= mCurrentLogSeverity && entry.mChannel.get().mEnabled;
		}

		std::array<uint32, static_cast<size_t>(NUM_OF_SEVERITIES)> mNumOfEntriesPerSeverity{};
	};
}
