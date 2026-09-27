#include "pch.h"
#include "Input/InputStateStore.h"
#include <algorithm>
#include <mutex>

namespace srh::engine
{
    bool InputStateStore::Apply(
        const InputEvent& event
    )
    {
        if (
            event.type ==
            InputEventType::Unknown
            )
        {
            return false;
        }

        InputControlState state;

        state.nodeId =
            event.nodeId;

        state.controlId =
            event.controlId;

        state.type =
            event.type;

        state.timestamp =
            event.timestamp;

        switch (event.type)
        {
        case InputEventType::ButtonDown:
            state.value = 1;
            break;

        case InputEventType::ButtonUp:
            state.value = 0;
            break;

        case InputEventType::EncoderDelta:
        case InputEventType::AxisValue:
            state.value =
                event.value;
            break;

        case InputEventType::Unknown:
        default:
            return false;
        }

        std::unique_lock lock(
            m_mutex
        );

        m_nodes[
            state.nodeId
        ][
            state.controlId
        ] = state;

        return true;
    }

    std::optional<InputControlState>
        InputStateStore::Find(
            const NodeId nodeId,
            const ControlId controlId
        ) const
    {
        std::shared_lock lock(
            m_mutex
        );

        const auto nodeIterator =
            m_nodes.find(
                nodeId
            );

        if (
            nodeIterator ==
            m_nodes.end()
            )
        {
            return std::nullopt;
        }

        const auto controlIterator =
            nodeIterator->second.find(
                controlId
            );

        if (
            controlIterator ==
            nodeIterator->second.end()
            )
        {
            return std::nullopt;
        }

        return
            controlIterator->second;
    }

    InputSnapshot
        InputStateStore::GetSnapshot() const
    {
        std::shared_lock lock(
            m_mutex
        );

        InputSnapshot snapshot;

        std::size_t controlCount = 0;

        for (
            const auto& [nodeId, controls] :
            m_nodes
            )
        {
            (void)nodeId;

            controlCount +=
                controls.size();
        }

        snapshot.controls.reserve(
            controlCount
        );

        for (
            const auto& [nodeId, controls] :
            m_nodes
            )
        {
            (void)nodeId;

            for (
                const auto& [controlId, state] :
                controls
                )
            {
                (void)controlId;

                snapshot.controls.push_back(
                    state
                );
            }
        }

        std::sort(
            snapshot.controls.begin(),
            snapshot.controls.end(),
            [](
                const InputControlState& left,
                const InputControlState& right
                )
            {
                if (
                    left.nodeId !=
                    right.nodeId
                    )
                {
                    return
                        left.nodeId <
                        right.nodeId;
                }

                return
                    left.controlId <
                    right.controlId;
            }
        );

        return snapshot;
    }

    void InputStateStore::ClearNode(
        const NodeId nodeId
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_nodes.erase(
            nodeId
        );
    }

    void InputStateStore::Clear()
    {
        std::unique_lock lock(
            m_mutex
        );

        m_nodes.clear();
    }
}