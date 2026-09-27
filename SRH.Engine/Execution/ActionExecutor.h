#pragma once

#include "Domain/Action.h"

#include <cstdint>
#include <functional>

namespace srh::engine
{
    enum class ActionExecutionStatus :
        std::uint8_t
    {
        Executed = 0,
        Unsupported,
        Failed
    };

    class ActionExecutor
    {
    public:
        using VirtualControllerHandler =
            std::function<
            ActionExecutionStatus(
                const VirtualControllerAction&
            )
            >;

        using SystemHandler =
            std::function<
            ActionExecutionStatus(
                const SystemAction&
            )
            >;

        using HubHandler =
            std::function<
            ActionExecutionStatus(
                const HubAction&
            )
            >;

        ActionExecutor() = default;

        ActionExecutor(
            const ActionExecutor&
        ) = delete;

        ActionExecutor& operator=(
            const ActionExecutor&
            ) = delete;

        void SetVirtualControllerHandler(
            VirtualControllerHandler handler
        );

        void SetSystemHandler(
            SystemHandler handler
        );

        void SetHubHandler(
            HubHandler handler
        );

        [[nodiscard]]
        ActionExecutionStatus Execute(
            const Action& action
        ) const;

    private:
        VirtualControllerHandler
            m_virtualControllerHandler;

        SystemHandler
            m_systemHandler;

        HubHandler
            m_hubHandler;
    };
}