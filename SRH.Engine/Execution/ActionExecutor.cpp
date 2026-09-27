#include "pch.h"

#include "Execution/ActionExecutor.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace srh::engine
{
    void ActionExecutor::SetVirtualControllerHandler(
        VirtualControllerHandler handler
    )
    {
        m_virtualControllerHandler =
            std::move(handler);
    }

    void ActionExecutor::SetSystemHandler(
        SystemHandler handler
    )
    {
        m_systemHandler =
            std::move(handler);
    }

    void ActionExecutor::SetHubHandler(
        HubHandler handler
    )
    {
        m_hubHandler =
            std::move(handler);
    }

    ActionExecutionStatus ActionExecutor::Execute(
        const Action& action
    ) const
    {
        return std::visit(
            [this](
                const auto& concreteAction
                ) -> ActionExecutionStatus
            {
                using ActionType =
                    std::decay_t<
                    decltype(
                        concreteAction
                        )
                    >;

                if constexpr (
                    std::is_same_v<
                    ActionType,
                    VirtualControllerAction
                    >
                    )
                {
                    if (
                        !m_virtualControllerHandler
                        )
                    {
                        return
                            ActionExecutionStatus::
                            Unsupported;
                    }

                    return
                        m_virtualControllerHandler(
                            concreteAction
                        );
                }
                else if constexpr (
                    std::is_same_v<
                    ActionType,
                    SystemAction
                    >
                    )
                {
                    if (
                        !m_systemHandler
                        )
                    {
                        return
                            ActionExecutionStatus::
                            Unsupported;
                    }

                    return
                        m_systemHandler(
                            concreteAction
                        );
                }
                else if constexpr (
                    std::is_same_v<
                    ActionType,
                    HubAction
                    >
                    )
                {
                    if (
                        !m_hubHandler
                        )
                    {
                        return
                            ActionExecutionStatus::
                            Unsupported;
                    }

                    return
                        m_hubHandler(
                            concreteAction
                        );
                }

                return
                    ActionExecutionStatus::
                    Unsupported;
            },
            action
        );
    }
}