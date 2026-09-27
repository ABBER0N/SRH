#include "pch.h"

#include "Execution/Windows/Media/MediaService.h"

#include <Windows.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

#pragma comment(lib, "windowsapp.lib")

namespace
{
    namespace media =
        winrt::Windows::Media::Control;

    class WinRtScope
    {
    public:
        WinRtScope()
        {
            try
            {
                winrt::init_apartment(
                    winrt::apartment_type::
                    multi_threaded
                );

                m_ready =
                    true;

                m_shouldUninitialize =
                    true;
            }
            catch (
                const winrt::hresult_error&
                error
                )
            {
                //
                // The current thread may already have
                // an apartment initialized by its owner,
                // for example a UI thread.
                //

                if (
                    error.code() ==
                    RPC_E_CHANGED_MODE
                    )
                {
                    m_ready =
                        true;

                    m_shouldUninitialize =
                        false;
                }
            }
        }

        ~WinRtScope()
        {
            if (
                m_shouldUninitialize
                )
            {
                winrt::uninit_apartment();
            }
        }

        WinRtScope(
            const WinRtScope&
        ) = delete;

        WinRtScope& operator=(
            const WinRtScope&
            ) = delete;

        [[nodiscard]]
        bool IsReady() const
        {
            return m_ready;
        }

    private:
        bool m_ready{
            false
        };

        bool m_shouldUninitialize{
            false
        };
    };

    srh::engine::MediaPlaybackStatus
        ConvertPlaybackStatus(
            const media::
            GlobalSystemMediaTransportControlsSessionPlaybackStatus
            status
        )
    {
        using WindowsStatus =
            media::
            GlobalSystemMediaTransportControlsSessionPlaybackStatus;

        using EngineStatus =
            srh::engine::
            MediaPlaybackStatus;

        switch (status)
        {
        case WindowsStatus::Closed:
            return
                EngineStatus::Closed;

        case WindowsStatus::Opened:
            return
                EngineStatus::Opened;

        case WindowsStatus::Changing:
            return
                EngineStatus::Changing;

        case WindowsStatus::Stopped:
            return
                EngineStatus::Stopped;

        case WindowsStatus::Playing:
            return
                EngineStatus::Playing;

        case WindowsStatus::Paused:
            return
                EngineStatus::Paused;

        default:
            return
                EngineStatus::Unknown;
        }
    }

    std::optional<
        media::
        GlobalSystemMediaTransportControlsSession
    >
        GetCurrentSession()
    {
        try
        {
            const auto manager =
                media::
                GlobalSystemMediaTransportControlsSessionManager::
                RequestAsync()
                .get();

            const auto session =
                manager.GetCurrentSession();

            if (!session)
            {
                return std::nullopt;
            }

            return session;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}

namespace srh::engine
{
    std::optional<MediaSessionInfo>
        MediaService::
        GetCurrentSessionInfo() const
    {
        WinRtScope winRtScope;

        if (!winRtScope.IsReady())
        {
            return std::nullopt;
        }

        const auto session =
            GetCurrentSession();

        if (!session.has_value())
        {
            return std::nullopt;
        }

        try
        {
            const auto properties =
                session->
                TryGetMediaPropertiesAsync()
                .get();

            const auto playbackInfo =
                session->
                GetPlaybackInfo();

            MediaSessionInfo info;

            info.sourceAppId =
                winrt::to_string(
                    session->
                    SourceAppUserModelId()
                );

            info.title =
                winrt::to_string(
                    properties.Title()
                );

            info.artist =
                winrt::to_string(
                    properties.Artist()
                );

            info.albumTitle =
                winrt::to_string(
                    properties.AlbumTitle()
                );

            info.playbackStatus =
                ConvertPlaybackStatus(
                    playbackInfo.PlaybackStatus()
                );

            return info;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    bool MediaService::
        TogglePlayPause() const
    {
        WinRtScope winRtScope;

        if (!winRtScope.IsReady())
        {
            return false;
        }

        const auto session =
            GetCurrentSession();

        if (!session.has_value())
        {
            return false;
        }

        try
        {
            return
                session->
                TryTogglePlayPauseAsync()
                .get();
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaService::
        PreviousTrack() const
    {
        WinRtScope winRtScope;

        if (!winRtScope.IsReady())
        {
            return false;
        }

        const auto session =
            GetCurrentSession();

        if (!session.has_value())
        {
            return false;
        }

        try
        {
            return
                session->
                TrySkipPreviousAsync()
                .get();
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaService::
        NextTrack() const
    {
        WinRtScope winRtScope;

        if (!winRtScope.IsReady())
        {
            return false;
        }

        const auto session =
            GetCurrentSession();

        if (!session.has_value())
        {
            return false;
        }

        try
        {
            return
                session->
                TrySkipNextAsync()
                .get();
        }
        catch (...)
        {
            return false;
        }
    }
}