#pragma once
#include "Core/EngineSubsystem.h"

namespace FMOD
{
	class System;
	class Sound;
	class SoundGroup;

	namespace Studio
	{
		class System;
		class Bank;
		class EventInstance;
	}  // namespace Studio
}  // namespace FMOD

namespace CE
{
	/// <summary>
	/// A class with a number of functions for audio handling in our engine. 
	/// The implementation uses FMOD Studio. We recommend to handle your game's audio using FMOD Studio banks and events.
	/// For convenience, we have also added functions for simple sound playback using FMOD Core only.
	/// </summary>
	class Audio :
		public EngineSubsystem<Audio>
	{
		friend class EngineSubsystem<Audio>;
		Audio();
		~Audio();
	public:
		void Update();

		/// <summary>
		/// Loads an FMOD Studio bank with the given filename.
		/// </summary>
		/// <param name="filename">The name of the file that contains the bank.</param>
		void LoadBank(const std::string& filename);

		/// <summary>
		/// Unloads an FMOD Studio bank with the given filename.
		/// </summary>
		/// <param name="filename">The name of the file that contains the bank.</param>
		void UnloadBank(const std::string& filename);

		/// <summary>
		/// Starts a new instance of an FMOD Studio event with the given name.
		/// </summary>
		/// <param name="filename">The name of the event to start, without the prefix "event:/".</param>
		/// <returns>The ID of the newly created event instance.
		/// If the operation failed, this function returns -1 and prints information to the console.</returns>
		int StartEvent(const std::string& name);

		/// <summary>
		/// Sets an FMOD Studio floating-point parameter to the given value.
		/// </summary>
		/// <param name="filename">The name of the parameter to set.</param>
		/// <param name="value">The new desired value of the parameter.</param>
		/// <param name="eventID">(Optional) The ID of the event instance to which the parameter is associated.
		/// Use -1 to set a system-wide parameter that is not tied to a specific event.</param>
		void SetParameter(const std::string& name, float value, int eventInstanceID = -1);

		/// <summary>
		/// Sets an FMOD Studio string parameter to the given value (label).
		/// </summary>
		/// <param name="filename">The name of the parameter to set.</param>
		/// <param name="value">The new desired value of the parameter.</param>
		/// <param name="eventID">(Optional) The ID of the event instance to which the parameter is associated.
		/// Use -1 to set a system-wide parameter that is not tied to a specific event.</param>
		void SetParameter(const std::string& name, const std::string& value, int eventInstanceID = -1);

		/// <summary>
		/// Loads a sound file for playback.
		/// NOTE: This uses FMOD Core only and offers limited control.
		/// We recommend to use FMOD Studio banks and events for all audio handling.
		/// </summary>
		/// <param name="filename">The name of the sound file to load.</param>
		/// <param name="isMusic">Whether the file should be treated as a music track (instead of a sound effect). 
		/// Music tracks are loaded and buffered differently.</param>
		void LoadSFX(const std::string& filename, bool isMusic);

		/// <summary>
		/// Plays a previously loaded sound file, and returns the channel on which it will play.
		/// NOTE: This uses FMOD Core only and offers limited control.
		/// We recommend to use FMOD Studio banks and events for all audio handling.
		/// </summary>
		/// <param name="filename">The name of the sound file to play.</param>
		/// <returns>The ID of the channel on which the sound will play.
		/// If the operation failed, this function returns -1 and prints information to the console.</returns>
		int PlaySFX(const std::string& filename);

		/// <summary>
		/// Pauses or unpauses a sound channel.
		/// NOTE: This uses FMOD Core only and offers limited control.
		/// We recommend to use FMOD Studio banks and events for all audio handling.
		/// </summary>
		/// <param name="channelID">The ID of the channel to pause or unpause.</param>
		/// <param name="paused">true if the channel should be paused; false if it should resume playing.</param>
		void SetChannelPaused(int channelID, bool paused);

	private:
		FMOD::Studio::System* m_system = nullptr;
		FMOD::System* m_core_system = nullptr;

		std::unordered_map<int, FMOD::Sound*> m_sounds;
		FMOD::SoundGroup* m_soundGroupSFX = nullptr;
		FMOD::SoundGroup* m_soundGroupMusic = nullptr;

		std::unordered_map<int, FMOD::Studio::Bank*> m_banks;
		std::unordered_map<int, FMOD::Studio::EventInstance*> m_events;
		int m_nextEventID = 0;
	};
}