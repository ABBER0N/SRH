#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace srh::engine
{
    using NodeId = std::uint16_t;
    using PhysicalDeviceUid = std::uint64_t;

    enum class NodeType : std::uint8_t
    {
        Unknown = 0,
        ButtonBox,
        Pedals,
        Shifter,
        Handbrake,
        SteeringWheel,
        Display,
        Lighting,
        Haptics,
        Custom
    };

    enum class DeviceHealth : std::uint8_t
    {
        Unknown = 0,
        Healthy,
        Warning,
        Error
    };

    struct HubState
    {
        bool connected{ false };

        std::string name{ "SRH Hub" };

        std::string firmwareVersion;
        std::string protocolVersion;

        DeviceHealth health{
            DeviceHealth::Unknown
        };

        std::string statusMessage;
    };

    struct NodeState
    {
        NodeId nodeId{ 0 };
        PhysicalDeviceUid physicalUid{ 0 };

        NodeType type{
            NodeType::Unknown
        };

        std::string name;

        bool connected{ false };
        bool configured{ false };
        bool enabled{ true };
        bool active{ false };

        DeviceHealth health{
            DeviceHealth::Unknown
        };

        std::string firmwareVersion;
        std::string statusMessage;
    };

    struct DeviceSnapshot
    {
        HubState hub;
        std::vector<NodeState> nodes;
    };
}