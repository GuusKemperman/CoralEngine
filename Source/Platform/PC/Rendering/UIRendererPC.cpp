#include "Precomp.h"
#include "Platform/PC/Rendering/UIRendererPC.h"

#include "Meta/MetaType.h"

Engine::UIRenderer::UIRenderer()
{}

Engine::UIRenderer::~UIRenderer()
{}

void Engine::UIRenderer::Render(const World&)
{}

Engine::MetaType Engine::UIRenderer::Reflect()
{
	return MetaType{ MetaType::T<UIRenderer>{}, "UIRendererSystem", MetaType::Base<System>{} };
}
