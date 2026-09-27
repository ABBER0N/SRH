#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace srh::engine::virtual_controller
{
    inline constexpr std::uint32_t
        ProtocolVersion = 1;

    inline constexpr std::uint8_t
        InputReportId = 1;

    inline constexpr std::size_t
        ButtonCount = 128;

    inline constexpr std::size_t
        ButtonByteCount = 16;

    inline constexpr std::size_t
        AxisCount = 8;

    inline constexpr std::size_t
        PovCount = 4;

    //
    // Equivalent to:
    //
    // CTL_CODE(
    //     FILE_DEVICE_UNKNOWN,
    //     0x800,
    //     METHOD_BUFFERED,
    //     FILE_WRITE_DATA
    // )
    //

    inline constexpr std::uint32_t
        IoctlSubmitReport =
        0x0022A000;

#pragma pack(push, 1)

    struct InputReportV1
    {
        std::uint8_t reportId{
            InputReportId
        };

        //
        // 128 buttons.
        //

        std::uint8_t buttons[
            ButtonByteCount
        ]{};

            //
            // 8 signed 16-bit axes.
            //

            std::int16_t axes[
                AxisCount
            ]{};

                //
                // 4 POV hats.
                //
                // 0..7 = directions
                // 8    = centered
                //

                std::uint8_t pov[
                    PovCount
                ]{
                    8,
                    8,
                    8,
                    8
                };
    };

    struct SubmitReportRequestV1
    {
        std::uint32_t protocolVersion{
            ProtocolVersion
        };

        std::uint16_t deviceId{ 0 };

        std::uint16_t reserved{ 0 };

        InputReportV1 report{};
    };

#pragma pack(pop)

    static_assert(
        sizeof(InputReportV1) ==
        37
        );

    static_assert(
        sizeof(SubmitReportRequestV1) ==
        45
        );

    static_assert(
        std::is_trivially_copyable_v<
        InputReportV1
        >
        );

    static_assert(
        std::is_trivially_copyable_v<
        SubmitReportRequestV1
        >
        );
}