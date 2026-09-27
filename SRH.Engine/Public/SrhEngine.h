#pragma once

#include "Domain/DeviceState.h"
#include "Domain/InputEvent.h"
#include "Domain/InputTypes.h"
#include <memory>
#include <optional>

namespace srh::engine
{
    class SrhEngine
    {
    public:
        SrhEngine();
        ~SrhEngine();

        SrhEngine(
            const SrhEngine&
        ) = delete;

        SrhEngine& operator=(
            const SrhEngine&
            ) = delete;

        SrhEngine(
            SrhEngine&&
        ) noexcept;

        SrhEngine& operator=(
            SrhEngine&&
            ) noexcept;

        //
        // Devices
        //

        void SetHubState(
            HubState state
        );

        void UpsertNodeState(
            NodeState state
        );

        void RemoveNode(
            NodeId nodeId
        );

        void ClearNodes();

        [[nodiscard]]
        HubState GetHubState() const;

        [[nodiscard]]
        std::optional<NodeState> FindNode(
            NodeId nodeId
        ) const;

        [[nodiscard]]
        DeviceSnapshot GetDeviceSnapshot() const;

        //
        // Input
        //

        [[nodiscard]]
        bool SubmitInputEvent(
            const InputEvent& event
        );

        [[nodiscard]]
        std::optional<InputControlState>
            FindInputState(
                NodeId nodeId,
                ControlId controlId
            ) const;

        [[nodiscard]]
        InputSnapshot GetInputSnapshot() const;

        void ClearInputState();

    private:
        class Impl;

        std::unique_ptr<Impl> m_impl;
    };
}