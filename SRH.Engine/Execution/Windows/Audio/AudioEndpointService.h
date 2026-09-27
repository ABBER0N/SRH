#pragma once

#include <optional>
#include <string>
#include <vector>

namespace srh::engine
{
    enum class AudioEndpointType
    {
        Render,
        Capture
    };

    struct AudioEndpointInfo
    {
        std::string id;
        std::string name;

        AudioEndpointType type{
            AudioEndpointType::Render
        };

        bool isDefault{ false };

        float volume{ 0.0f };
        bool muted{ false };
    };

    class AudioEndpointService
    {
    public:
        AudioEndpointService() = default;

        AudioEndpointService(
            const AudioEndpointService&
        ) = delete;

        AudioEndpointService& operator=(
            const AudioEndpointService&
            ) = delete;

        [[nodiscard]]
        std::vector<AudioEndpointInfo>
            EnumerateOutputDevices() const;

        [[nodiscard]]
        std::vector<AudioEndpointInfo>
            EnumerateInputDevices() const;

        [[nodiscard]]
        std::optional<AudioEndpointInfo>
            FindEndpoint(
                const std::string& endpointId
            ) const;

        [[nodiscard]]
        bool SetEndpointVolume(
            const std::string& endpointId,
            float volume
        ) const;

        [[nodiscard]]
        bool SetEndpointMuted(
            const std::string& endpointId,
            bool muted
        ) const;

    private:
        [[nodiscard]]
        std::vector<AudioEndpointInfo>
            EnumerateDevices(
                AudioEndpointType type
            ) const;
    };
}