#include "pch.h"

#include "Execution/Windows/WindowsActionExecutor.h"

#include <Windows.h>

#include <algorithm>

#pragma comment(lib, "User32.lib")

namespace srh::engine
{
    ActionExecutionStatus
        WindowsActionExecutor::Execute(
            const SystemAction& action
        ) const
    {
        switch (action.kind)
        {
            //
            // Master audio
            //

        case SystemActionKind::MasterVolumeSet:
            return
                m_audioService.SetMasterVolume(
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MasterVolumeAdjust:
        {
            const auto state =
                m_audioService.GetMasterState();

            if (!state.has_value())
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioService.SetMasterVolume(
                    state->volume +
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        case SystemActionKind::MasterMuteSet:
            return
                m_audioService.SetMasterMuted(
                    action.state
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MasterMuteToggle:
        {
            const auto state =
                m_audioService.GetMasterState();

            if (!state.has_value())
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioService.SetMasterMuted(
                    !state->muted
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        //
        // Application audio
        //

        case SystemActionKind::ApplicationVolumeSet:
            return
                !action.targetId.empty() &&
                m_audioService.SetSessionVolume(
                    action.targetId,
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::ApplicationVolumeAdjust:
        {
            const auto sessions =
                m_audioService.EnumerateSessions();

            const auto iterator =
                std::find_if(
                    sessions.begin(),
                    sessions.end(),
                    [&action](
                        const AudioSessionInfo& session
                        )
                    {
                        return
                            session.sessionInstanceId ==
                            action.targetId;
                    }
                );

            if (
                iterator ==
                sessions.end()
                )
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioService.SetSessionVolume(
                    action.targetId,
                    iterator->volume +
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        case SystemActionKind::ApplicationMuteSet:
            return
                !action.targetId.empty() &&
                m_audioService.SetSessionMuted(
                    action.targetId,
                    action.state
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::ApplicationMuteToggle:
        {
            const auto sessions =
                m_audioService.EnumerateSessions();

            const auto iterator =
                std::find_if(
                    sessions.begin(),
                    sessions.end(),
                    [&action](
                        const AudioSessionInfo& session
                        )
                    {
                        return
                            session.sessionInstanceId ==
                            action.targetId;
                    }
                );

            if (
                iterator ==
                sessions.end()
                )
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioService.SetSessionMuted(
                    action.targetId,
                    !iterator->muted
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        //
        // Audio endpoints
        //

        case SystemActionKind::EndpointVolumeSet:
            return
                !action.targetId.empty() &&
                m_audioEndpointService
                .SetEndpointVolume(
                    action.targetId,
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::EndpointVolumeAdjust:
        {
            const auto endpoint =
                m_audioEndpointService
                .FindEndpoint(
                    action.targetId
                );

            if (!endpoint.has_value())
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioEndpointService
                .SetEndpointVolume(
                    action.targetId,
                    endpoint->volume +
                    action.value
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        case SystemActionKind::EndpointMuteSet:
            return
                !action.targetId.empty() &&
                m_audioEndpointService
                .SetEndpointMuted(
                    action.targetId,
                    action.state
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::EndpointMuteToggle:
        {
            const auto endpoint =
                m_audioEndpointService
                .FindEndpoint(
                    action.targetId
                );

            if (!endpoint.has_value())
            {
                return
                    ActionExecutionStatus::Failed;
            }

            return
                m_audioEndpointService
                .SetEndpointMuted(
                    action.targetId,
                    !endpoint->muted
                )
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;
        }

        //
        // Media
        //

        case SystemActionKind::MediaPlayPause:
            return
                m_mediaService.TogglePlayPause()
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MediaPrevious:
            return
                m_mediaService.PreviousTrack()
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

        case SystemActionKind::MediaNext:
            return
                m_mediaService.NextTrack()
                ? ActionExecutionStatus::Executed
                : ActionExecutionStatus::Failed;

            //
            // Keyboard
            //

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

        default:
            return
                ActionExecutionStatus::
                Unsupported;
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