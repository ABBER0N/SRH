#pragma once

#include "Domain/Action.h"
#include "Domain/ExecutionTypes.h"
#include "Execution/Windows/Audio/AudioEndpointService.h"
#include "Execution/Windows/Audio/AudioService.h"

namespace srh::engine
{
    class WindowsActionExecutor
    {
    public:
        WindowsActionExecutor() = default;

        WindowsActionExecutor(
            const WindowsActionExecutor&
        ) = delete;

        WindowsActionExecutor& operator=(
            const WindowsActionExecutor&
            ) = delete;

        [[nodiscard]]
        ActionExecutionStatus Execute(
            const SystemAction& action
        ) const;

    private:
        [[nodiscard]]
        static bool SendKeyDown(
            std::uint32_t virtualKeyCode
        );

        [[nodiscard]]
        static bool SendKeyUp(
            std::uint32_t virtualKeyCode
        );

        [[nodiscard]]
        static bool SendKeyPress(
            std::uint32_t virtualKeyCode
        );

        AudioService
            m_audioService;

        AudioEndpointService
            m_audioEndpointService;
    };
}