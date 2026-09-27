#include "pch.h"

#include "Execution/Windows/WindowsActionExecutor.h"

#include <Windows.h>

namespace srh::engine
{
    ActionExecutionStatus
        WindowsActionExecutor::Execute(
            const SystemAction& action
        ) const
    {
        switch (action.kind)
        {
        case SystemActionKind::KeyboardDown:
            return
                SendKeyDown(
                    action.code
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::KeyboardUp:
            return
                SendKeyUp(
                    action.code
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::KeyboardPress:
            return
                SendKeyPress(
                    action.code
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MediaPlayPause:
            return
                SendKeyPress(
                    VK_MEDIA_PLAY_PAUSE
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MediaPrevious:
            return
                SendKeyPress(
                    VK_MEDIA_PREV_TRACK
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MediaNext:
            return
                SendKeyPress(
                    VK_MEDIA_NEXT_TRACK
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        default:
            return
                ActionExecutionStatus::Unsupported;
        }
    }

    bool WindowsActionExecutor::SendKeyDown(
        const std::uint32_t virtualKeyCode
    )
    {
        if (
            virtualKeyCode == 0 ||
            virtualKeyCode > 0xFF
            )
        {
            return false;
        }

        INPUT input{};

        input.type =
            INPUT_KEYBOARD;

        input.ki.wVk =
            static_cast<WORD>(
                virtualKeyCode
                );

        input.ki.dwFlags =
            0;

        return
            SendInput(
                1,
                &input,
                sizeof(INPUT)
            ) == 1;
    }

    bool WindowsActionExecutor::SendKeyUp(
        const std::uint32_t virtualKeyCode
    )
    {
        if (
            virtualKeyCode == 0 ||
            virtualKeyCode > 0xFF
            )
        {
            return false;
        }

        INPUT input{};

        input.type =
            INPUT_KEYBOARD;

        input.ki.wVk =
            static_cast<WORD>(
                virtualKeyCode
                );

        input.ki.dwFlags =
            KEYEVENTF_KEYUP;

        return
            SendInput(
                1,
                &input,
                sizeof(INPUT)
            ) == 1;
    }

    bool WindowsActionExecutor::SendKeyPress(
        const std::uint32_t virtualKeyCode
    )
    {
        if (
            virtualKeyCode == 0 ||
            virtualKeyCode > 0xFF
            )
        {
            return false;
        }

        INPUT inputs[2]{};

        inputs[0].type =
            INPUT_KEYBOARD;

        inputs[0].ki.wVk =
            static_cast<WORD>(
                virtualKeyCode
                );

        inputs[1].type =
            INPUT_KEYBOARD;

        inputs[1].ki.wVk =
            static_cast<WORD>(
                virtualKeyCode
                );

        inputs[1].ki.dwFlags =
            KEYEVENTF_KEYUP;

        return
            SendInput(
                2,
                inputs,
                sizeof(INPUT)
            ) == 2;
    }
}