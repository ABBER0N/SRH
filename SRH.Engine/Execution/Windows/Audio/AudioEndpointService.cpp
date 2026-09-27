#include "pch.h"

#include "Execution/Windows/Audio/AudioEndpointService.h"
#include "Execution/Windows/Text/Utf8.h"

#include <Windows.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>

#include <algorithm>
#include <string>
#include <utility>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Uuid.lib")

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

    std::wstring GetDeviceId(
        IMMDevice* device
    )
    {
        if (device == nullptr)
        {
            return {};
        }

        LPWSTR rawId =
            nullptr;

        const HRESULT result =
            device->GetId(
                &rawId
            );

        if (
            FAILED(result) ||
            rawId == nullptr
            )
        {
            return {};
        }

        std::wstring id =
            rawId;

        CoTaskMemFree(
            rawId
        );

        return id;
    }

    std::wstring GetDeviceName(
        IMMDevice* device
    )
    {
        if (device == nullptr)
        {
            return {};
        }

        ComPtr<IPropertyStore>
            propertyStore;

        HRESULT result =
            device->OpenPropertyStore(
                STGM_READ,
                propertyStore.GetAddressOf()
            );

        if (FAILED(result))
        {
            return {};
        }

        PROPVARIANT value{};

        result =
            propertyStore->GetValue(
                PKEY_Device_FriendlyName,
                &value
            );

        if (FAILED(result))
        {
            return {};
        }

        std::wstring name;

        if (
            value.vt == VT_LPWSTR &&
            value.pwszVal != nullptr
            )
        {
            name =
                value.pwszVal;
        }

        PropVariantClear(
            &value
        );

        return name;
    }

    bool GetEndpointVolumeInterface(
        IMMDevice* device,
        ComPtr<IAudioEndpointVolume>&
        endpointVolume
    )
    {
        if (device == nullptr)
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

    std::wstring GetDefaultEndpointId(
        IMMDeviceEnumerator* enumerator,
        const EDataFlow dataFlow
    )
    {
        if (enumerator == nullptr)
        {
            return {};
        }

        ComPtr<IMMDevice>
            device;

        const HRESULT result =
            enumerator->
            GetDefaultAudioEndpoint(
                dataFlow,
                eMultimedia,
                device.GetAddressOf()
            );

        if (FAILED(result))
        {
            return {};
        }

        return
            GetDeviceId(
                device.Get()
            );
    }

    bool OpenEndpointById(
        const std::string& endpointId,
        ComPtr<IMMDevice>& device
    )
    {
        const std::wstring wideId =
            srh::engine::windows::
            Utf8ToWide(
                endpointId
            );

        if (wideId.empty())
        {
            return false;
        }

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
            enumerator->GetDevice(
                wideId.c_str(),
                device.GetAddressOf()
            );

        return SUCCEEDED(result);
    }
}

namespace srh::engine
{
    std::vector<AudioEndpointInfo>
        AudioEndpointService::
        EnumerateOutputDevices() const
    {
        return
            EnumerateDevices(
                AudioEndpointType::Render
            );
    }

    std::vector<AudioEndpointInfo>
        AudioEndpointService::
        EnumerateInputDevices() const
    {
        return
            EnumerateDevices(
                AudioEndpointType::Capture
            );
    }

    std::vector<AudioEndpointInfo>
        AudioEndpointService::
        EnumerateDevices(
            const AudioEndpointType type
        ) const
    {
        std::vector<AudioEndpointInfo>
            endpoints;

        ComScope comScope;

        if (!comScope.IsReady())
        {
            return endpoints;
        }

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
            return endpoints;
        }

        const EDataFlow dataFlow =
            type ==
            AudioEndpointType::Render
            ? eRender
            : eCapture;

        const std::wstring defaultId =
            GetDefaultEndpointId(
                enumerator.Get(),
                dataFlow
            );

        ComPtr<IMMDeviceCollection>
            collection;

        result =
            enumerator->EnumAudioEndpoints(
                dataFlow,
                DEVICE_STATE_ACTIVE,
                collection.GetAddressOf()
            );

