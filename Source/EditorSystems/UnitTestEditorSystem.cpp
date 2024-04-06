#include "Precomp.h"
#include "EditorSystems/UnitTestEditorSystem.h"

#include "Core/UnitTests.h"
#include "Meta/MetaType.h"
#include "Utilities/Imgui/ImguiHelpers.h"

CE::UnitTestEditorSystem::UnitTestEditorSystem() :
	EditorSystem("UnitTests")
{
}

void CE::UnitTestEditorSystem::Tick(const float)
{
	if (!Begin(ImGuiWindowFlags_MenuBar))
	{
		End();
		return;
	}

	Span<UnitTest> allTests = UnitTestManager::Get().GetAllTests();

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::Button("Run failed"))
		{
			UnitTestManager::Get().RunTests(static_cast<UnitTest::Result>(UnitTest::NotRan | UnitTest::Failure));
		}
		ImGui::SetItemTooltip("Runs all failed tests, and those that have never been run.");

		if (ImGui::Button("Run outdated"))
		{
			UnitTestManager::Get().RunTests(static_cast<UnitTest::Result>(UnitTest::NotRan | UnitTest::OutDated));
		}
		ImGui::SetItemTooltip("Runs all tests that were run before the last compilation, and those that have never been run.");

		if (ImGui::Button("Run all"))
		{
			UnitTestManager::Get().RunTests(UnitTest::All);
		}
		ImGui::SetItemTooltip("Runs all tests, even those that succeeded.");

		if (ImGui::Button("Clear results"))
		{
			for (UnitTest& test : allTests)
			{
				test.Clear();
			}
		}

		ImGui::EndMenuBar();
	}

	auto getTextColor = [](std::string& text, int result)
		{
			glm::vec4 color{};

			if (result & UnitTest::Failure)
			{
				color = { 1.0f, 0.0f, 0.0f, 1.0f };
			}
			else if (result & UnitTest::NotRan)
			{
				color = { 1.0f, 0.5f, 0.0f, 1.0f };
			}
			else
			{
				color = { 0.0f, 1.0f, 0.0f, 1.0f };
			}

			if (result & UnitTest::OutDated)
			{
				text.insert(0, "* ");
				color *= .8f;
			}

			return color;
		};

	for (auto first = allTests.begin(), last = first; first != allTests.end(); first = last)
	{
		last = std::find_if(first, allTests.end(),
			[&first](const UnitTest& unitTest)
			{
				return unitTest.mCategory != first->mCategory;
			});

		int combinedResult{};

		for (auto it = first; it != last; ++it)
		{
			combinedResult |= it->mResult;
		}

		bool runAllInCategory{};
		std::string categoryText{ first->mCategory };
		ImVec4 categoryColor = getTextColor(categoryText, combinedResult);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.51f, 0.51f, 0.51f, 0.52f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.77f, 0.77f, 0.77f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_Text, categoryColor);
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.51f, 0.51f, 0.51f, 0.52f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.77f, 0.77f, 0.77f, 1.00f));
		const bool isOpen = ImGui::CollapsingHeaderWithButton(categoryText.c_str(), ICON_FA_PLAY_CIRCLE, &runAllInCategory);
		ImGui::PopStyleColor(4);

		if (runAllInCategory)
		{
			for (auto it = first; it != last; ++it)
			{
				(*it)();
			}
		}

		if (!isOpen)
		{
			ImGui::PopStyleColor(3);
			continue;
		}

		ImGui::Indent();

		for (auto it = first; it != last; ++it)
		{
			UnitTest& test = *it;

			if (runAllInCategory)
			{
				test();
			}

			auto displayToolTip = [&test]
				{
					if (test.mResult & UnitTest::NotRan)
					{
						ImGui::TextUnformatted("Test has not been run yet.");
					}
					else
					{
						const auto now = std::chrono::system_clock::now();

						if (test.mResult & UnitTest::OutDated)
						{
							ImGui::TextUnformatted(Format("Results are out of date, ran {:2}:{:2}:{:2} ago",
								std::chrono::duration_cast<std::chrono::hours>(now - test.mTimeLastRan).count(),
								std::chrono::duration_cast<std::chrono::minutes>(now - test.mTimeLastRan).count() % 60,
								std::chrono::duration_cast<std::chrono::seconds>(now - test.mTimeLastRan).count() % 60).c_str());
						}

						ImGui::TextUnformatted(Format("Test took {} ms", test.mLastTestDuration.count()).c_str());
					}

					ImGui::EndTooltip();
				};

			std::string testText{ test.mName };
			ImVec4 color = getTextColor(testText, test.mResult);

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(testText.c_str());
			ImGui::PopStyleColor();

			if (ImGui::BeginItemTooltip())
			{
				displayToolTip();
			}

			ImGui::PushID(test.mName.c_str());
			const float buttonWidth = ImGui::CalcTextSize("|>", NULL, true).x;
			ImGui::SameLine(ImGui::GetWindowWidth() - buttonWidth - 16.0f);

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			if (ImGui::SmallButton("|>"))
			{
				test();
			}
			ImGui::PopStyleColor();

			if (ImGui::BeginItemTooltip())
			{
				displayToolTip();
			}

			ImGui::PopID();
		}

		ImGui::PopStyleColor(3);
		ImGui::Unindent();
	}

	End();
}

CE::MetaType CE::UnitTestEditorSystem::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UnitTestEditorSystem>{}, "UnitTestEditorSystem", MetaType::Base<EditorSystem>{} };
	return type;
}
