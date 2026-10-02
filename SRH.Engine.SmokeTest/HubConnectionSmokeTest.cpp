#include "HubConnectionSmokeTest.h"

#include "Hub/HubConnectionService.h"

#include <Windows.h>

#include <iostream>

namespace
{
    using namespace
        srh::engine;

    using namespace
        srh::engine::hub;

    using namespace
        srh::engine::hub::protocol;

    bool TestInitialState()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        if (
            connection
            .IsTransportConnected()
            )
        {
            return false;
        }

        if (
            connection
            .IsSessionEstablished()
            )
        {
            return false;
        }

        if (
            !connection
            .PortName()
            .empty()
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestInvalidArguments()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        if (
            connection.Connect(
                ""
            )
            )
        {
            return false;
        }

        if (
            connection
            .IsTransportConnected()
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        if (
            connection.Connect(
                "COM1",
                0
            )
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        return true;
    }

    bool TestMissingPort()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        if (
            connection.Connect(
                "SRH_CONNECTION_PORT_DOES_NOT_EXIST"
            )
            )
        {
            return false;
        }

        if (
            connection
            .IsTransportConnected()
            )
        {
            return false;
        }

        if (
            connection
            .IsSessionEstablished()
            )
        {
            return false;
        }

        if (
            connection.LastError() ==
            ERROR_SUCCESS
            )
        {
            return false;
        }

        if (
            !connection
            .PortName()
            .empty()
            )
        {
            return false;
        }

        const auto hub =
            engine.GetHubState();

        if (hub.connected)
        {
            return false;
        }

        return true;
    }

    bool TestSendWhileDisconnected()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        HelloAckMessage message;

        message.heartbeatIntervalMs =
            1000;

        if (
            connection.SendHubMessage(
                HubMessage{
                    message
                },
                HubPacketFlags::
                Response
            )
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_INVALID_HANDLE
            )
        {
            return false;
        }

        if (
            connection
            .IsTransportConnected()
            )
        {
            return false;
        }

        return true;
    }

    bool TestInvalidOutgoingMessage()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        InputEvent event;

        event.nodeId =
            1;

        event.controlId =
            1;

        event.type =
            InputEventType::
            Unknown;

        event.value =
            1;

        event.timestamp =
            1;

        if (
            connection.SendHubMessage(
                HubMessage{
                    event
                }
            )
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_INVALID_DATA
            )
        {
            return false;
        }

        return true;
    }

    bool TestDisconnectLifecycle()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        if (
            connection.Connect(
                ""
            )
            )
        {
            return false;
        }

        if (
            connection.LastError() ==
            ERROR_SUCCESS
            )
        {
            return false;
        }

        connection.Disconnect();

        if (
            connection
            .IsTransportConnected()
            )
        {
            return false;
        }

        if (
            connection
            .IsSessionEstablished()
            )
        {
            return false;
        }

        if (
            connection.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        if (
            !connection
            .PortName()
            .empty()
            )
        {
            return false;
        }

        const auto state =
            engine.GetHubState();

        if (
            state.connected ||
            state.health !=
            DeviceHealth::
            Unknown ||
            state.statusMessage !=
            "Disconnected"
            )
        {
            return false;
        }

        connection.Disconnect();

        if (
            connection.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestRepeatedFailedConnect()
    {
        SrhEngine engine;

        HubConnectionService
            connection(
                engine
            );

        for (
            int attempt = 0;
            attempt < 3;
            ++attempt
            )
        {
            if (
                connection.Connect(
                    "SRH_CONNECTION_PORT_DOES_NOT_EXIST"
                )
                )
            {
                return false;
            }

            if (
                connection
                .IsTransportConnected()
                )
            {
                return false;
            }

            if (
                connection.LastError() ==
                ERROR_SUCCESS
                )
            {
                return false;
            }
        }

        connection.Disconnect();

        return
            !connection
            .IsTransportConnected() &&
            connection.LastError() ==
            ERROR_SUCCESS;
    }
}

namespace srh::smoketest
{
    bool RunHubConnectionSmokeTest()
    {
        if (!TestInitialState())
        {
            std::cout
                << "FAIL: Hub connection initial state\n";

            return false;
        }

        if (!TestInvalidArguments())
        {
            std::cout
                << "FAIL: Hub connection argument validation\n";

            return false;
        }

        if (!TestMissingPort())
        {
            std::cout
                << "FAIL: Hub connection missing port\n";

            return false;
        }

        if (!TestSendWhileDisconnected())
        {
            std::cout
                << "FAIL: Hub connection disconnected send\n";

            return false;
        }

        if (!TestInvalidOutgoingMessage())
        {
            std::cout
                << "FAIL: Hub connection outgoing validation\n";

            return false;
        }

        if (!TestDisconnectLifecycle())
        {
            std::cout
                << "FAIL: Hub connection disconnect lifecycle\n";

            return false;
        }

        if (!TestRepeatedFailedConnect())
        {
            std::cout
                << "FAIL: Hub connection reconnect lifecycle\n";

            return false;
        }

        std::cout
            << "INFO: Hub connection lifecycle verified offline\n";

        return true;
    }
}