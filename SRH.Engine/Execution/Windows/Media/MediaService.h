#pragma once

#include "Domain/MediaState.h"

#include <optional>

namespace srh::engine
{
    class MediaService
    {
    public:
        MediaService() = default;

        MediaService(
            const MediaService&
        ) = delete;

        MediaService& operator=(
            const MediaService&
            ) = delete;

        [[nodiscard]]
        std::optional<MediaSessionInfo>
            GetCurrentSessionInfo() const;

        [[nodiscard]]
        bool TogglePlayPause() const;

        [[nodiscard]]
        bool PreviousTrack() const;

        [[nodiscard]]
        bool NextTrack() const;
    };
}