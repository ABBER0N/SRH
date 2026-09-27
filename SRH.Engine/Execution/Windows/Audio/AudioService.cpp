#include "pch.h"

#include "Execution/Windows/Audio/AudioService.h"
#include "Execution/Windows/Text/Utf8.h"

#include <Windows.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <string>
#include <vector>

#pragma comment(lib, "Ole32.lib")

namespace
{
    using Microsoft::WRL::ComPtr;

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
        bool m_ready{
            false
        };

        bool m_shouldUninitialize{
            false
        };
    };

    std::wstring GetProcessExecutablePath(
        const DWORD processId
    )
    {
        if (processId == 0)
        {
            return {};
        }

        const HANDLE processHandle =
            OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION,
                FALSE,
                processId
            );

        if (
            processHandle ==
            nullptr
            )
        {
            return {};
        }

        std::vector<wchar_t>
            buffer(32768);

        DWORD bufferLength =
            static_cast<DWORD>(
                buffer.size()
                );

        const BOOL success =
            QueryFullProcessImageNameW(
                processHandle,
                0,
                buffer.data(),
                &bufferLength
            );

        CloseHandle(
            processHandle
        );

        if (!success)
        {
            return {};
        }

        return std::wstring(
            buffer.data(),
            bufferLength
        );
    }

    std::wstring GetFileName(
        const std::wstring& path
    )
    {
        if (path.empty())
        {
            return {};
        }

        const auto separator =
            path.find_last_of(
                L"\\/"
            );

        if (
            separator ==
            std::wstring::npos
            )
        {
            return path;
        }

        return path.substr(
            separator + 1
        );
    }

    bool GetDefaultRenderDevice(
        ComPtr<IMMDevice>& device
    )
    {
        ComPtr<IMMDeviceEnumerator>
            enumerator;

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

        result =
            enumerator->
            GetDefaultAudioEndpoint(
                eRender,
                eMultimedia,
                device.GetAddressOf()
            );

        return SUCCEEDED(result);
    }

    bool GetDefaultRenderEndpointVolume(
        ComPtr<IAudioEndpointVolume>&
        endpointVolume
    )
    {
        ComPtr<IMMDevice>
            device;

        if (
            !GetDefaultRenderDevice(
                device
            )
            )
        {
            return false;
        }

        IAudioEndpointVolume*
            rawEndpointVolume =
            nullptr;

        const HRESULT result =
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
            rawEndpointVolume ==
            nullptr
            )
        {
            return false;
        }

        endpointVolume.Attach(
            rawEndpointVolume
        );

        return true;
    }

    bool GetDefaultRenderSessionManager(
        ComPtr<IAudioSessionManager2>&
        sessionManager
    )
    {
        ComPtr<IMMDevice>
            device;

        if (
            !GetDefaultRenderDevice(
                device
            )
            )
        {
            return false;
        }

        IAudioSessionManager2*
            rawSessionManager =
            nullptr;

        const HRESULT result =
            device->Activate(
                __uuidof(
                    IAudioSessionManager2
                    ),
                CLSCTX_ALL,
                nullptr,
                reinterpret_cast<void**>(
                    &rawSessionManager
                    )
            );

        if (
            FAILED(result) ||
            rawSessionManager ==
            nullptr
            )
        {
            return false;
        }

        sessionManager.Attach(
            rawSessionManager
        );

        return true;
    }

    bool GetSessionInstanceId(
        IAudioSessionControl2* control,
        std::wstring& sessionInstanceId
    )
    {
        if (control == nullptr)
        {
            return false;
        }

        LPWSTR rawId =
            nullptr;

        const HRESULT result =
            control->
            GetSessionInstanceIdentifier(
                &rawId
            );

        if (
            FAILED(result) ||
            rawId == nullptr
            )
        {
            return false;
        }

        sessionInstanceId =
            rawId;

        CoTaskMemFree(
            rawId
        );

        return
            !sessionInstanceId.empty();
    }

    bool FindSessionVolume(
        const std::wstring&
        targetSessionInstanceId,
        ComPtr<ISimpleAudioVolume>&
        sessionVolume
    )
    {
        if (
            targetSessionInstanceId.empty()
            )
        {
            return false;
        }

        ComPtr<IAudioSessionManager2>
            sessionManager;

        if (
            !GetDefaultRenderSessionManager(
                sessionManager
            )
            )
        {
            return false;
        }

        ComPtr<IAudioSessionEnumerator>
            sessionEnumerator;

        HRESULT result =
            sessionManager->
            GetSessionEnumerator(
                sessionEnumerator
                .GetAddressOf()
            );

        if (FAILED(result))
        {
            return false;
        }

        int sessionCount =
            0;

        result =
            sessionEnumerator->GetCount(
                &sessionCount
            );

        if (FAILED(result))
        {
            return false;
        }

        for (
            int index = 0;
            index < sessionCount;
            ++index
            )
        {
            ComPtr<IAudioSessionControl>
                control;

            result =
                sessionEnumerator->GetSession(
                    index,
                    control.GetAddressOf()
                );

            if (FAILED(result))
            {
                continue;
            }

            ComPtr<IAudioSessionControl2>
                control2;

            result =
                control.As(
                    &control2
                );

            if (FAILED(result))
            {
                continue;
            }

            std::wstring
                sessionInstanceId;

            if (
                !GetSessionInstanceId(
                    control2.Get(),
                    sessionInstanceId
                )
                )
            {
                continue;
            }

            if (
                sessionInstanceId !=
                targetSessionInstanceId
                )
            {
                continue;
            }

            result =
                control.As(
                    &sessionVolume
                );

            return SUCCEEDED(result);
        }

        return false;
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

        ComPtr<IAudioEndpointVolume>
            endpointVolume;

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

        ComPtr<IAudioEndpointVolume>
            endpointVolume;

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

        ComPtr<IAudioEndpointVolume>
            endpointVolume;

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

    std::vector<AudioSessionInfo>
        AudioService::EnumerateSessions() const
    {
        std::vector<AudioSessionInfo>
            sessions;

        ComScope comScope;

        if (!comScope.IsReady())
        {
            return sessions;
        }

        ComPtr<IAudioSessionManager2>
            sessionManager;

        if (
            !GetDefaultRenderSessionManager(
                sessionManager
            )
            )
        {
            return sessions;
        }

        ComPtr<IAudioSessionEnumerator>
            sessionEnumerator;

        HRESULT result =
            sessionManager->
            GetSessionEnumerator(
                sessionEnumerator
                .GetAddressOf()
            );

        if (FAILED(result))
        {
            return sessions;
        }

        int sessionCount =
            0;

        result =
            sessionEnumerator->GetCount(
                &sessionCount
            );

        if (FAILED(result))
        {
            return sessions;
        }

        sessions.reserve(
            static_cast<std::size_t>(
                sessionCount
                )
        );

        for (
            int index = 0;
            index < sessionCount;
            ++index
            )
        {
            ComPtr<IAudioSessionControl>
                control;

            result =
                sessionEnumerator->GetSession(
                    index,
                    control.GetAddressOf()
                );

            if (FAILED(result))
            {
                continue;
            }

            ComPtr<IAudioSessionControl2>
                control2;

            result =
                control.As(
                    &control2
                );

            if (FAILED(result))
            {
                continue;
            }

            ComPtr<ISimpleAudioVolume>
                simpleVolume;

            result =
                control.As(
                    &simpleVolume
                );

            if (FAILED(result))
            {
                continue;
            }

            std::wstring
                sessionInstanceId;

            if (
                !GetSessionInstanceId(
                    control2.Get(),
                    sessionInstanceId
                )
                )
            {
                continue;
            }

            DWORD processId =
                0;

            (void)control2->
                GetProcessId(
                    &processId
                );

            AudioSessionState
                sessionState =
                AudioSessionStateInactive;

            (void)control->GetState(
                &sessionState
            );

            LPWSTR rawDisplayName =
                nullptr;

            std::wstring
                displayName;

            if (
                SUCCEEDED(
                    control->GetDisplayName(
                        &rawDisplayName
                    )
                ) &&
                rawDisplayName != nullptr
                )
            {
                displayName =
                    rawDisplayName;

                CoTaskMemFree(
                    rawDisplayName
                );
            }

            float volume =
                0.0f;

            BOOL muted =
                FALSE;

            if (
                FAILED(
                    simpleVolume->
                    GetMasterVolume(
                        &volume
                    )
                )
                )
            {
                continue;
            }

            if (
                FAILED(
                    simpleVolume->GetMute(
                        &muted
                    )
                )
                )
            {
                continue;
            }

            const std::wstring
                executablePath =
                GetProcessExecutablePath(
                    processId
                );

            const std::wstring
                executableName =
                GetFileName(
                    executablePath
                );

            AudioSessionInfo info;

            info.sessionInstanceId =
                windows::WideToUtf8(
                    sessionInstanceId
                );

            info.displayName =
                windows::WideToUtf8(
                    displayName
                );

            info.executablePath =
                windows::WideToUtf8(
                    executablePath
                );

            info.executableName =
                windows::WideToUtf8(
                    executableName
                );

            info.processId =
                processId;

            info.volume =
                volume;

            info.muted =
                muted != FALSE;

            info.active =
                sessionState ==
                AudioSessionStateActive;

            info.systemSounds =
                control2->
                IsSystemSoundsSession() ==
                S_OK;

            sessions.push_back(
                std::move(info)
            );
        }

        std::sort(
            sessions.begin(),
            sessions.end(),
            [](
                const AudioSessionInfo& left,
                const AudioSessionInfo& right
                )
            {
                if (
                    left.executableName !=
                    right.executableName
                    )
                {
                    return
                        left.executableName <
                        right.executableName;
                }

                return
                    left.processId <
                    right.processId;
            }
        );

        return sessions;
    }

    bool AudioService::SetSessionVolume(
        const std::string&
        sessionInstanceId,
        const float volume
    ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        const std::wstring
            wideSessionInstanceId =
            windows::Utf8ToWide(
                sessionInstanceId
            );

        if (
            wideSessionInstanceId.empty()
            )
        {
            return false;
        }

        ComPtr<ISimpleAudioVolume>
            sessionVolume;

        if (
            !FindSessionVolume(
                wideSessionInstanceId,
                sessionVolume
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
            sessionVolume->
            SetMasterVolume(
                clampedVolume,
                nullptr
            );

        return SUCCEEDED(result);
    }

    bool AudioService::SetSessionMuted(
        const std::string&
        sessionInstanceId,
        const bool muted
    ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        const std::wstring
            wideSessionInstanceId =
            windows::Utf8ToWide(
                sessionInstanceId
            );

        if (
            wideSessionInstanceId.empty()
            )
        {
            return false;
        }

        ComPtr<ISimpleAudioVolume>
            sessionVolume;

        if (
            !FindSessionVolume(
                wideSessionInstanceId,
                sessionVolume
            )
            )
        {
            return false;
        }

        const HRESULT result =
            sessionVolume->SetMute(
                muted
                ? TRUE
                : FALSE,
                nullptr
            );

        return SUCCEEDED(result);
    }
}