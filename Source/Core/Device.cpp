#include "Precomp.h"
#include "Core/Device.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#ifdef EDITOR
#include "imgui/ImGuizmo.h"
#include "imgui/imgui_impl_glfw.h"
#endif // EDITOR

#include "Core/Engine.h"
#include "Core/FileIO.h"

struct CE::Device::Impl
{
	GLFWwindow* mWindow{};
	glm::ivec2 mSize{};
	glm::ivec2 mPos{};
};

CE::Device::Device(const EngineConfig& config) :
	mImpl(new Impl)
{
	LOG(LogCore, Verbose, "Initializing GLFW");

	if (!glfwInit())
	{
		LOG(LogCore, Fatal, "GLFW could not be initialized");
	}

	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

	const std::string applicationName = !config.mWindowTitle.empty() ? config.mWindowTitle :
		[]()
		{
			std::string applicationName{};
#ifdef EDITOR
			applicationName = "Coral Engine - ";
#endif

			const std::filesystem::path thisExe = FileIO::Get().GetPath(FileIO::Directory::ThisExecutable, "");
			applicationName += thisExe.filename().replace_extension().string();
			if (applicationName.empty())
			{
				applicationName += "Unnamed application";
			}

			return applicationName;
		}();

	GLFWmonitor* monitor{};

	if (config.mIsFullScreen)
	{
		monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		mImpl->mSize = { mode->width, mode->height };
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	}
	else
	{
		mImpl->mSize = config.mWindowSizeHint;
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}

	mImpl->mWindow = glfwCreateWindow(mImpl->mSize.x, mImpl->mSize.y, applicationName.c_str(), monitor, nullptr);

	if (mImpl->mWindow == nullptr)
	{
		glfwTerminate();
		LOG(LogCore, Fatal, "GLFW window could not be created");
	}

	glfwMakeContextCurrent(mImpl->mWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		LOG(LogCore, Fatal, "Glad could not be loaded");
	}
}

CE::Device::~Device()
{
	glfwTerminate();
}

void CE::Device::NewFrame()
{
	glfwGetWindowSize(mImpl->mWindow, &mImpl->mSize.x, &mImpl->mSize.y);
	glfwGetWindowPos(mImpl->mWindow, &mImpl->mPos.x, &mImpl->mPos.y);

#ifdef EDITOR
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif // EDITOR
}

void CE::Device::EndFrame()
{
#ifdef EDITOR
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows 
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere. 
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly) 
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
#endif // EDITOR

	glfwSwapBuffers(mImpl->mWindow);

#ifdef EDITOR
	ImGui::EndFrame();
#endif // EDITOR
}

#ifdef EDITOR

void CE::Device::CreateImguiContext()
{
	LOG(LogCore, Verbose, "Creating imgui context");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |=
		ImGuiConfigFlags_DockingEnable
		| ImGuiConfigFlags_ViewportsEnable
		| ImGuiConfigFlags_NavEnableGamepad
		| ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigViewportsNoDecoration = false;
	io.DisplaySize = GetDisplaySize();

	ImGui_ImplOpenGL3_Init();
	ImGui_ImplGlfw_InitForOpenGL(mImpl->mWindow, true);
	ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);

	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	std::filesystem::create_directories("Intermediate/Layout");
	ImGui::GetIO().IniFilename = "Intermediate/Layout/imgui.ini";
}
#endif

bool CE::Device::ShouldClose() const
{
	return glfwWindowShouldClose(mImpl->mWindow);
}

glm::vec2 CE::Device::GetDisplaySize() const
{
	return mImpl->mSize;
}

glm::vec2 CE::Device::GetWindowPosition() const
{
	return mImpl->mPos;
}

void* CE::Device::GetWindow()
{
	return mImpl->mWindow;
}

bool CE::Device::IsHeadless()
{
	return sInstance == nullptr;
}

void CE::Device::ImplDeleter::operator()(Impl* impl) const
{
	delete impl;
}
