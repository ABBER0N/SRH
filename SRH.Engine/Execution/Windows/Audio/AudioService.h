#pragma once

#include <optional>

namespace srh::engine
{
    struct MasterAudioState
    {
        float volume{ 0.0f };
        bool muted{ false };
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
    };
}