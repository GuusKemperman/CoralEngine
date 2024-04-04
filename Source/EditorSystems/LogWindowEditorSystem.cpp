#include "Precomp.h"
#include "EditorSystems/LogWindowEditorSystem.h"

#include "Containers/ManyStrings.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

CE::LogWindow::LogWindow() :
	EditorSystem("LogWindow")
{
}

CE::LogWindow::~LogWindow() = default;

void CE::LogWindow::Tick(const float)
{
	if (Begin(mFlags))
	{
		DisplayMenuBar();
		DisplayWindowContents();
	}

	End();
}

void CE::LogWindow::SaveState(std::ostream& toStream) const
{
	toStream << mAutoScroll;
}

void CE::LogWindow::LoadState(std::istream& fromStream)
{
	fromStream >> mAutoScroll;
}

void CE::LogWindow::DisplayMenuBar()
{
	if (!ImGui::BeginMenuBar())
	{
		return;
	}

	Logger& logger = Logger::Get();

	if (ImGui::BeginMenu("Channels"))
	{
		std::unordered_map<uint32, Logger::Channel>& channels = logger.mChannels;
		for (auto& [key, channel] : channels)
		{
			if (ImGui::MenuItem(channel.mName.c_str(), nullptr, channel.mEnabled))
			{
				channel.mEnabled = !channel.mEnabled;
			}
		}

		ImGui::EndMenu();
	}

	const LogSeverity severity = logger.GetCurrentSeverityLevel();

	if (ImGui::BeginMenu("Severity"))
	{
		static_assert(std::is_same_v<int, std::underlying_type_t<LogSeverity>>);
		for (int i = 0; i < static_cast<int>(NUM_OF_SEVERITIES); i++)
		{
			bool selected = severity == static_cast<LogSeverity>(i);
			if (ImGui::MenuItem(Logger::sDisplayNameOfLogSeverities[i].data(), nullptr, &selected))
			{
				logger.SetCurrentSeverityLevel(static_cast<LogSeverity>(i));
			}
		}

		ImGui::EndMenu();
	}

	if (ImGui::SmallButton("Clear"))
	{
		logger.Clear();
	}

	if (ImGui::RadioButton("Autoscroll", mAutoScroll))
	{
		mAutoScroll = !mAutoScroll;
	}

	static_assert(std::is_same_v<int, std::underlying_type_t<LogSeverity>>);
	for (int i = 0; i < static_cast<int>(NUM_OF_SEVERITIES) - 1; i++)
	{
		ImGui::TextColored(Logger::sDisplayColorOfLogSeverities[i], "%u %s\t", logger.GetNumOfEntriesPerSeverity()[i], Logger::sDisplayNameOfLogSeverities[i].data());
	}

	ImGui::EndMenuBar();
}

void CE::LogWindow::DisplayWindowContents()
{
	const Logger& logger = Logger::Get();

	const ManyStrings& entryContents = *logger.mEntryContents;
	ImGui::PushTextWrapPos(0.0f);

	for (uint32 i = 0; i < static_cast<uint32>(logger.mEntries.size()); i++)
	{
		const Logger::Entry& entry = logger.mEntries[i];

		if (!logger.ShouldLog(entry))
		{
			continue;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, Logger::sDisplayColorOfLogSeverities[static_cast<int>(entry.mSeverity)]);

		const std::string_view text = entryContents[i];

		if (entry.mOnClick)
		{
			ImGui::PushID(i);

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->ChannelsSplit(2);

			// Channel number is like z-order. Widgets in higher channels are rendered above widgets in lower channels.
			drawList->ChannelsSetCurrent(1);

			if (ImGui::Selectable(text.data(), false))
			{
				entry.mOnClick();
			}

			ImVec2 p_min = ImGui::GetItemRectMin();
			ImVec2 p_max = ImGui::GetItemRectMax();

			drawList->ChannelsSetCurrent(0);
			drawList->AddRectFilled(p_min, p_max, ImColor(1.0f, 1.0f, 1.0f, .05f));

			drawList->ChannelsMerge();

			ImGui::PopID();
		}
		else
		{
			ImGui::TextUnformatted(text.data(), text.data() + text.size());
		}

		ImGui::PopStyleColor();
	}

	ImGui::PopTextWrapPos();

	if (mAutoScroll)
	{
		const float max = ImGui::GetScrollMaxY();
		ImGui::SetScrollY(max);

		mFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	}
	else
	{
		mFlags &= ~(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	}
}

CE::MetaType CE::LogWindow::Reflect()
{
	MetaType type{MetaType::T<LogWindow>{}, "LogWindow", MetaType::Base<EditorSystem>{} };
	type.GetProperties().Add(Props::sEditorSystemDefaultOpenTag);
	return type;
}

