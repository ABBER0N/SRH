#pragma once

#include <cstdint>
#include <string>

namespace srh::engine
{
    enum class MediaPlaybackStatus :
        std::uint8_t
    {
        Unknown = 0,
        Closed,
        Opened,
        Changing,
        Stopped,
        Playing,
        Paused
    };

    struct MediaSessionInfo
    {
        std::string sourceAppId;

        std::string title;
        std::string artist;
        std::string albumTitle;

        MediaPlaybackStatus playbackStatus{
            MediaPlaybackStatus::Unknown
        };
    };
}