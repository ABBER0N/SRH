#include "pch.h"

#include "Execution/Windows/Audio/AudioService.h"

#include <Windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>

#include <algorithm>

#pragma comment(lib, "Ole32.lib")

namespace
{
    class ComScope
    {
    public:
        ComScope()
        {
            const HRESULT result =
                CoInitializeEx(
                    nullptr,
                    COINIT_MULTITHREADED
                );

            if (
                result == S_OK ||
                result == S_FALSE
                )
            {
                m_ready =
                    true;

                m_shouldUninitialize =
                    true;

                return;
            }

            //
            // The current thread may already have
            // COM initialized using another apartment
            // model. COM is still available in that case.
            //

            if (
                result ==
                RPC_E_CHANGED_MODE
                )
            {
                m_ready =
                    true;

                return;
            }

            m_ready =
                false;
        }

        ~ComScope()
        {
            if (
                m_shouldUninitialize
                )
            {
                CoUninitialize();
            }
        }

        ComScope(
            const ComScope&
        ) = delete;

        ComScope& operator=(
            const ComScope&
            ) = delete;

        [[nodiscard]]
        bool IsReady() const
        {
            return m_ready;
        }

    private:
        bool m_ready{ false };

        bool m_shouldUninitialize{
            false
        };
    };

    bool GetDefaultRenderEndpointVolume(
        Microsoft::WRL::ComPtr<
        IAudioEndpointVolume
        >& endpointVolume
    )
    {
        Microsoft::WRL::ComPtr<
            IMMDeviceEnumerator
        > enumerator;

        HRESULT result =
            CoCreateInstance(
                __uuidof(
                    MMDeviceEnumerator
                    ),
                nullptr,
                CLSCTX_ALL,
                IID_PPV_ARGS(
                    enumerator.GetAddressOf()
                )
            );

        if (FAILED(result))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<
            IMMDevice
        > device;

        result =
            enumerator->GetDefaultAudioEndpoint(
                eRender,
                eMultimedia,
                device.GetAddressOf()
            );

        if (FAILED(result))
        {
            return false;
        }

        IAudioEndpointVolume*
            rawEndpointVolume =
            nullptr;

        result =
            device->Activate(
                __uuidof(
                    IAudioEndpointVolume
                    ),
                CLSCTX_ALL,
                nullptr,
                reinterpret_cast<void**>(
                    &rawEndpointVolume
                    )
            );

        if (
            FAILED(result) ||
            rawEndpointVolume == nullptr
            )
        {
            return false;
        }

        endpointVolume.Attach(
            rawEndpointVolume
        );

        return true;
    }
}

namespace srh::engine
{
    std::optional<MasterAudioState>
        AudioService::GetMasterState() const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return std::nullopt;
        }

        Microsoft::WRL::ComPtr<
            IAudioEndpointVolume
        > endpointVolume;

        if (
            !GetDefaultRenderEndpointVolume(
                endpointVolume
            )
            )
        {
            return std::nullopt;
        }

        float volume =
            0.0f;

        BOOL muted =
            FALSE;

        HRESULT result =
            endpointVolume->
            GetMasterVolumeLevelScalar(
                &volume
            );

        if (FAILED(result))
        {
            return std::nullopt;
        }

        result =
            endpointVolume->GetMute(
                &muted
            );

        if (FAILED(result))
        {
            return std::nullopt;
        }

        MasterAudioState state;

        state.volume =
            volume;

        state.muted =
            muted != FALSE;

        return state;
    }

    bool AudioService::SetMasterVolume(
        const float volume
    ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        Microsoft::WRL::ComPtr<
            IAudioEndpointVolume
        > endpointVolume;

        if (
            !GetDefaultRenderEndpointVolume(
                endpointVolume
            )
            )
        {
            return false;
        }

        const float clampedVolume =
            std::clamp(
                volume,
                0.0f,
                1.0f
            );

        const HRESULT result =
            endpointVolume->
            SetMasterVolumeLevelScalar(
                clampedVolume,
                nullptr
            );

        return SUCCEEDED(result);
    }

    bool AudioService::SetMasterMuted(
        const bool muted
    ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        Microsoft::WRL::ComPtr<
            IAudioEndpointVolume
        > endpointVolume;

        if (
            !GetDefaultRenderEndpointVolume(
                endpointVolume
            )
            )
        {
            return false;
        }

        const HRESULT result =
            endpointVolume->SetMute(
                muted
                ? TRUE
                : FALSE,
                nullptr
            );

        return SUCCEEDED(result);
    }
}