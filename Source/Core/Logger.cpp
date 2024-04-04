#include "Precomp.h"
#include "Core/Logger.h"

#include <chrono>

#include "Core/FileIO.h"
#include "GSON/GSONReadable.h"
#include "Containers/ManyStrings.h"

static std::filesystem::path GetLogIniPath()
{
	return CE::FileIO::Get().GetPath(CE::FileIO::Directory::Intermediate, "Editor/Logger.ini");
}

static std::filesystem::path GetCrashLogDir()
{
	std::filesystem::path path = CE::FileIO::Get().GetPath(CE::FileIO::Directory::Intermediate, "Logs");
	std::filesystem::create_directories(path);
	return path;
}

CE::Logger::Logger() = default;

void CE::Logger::PostConstruct()
{
	mEntryContents = std::make_unique<ManyStrings>();

	ReadableGSONObject logIni{};

	std::ifstream ifstream{ GetLogIniPath() };

	if (ifstream.is_open())
	{
		logIni.LoadFrom(ifstream);
	}

	const ReadableGSONObject* channels = logIni.TryGetGSONObject("Channels");

	if (channels != nullptr)
	{
		for (const ReadableGSONMember& member : channels->GetGSONMembers())
		{
			member >> mChannels.emplace(Name::HashString(member.GetName()), Channel{ member.GetName() }).first->second.mEnabled;
		}
	}

	const ReadableGSONMember* severity = logIni.TryGetGSONMember("Severity");

	if (severity != nullptr)
	{
		*severity >> reinterpret_cast<std::underlying_type_t<decltype(mCurrentLogSeverity)>&>(mCurrentLogSeverity);
	}
}

CE::Logger::~Logger()
{
	const std::filesystem::path iniPath = GetLogIniPath();

	std::error_code err{};
	create_directories(GetLogIniPath().parent_path(), err);

	if (err)
	{
		printf_s("Failed to create directories %s, %s", iniPath.string().c_str(), err.message().c_str());
		return;
	}

	std::ofstream ofstream{ iniPath };

	if (!ofstream.is_open())
	{
		printf_s("Failed to write to ini %s", iniPath.string().c_str());
		return;
	}

	ReadableGSONObject logIni{};
	ReadableGSONObject& channels = logIni.AddGSONObject("Channels");
	
	for (const auto& [key, channel] : mChannels)
	{
		channels.AddGSONMember(channel.mName) << channel.mEnabled;
	}

	logIni.AddGSONMember("Severity") << static_cast<std::underlying_type_t<decltype(mCurrentLogSeverity)>>(mCurrentLogSeverity);

	logIni.SaveTo(ofstream);
	ofstream.close();
};

void CE::Logger::Log(std::string_view message, 
	const std::string_view channel, 
	const LogSeverity severity, 
	std::string_view file,
	uint32 line, 
	std::function<void()>&& onClick)
{
	Name::HashType channelHash = Name::HashString(channel);

	const std::string formattedMessage = Format("{}({})\n\t{}\n\n",
		std::filesystem::path{ file }.filename().string(), // Only the filename, not all that C:/projects nonsense
		line,
		message);

	mMutex.lock();
	++mNumOfEntriesPerSeverity[static_cast<int>(severity)];

	auto existingChannel = mChannels.find(channelHash);

	if (existingChannel == mChannels.end())
	{
		existingChannel = mChannels.emplace(channelHash, Channel{ std::string{ channel } }).first;
	}

	mEntries.emplace_back(existingChannel->second, severity, file, line, std::move(onClick));
	mEntryContents->Emplace(formattedMessage);
	mMutex.unlock();

	fputs(formattedMessage.c_str(), stdout);
	
	if (severity == Fatal)
	{
		DumpToCrashLog();
		throw std::runtime_error{ formattedMessage };
	}
}

void CE::Logger::Clear()
{
	mMutex.lock();
	mEntries.clear();
	mEntryContents->Clear();
	mNumOfEntriesPerSeverity = {};
	mMutex.unlock();
}

void CE::Logger::DumpToCrashLog() const
{
	const auto now = std::chrono::system_clock::now();
	const std::filesystem::path logPath = (GetCrashLogDir() / std::to_string(now.time_since_epoch().count())).replace_extension(".txt");

	std::ofstream file{ logPath };

	if (!file.is_open())
	{
		puts("Failed to dump to crashlog, as the file could not be opened");
	}

	file.write(mEntryContents->Data(), mEntryContents->SizeInBytes());
	file.close();
}
