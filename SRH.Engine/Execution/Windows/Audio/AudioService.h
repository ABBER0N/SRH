#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace srh::engine
{
    struct MasterAudioState
    {
        float volume{ 0.0f };
        bool muted{ false };
    };

    struct AudioSessionInfo
    {
        std::string sessionInstanceId;

        std::string displayName;

        std::string executableName;
        std::string executablePath;

        std::uint32_t processId{ 0 };

        float volume{ 0.0f };

        bool muted{ false };
        bool active{ false };
        bool systemSounds{ false };
    };

    class AudioService
    {
    public:
        AudioService() = default;

        AudioService(
            const AudioService&
        ) = delete;

        AudioService& operator=(
            const AudioService&
            ) = delete;

        //
        // Master audio
        //

        [[nodiscard]]
        std::optional<MasterAudioState>
            GetMasterState() const;

        [[nodiscard]]
        bool SetMasterVolume(
            float volume
        ) const;

        [[nodiscard]]
        bool SetMasterMuted(
            bool muted
        ) const;

        //
        // Application audio sessions
        //

        [[nodiscard]]
        std::vector<AudioSessionInfo>
            EnumerateSessions() const;

        [[nodiscard]]
        bool SetSessionVolume(
            const std::string& sessionInstanceId,
            float volume
        ) const;

        [[nodiscard]]
        bool SetSessionMuted(
            const std::string& sessionInstanceId,
            bool muted
        ) const;
    };
}