        if (FAILED(result))
        {
            return endpoints;
        }

        UINT count =
            0;

        result =
            collection->GetCount(
                &count
            );

        if (FAILED(result))
        {
            return endpoints;
        }

        endpoints.reserve(
            count
        );

        for (
            UINT index = 0;
            index < count;
            ++index
            )
        {
            ComPtr<IMMDevice>
                device;

            result =
                collection->Item(
                    index,
                    device.GetAddressOf()
                );

            if (FAILED(result))
            {
                continue;
            }

            const std::wstring id =
                GetDeviceId(
                    device.Get()
                );

            if (id.empty())
            {
                continue;
            }

            ComPtr<IAudioEndpointVolume>
                endpointVolume;

            if (
                !GetEndpointVolumeInterface(
                    device.Get(),
                    endpointVolume
                )
                )
            {
                continue;
            }

            float volume =
                0.0f;

            BOOL muted =
                FALSE;

            if (
                FAILED(
                    endpointVolume->
                    GetMasterVolumeLevelScalar(
                        &volume
                    )
                )
                )
            {
                continue;
            }

            if (
                FAILED(
                    endpointVolume->
                    GetMute(
                        &muted
                    )
                )
                )
            {
                continue;
            }

            AudioEndpointInfo info;

            info.id =
                windows::WideToUtf8(
                    id
                );

            info.name =
                windows::WideToUtf8(
                    GetDeviceName(
                        device.Get()
                    )
                );

            info.type =
                type;

            info.isDefault =
                !defaultId.empty() &&
                id == defaultId;

            info.volume =
                volume;

            info.muted =
                muted != FALSE;

            endpoints.push_back(
                std::move(
                    info
                )
            );
        }

        std::sort(
            endpoints.begin(),
            endpoints.end(),
            [](
                const AudioEndpointInfo& left,
                const AudioEndpointInfo& right
                )
            {
                if (
                    left.isDefault !=
                    right.isDefault
                    )
                {
                    return
                        left.isDefault >
                        right.isDefault;
                }

                return
                    left.name <
                    right.name;
            }
        );

        return endpoints;
    }

    std::optional<AudioEndpointInfo>
        AudioEndpointService::FindEndpoint(
            const std::string& endpointId
        ) const
    {
        if (endpointId.empty())
        {
            return std::nullopt;
        }

        const auto outputs =
            EnumerateOutputDevices();

        auto iterator =
            std::find_if(
                outputs.begin(),
                outputs.end(),
                [&endpointId](
                    const AudioEndpointInfo& endpoint
                    )
                {
                    return
                        endpoint.id ==
                        endpointId;
                }
            );

        if (
            iterator !=
            outputs.end()
            )
        {
            return *iterator;
        }

        const auto inputs =
            EnumerateInputDevices();

        iterator =
            std::find_if(
                inputs.begin(),
                inputs.end(),
                [&endpointId](
                    const AudioEndpointInfo& endpoint
                    )
                {
                    return
                        endpoint.id ==
                        endpointId;
                }
            );

        if (
            iterator !=
            inputs.end()
            )
        {
            return *iterator;
        }

        return std::nullopt;
    }

    bool AudioEndpointService::
        SetEndpointVolume(
            const std::string& endpointId,
            const float volume
        ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        ComPtr<IMMDevice>
            device;

        if (
            !OpenEndpointById(
                endpointId,
                device
            )
            )
        {
            return false;
        }

        ComPtr<IAudioEndpointVolume>
            endpointVolume;

        if (
            !GetEndpointVolumeInterface(
                device.Get(),
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

    bool AudioEndpointService::
        SetEndpointMuted(
            const std::string& endpointId,
            const bool muted
        ) const
    {
        ComScope comScope;

        if (!comScope.IsReady())
        {
            return false;
        }

        ComPtr<IMMDevice>
            device;

        if (
            !OpenEndpointById(
                endpointId,
                device
            )
            )
        {
            return false;
        }

        ComPtr<IAudioEndpointVolume>
            endpointVolume;

        if (
            !GetEndpointVolumeInterface(
                device.Get(),
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