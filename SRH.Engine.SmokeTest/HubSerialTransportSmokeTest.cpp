#include "HubSerialTransportSmokeTest.h"

#include "Hub/Transport/HubSerialTransport.h"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>

namespace
{
    using srh::engine::hub::
        HubSerialTransport;

    bool TestInitialState()
    {
        HubSerialTransport
            transport;

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            !transport
            .PortName()
            .empty()
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestInvalidOpenArguments()
    {
        HubSerialTransport
            transport;

        //
        // Empty port name.
        //

        if (
            transport.Open(
                ""
            )
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        //
        // Zero baud rate must be rejected before
        // Windows tries to open the supplied port.
        //
        // COM1 is deliberately irrelevant here:
        // the argument validation happens first.
        //

        if (
            transport.Open(
                "COM1",
                0
            )
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            !transport
            .PortName()
            .empty()
            )
        {
            return false;
        }

        return true;
    }

    bool TestClosedRead()
    {
        HubSerialTransport
            transport;

        std::uint8_t buffer[16]{};

        std::size_t bytesRead =
            123;

        const bool result =
            transport.ReadSome(
                buffer,
                sizeof(buffer),
                bytesRead
            );

        if (result)
        {
            return false;
        }

        if (
            bytesRead !=
            0
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_HANDLE
            )
        {
            return false;
        }

        return true;
    }

    bool TestClosedWrite()
    {
        HubSerialTransport
            transport;

        const std::uint8_t data[]
        {
            0x53,
            0x52,
            0x48,
            0x50
        };

        const bool result =
            transport.WriteAll(
                data,
                sizeof(data)
            );

        if (result)
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_HANDLE
            )
        {
            return false;
        }

        return true;
    }

    bool TestNullBufferValidation()
    {
        HubSerialTransport
            transport;

        std::size_t bytesRead =
            999;

        if (
            transport.ReadSome(
                nullptr,
                1,
                bytesRead
            )
            )
        {
            return false;
        }

        if (
            bytesRead !=
            0
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        if (
            transport.WriteAll(
                nullptr,
                1
            )
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        return true;
    }

    bool TestZeroLengthOperations()
    {
        HubSerialTransport
            transport;

        std::size_t bytesRead =
            999;

        //
        // Zero-byte operations are valid no-ops,
        // even while the transport is closed.
        //

        if (
            !transport.ReadSome(
                nullptr,
                0,
                bytesRead
            )
            )
        {
            return false;
        }

        if (
            bytesRead !=
            0
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        if (
            !transport.WriteAll(
                nullptr,
                0
            )
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestMissingPort()
    {
        HubSerialTransport
            transport;

        //
        // This is deliberately not a valid COM
        // device name.
        //
        // CreateFileW() must fail without touching
        // any real serial device in the system.
        //

        const bool result =
            transport.Open(
                "SRH_PORT_DOES_NOT_EXIST"
            );

        if (result)
        {
            return false;
        }

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            !transport
            .PortName()
            .empty()
            )
        {
            return false;
        }

        //
        // The exact Windows error may vary between
        // system versions/configurations:
        //
        // ERROR_FILE_NOT_FOUND,
        // ERROR_PATH_NOT_FOUND,
        // ERROR_INVALID_NAME, etc.
        //
        // It must simply be a real Windows error.
        //

        if (
            transport.LastError() ==
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestPrefixedMissingPort()
    {
        HubSerialTransport
            transport;

        //
        // HubSerialTransport also accepts an
        // already-prefixed Win32 device path.
        //
        // This verifies that the implementation
        // does not prepend "\\.\" twice.
        //

        const bool result =
            transport.Open(
                "\\\\.\\SRH_PORT_DOES_NOT_EXIST"
            );

        if (result)
        {
            return false;
        }

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            transport.LastError() ==
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestCloseIsIdempotent()
    {
        HubSerialTransport
            transport;

        //
        // Create an error state first.
        //

        if (
            transport.Open(
                ""
            )
            )
        {
            return false;
        }

        if (
            transport.LastError() ==
            ERROR_SUCCESS
            )
        {
            return false;
        }

        transport.Close();

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        if (
            !transport
            .PortName()
            .empty()
            )
        {
            return false;
        }

        //
        // Closing an already closed transport must
        // remain harmless.
        //

        transport.Close();

        if (
            transport.IsOpen()
            )
        {
            return false;
        }

        if (
            transport.LastError() !=
            ERROR_SUCCESS
            )
        {
            return false;
        }

        return true;
    }

    bool TestMoveConstruction()
    {
        HubSerialTransport
            source;

        //
        // Put some observable state into source.
        //

        std::size_t bytesRead =
            0;

        std::uint8_t buffer[1]{};

        if (
            source.ReadSome(
                buffer,
                sizeof(buffer),
                bytesRead
            )
            )
        {
            return false;
        }

        if (
            source.LastError() !=
            ERROR_INVALID_HANDLE
            )
        {
            return false;
        }

        HubSerialTransport destination(
            std::move(
                source
            )
        );

        //
        // The PImpl state must have moved into the
        // destination object.
        //

        if (
            destination.IsOpen()
            )
        {
            return false;
        }

        if (
            destination.LastError() !=
            ERROR_INVALID_HANDLE
            )
        {
            return false;
        }

        //
        // A moved-from instance must remain safe to
        // reuse. Open() recreates its PImpl.
        //

        if (
            source.Open(
                ""
            )
            )
        {
            return false;
        }

        if (
            source.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        return true;
    }

    bool TestMoveAssignment()
    {
        HubSerialTransport
            source;

        HubSerialTransport
            destination;

        //
        // Give source a known error state.
        //

        if (
            source.Open(
                ""
            )
            )
        {
            return false;
        }

        if (
            source.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        destination =
            std::move(
                source
            );

        if (
            destination.IsOpen()
            )
        {
            return false;
        }

        if (
            destination.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        //
        // Reuse moved-from source.
        //

        std::size_t bytesRead =
            0;

        std::uint8_t buffer[1]{};

        if (
            source.ReadSome(
                buffer,
                sizeof(buffer),
                bytesRead
            )
            )
        {
            return false;
        }

        //
        // ReadSome() on a moved-from object has no
        // PImpl yet and therefore returns false.
        //
        // Open() is the operation that recreates
        // that internal state.
        //

        if (
            source.Open(
                ""
            )
            )
        {
            return false;
        }

        if (
            source.LastError() !=
            ERROR_INVALID_PARAMETER
            )
        {
            return false;
        }

        return true;
    }
}

namespace srh::smoketest
{
    bool RunHubSerialTransportSmokeTest()
    {
        if (!TestInitialState())
        {
            std::cout
                << "FAIL: Hub serial initial state\n";

            return false;
        }

        if (!TestInvalidOpenArguments())
        {
            std::cout
                << "FAIL: Hub serial argument validation\n";

            return false;
        }

        if (!TestClosedRead())
        {
            std::cout
                << "FAIL: Hub serial closed read\n";

            return false;
        }

        if (!TestClosedWrite())
        {
            std::cout
                << "FAIL: Hub serial closed write\n";

            return false;
        }

        if (!TestNullBufferValidation())
        {
            std::cout
                << "FAIL: Hub serial buffer validation\n";

            return false;
        }

        if (!TestZeroLengthOperations())
        {
            std::cout
                << "FAIL: Hub serial zero-length operations\n";

            return false;
        }

        if (!TestMissingPort())
        {
            std::cout
                << "FAIL: Hub serial missing port handling\n";

            return false;
        }

        if (!TestPrefixedMissingPort())
        {
            std::cout
                << "FAIL: Hub serial Win32 path handling\n";

            return false;
        }

        if (!TestCloseIsIdempotent())
        {
            std::cout
                << "FAIL: Hub serial close lifecycle\n";

            return false;
        }

        if (!TestMoveConstruction())
        {
            std::cout
                << "FAIL: Hub serial move construction\n";

            return false;
        }

        if (!TestMoveAssignment())
        {
            std::cout
                << "FAIL: Hub serial move assignment\n";

            return false;
        }

        std::cout
            << "INFO: Hub serial transport API verified offline\n";

        return true;
    }
}