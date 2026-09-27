#pragma once

#include "Domain/InputEvent.h"
#include "Domain/InputTypes.h"
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace srh::engine
{
    class InputStateStore
    {
    public:
        InputStateStore() = default;

        InputStateStore(
            const InputStateStore&
        ) = delete;

        InputStateStore& operator=(
            const InputStateStore&
            ) = delete;

        [[nodiscard]]
        bool Apply(
            const InputEvent& event
        );

        [[nodiscard]]
        std::optional<InputControlState>
            Find(
                NodeId nodeId,
                ControlId controlId
            ) const;

        [[nodiscard]]
        InputSnapshot GetSnapshot() const;

        void ClearNode(
            NodeId nodeId
        );

        void Clear();

    private:
        using ControlMap =
            std::unordered_map<
            ControlId,
            InputControlState
            >;

        mutable std::shared_mutex
            m_mutex;

        std::unordered_map<
            NodeId,
            ControlMap
        >
            m_nodes;
    };
}