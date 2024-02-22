#include "Precomp.h"
#include "Core/Renderer.h"

#include "xsr.hpp"

#ifdef PLATFORM_WINDOWS

// Disable warnings from external code
#pragma warning(push, 0)

#ifdef APIENTRY
#define TMP_APIENTRY APIENTRY
#undef APIENTRY
#endif // APIENTRY

#define GLFW_USE_CHDIR 0
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/ImGuizmo.h"

#ifndef APIENTRY
#define APIENTRY TMP_APIENTRY
#endif

#ifdef TMP_APIENTRY
#undef TMP_APIENTRY
#endif

#pragma warning(pop)

void ErrorCallback([[maybe_unused]] int error, [[maybe_unused]] const char* description)
{
	if (error != 0)
	{
		LOG(LogCore, Error, "GLFW Error: {}, {}", error, description);
	}
}

Engine::Renderer::Renderer()
{
	constexpr int width = 1600;
	constexpr int height = 900;

	LOG(LogCore, Message, "Initializing GLFW");
	[[maybe_unused]] bool succes = glfwInit();
	ASSERT(succes);

	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // 3.3 is enough for our needs
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_STENCIL_BITS, GL_FALSE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE /* easier :) */);

	glfwWindowHint(GLFW_SAMPLES, 4);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	std::string windowTitle = "Engine";

#ifdef _DEBUG
	windowTitle += "_DEBUG";
#else
	windowTitle += "_RELEASE";
#endif

	LOG(LogCore, Message, "Creating window");
	mWindow = glfwCreateWindow(width, height, windowTitle.c_str(), 0, 0);
	ASSERT(mWindow != nullptr);

	glfwMakeContextCurrent(mWindow);
	succes = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	ASSERT(succes);

	int dummy = glGetError();

	if (dummy != GL_NO_ERROR)
	{
		LOG(LogCore, Verbose, "I still get this error: {}", dummy);
	}

	glfwSwapInterval(0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);

	CheckGL();

	// create xsr render config
	xsr::render_configuration config;
	config.enable_shadows = true;
	config.shadow_resolution = 1024;
	config.texture_filter = xsr::render_configuration::texture_filtering::linear;
	config.shader_path = "external/xsr/";

	LOG(LogCore, Message, "Initializing xsr");
	[[maybe_unused]] const bool success = initialize(config);
	ASSERT_LOG(success, "Failed to initialize xsr");
}

Engine::Renderer::~Renderer()
{
	xsr::shutdown();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void Engine::Renderer::CreateImguiContext()
{
	LOG(LogCore, Message, "Creating imgui context");

	glfwShowWindow(mWindow);
	mIsWindowOpen = true;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigViewportsNoDecoration = false;

	ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	std::filesystem::create_directories("Intermediate/Layout");
	ImGui::GetIO().IniFilename = "Intermediate/Layout/imgui.ini";
}

void Engine::Renderer::NewFrame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	// Reset the viewport
	const glm::vec2 displaySize = ImGui::GetIO().DisplaySize;

	if (displaySize.x > 1.0f
		&& displaySize.y > 1.0f)
	{
		glViewport(0, 0, static_cast<GLsizei>(displaySize.x), static_cast<GLsizei>(displaySize.y));
	}
}

void Engine::Renderer::Render()
{
	// Rendering 
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

	glfwSwapBuffers(mWindow);
	mIsWindowOpen = !glfwWindowShouldClose(mWindow);
	CheckGL();
}

void _CheckGL([[maybe_unused]] const char* f, [[maybe_unused]] int l)
{
	const GLenum error = glGetError();
	std::string errorStr = "UNKNOWN ERROR";

	switch (error)
	{
	case GL_NO_ERROR: return;
	case GL_INVALID_ENUM: errorStr = "INVALID ENUM"; break;
	case GL_INVALID_OPERATION: errorStr = "INVALID OPERATION"; break;
	case GL_INVALID_VALUE: errorStr = "INVALID VALUE"; break;
	case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "INVALID FRAMEBUFFER OPERATION"; break;
	case GL_INVALID_INDEX: errorStr = "INVALID INDEX"; break;
	}

	LOG(LogCore, Error, "GLerror: File: {}, Line {}, GLError {}, GLErrorCode {}", f, l, errorStr, error);
}

void _CheckFrameBuffer([[maybe_unused]] const char* f, [[maybe_unused]] int l)
{
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	std::string error{};

	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE: return;
	case GL_FRAMEBUFFER_UNDEFINED: error = ("GL_FRAMEBUFFER_UNDEFINED");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: error = ("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: error = ("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: error = ("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: error = ("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");	break;
	case GL_FRAMEBUFFER_UNSUPPORTED: error = ("GL_FRAMEBUFFER_UNSUPPORTED");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: error = ("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");	break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: error = ("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");	break;
	}

	LOG(LogCore, Error, "Invalid frame buffer: File: {}, Line: {}, GLError: {}, GLErrorCode {}", f, l, error, status);
}

#elif PLATFORM_***REMOVED***

Engine::Renderer::Renderer()
{
	xsr::device_configuration deviceConfig;
	deviceConfig.title = "***REMOVED***";
	deviceConfig.width = 3840;
	deviceConfig.height = 2160;

	LOG(LogCore, Message, "Initializing xsr");
	[[maybe_unused]] bool success = xsr::device::initialize(deviceConfig);
	ASSERT_LOG(success, "Failed to initialize xsr device");

	// create xsr render config
	xsr::render_configuration config;
	config.enable_shadows = true;
	config.shadow_resolution = 1024;
	config.texture_filter = xsr::render_configuration::texture_filtering::linear;
	config.shader_path = "external/xsr/";

	LOG(LogCore, Message, "Initializing xsr");
	success = initialize(config);
	ASSERT_LOG(success, "Failed to initialize xsr");
}

Engine::Renderer::~Renderer()
{
	xsr::shutdown();
}

void Engine::Renderer::CreateImguiContext()
{}

void Engine::Renderer::NewFrame()
{
	xsr::device::update();
}

void Engine::Renderer::Render()
{}

#endif