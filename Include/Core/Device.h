#pragma once

namespace CE
{
	class EngineConfig;

	class Device final :
        public EngineSubsystem<Device>
    {
        friend EngineSubsystem;

        Device(const EngineConfig& config);
        ~Device();

	public:
		void NewFrame();
		void EndFrame();

#ifdef EDITOR
		void CreateImguiContext();
#endif // EDITOR

		bool ShouldClose() const;

		glm::vec2 GetDisplaySize() const;
		glm::vec2 GetWindowPosition() const;

		void* GetWindow();

        /**
		 * \brief Some build/testing servers do not support graphics. This function can be used to check that.
		 *
		 * Device will not be initialized when the engine is in headless mode. Device::Get() will then throw an error.
		 * Only check for IsHeadless if you find that Device::Get() is being called from somewhere during unit tests.
		 */
		static bool IsHeadless();

	private:
		struct Impl;
		struct ImplDeleter
		{
			void operator()(Impl* impl) const;
		};
		std::unique_ptr<Impl, ImplDeleter> mImpl{};
	};
}
