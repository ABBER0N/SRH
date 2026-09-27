#include "pch.h"

#include "Execution/VirtualController/VirtualDeviceClient.h"

#include "Execution/VirtualController/VirtualDeviceInterface.h"

#include <Windows.h>
#include <SetupAPI.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "Setupapi.lib")

namespace srh::engine::virtual_controller
{
    struct VirtualDeviceClient::Impl
    {
        HANDLE handle{
            INVALID_HANDLE_VALUE
        };

        std::uint32_t lastError{
            ERROR_SUCCESS
        };
    };

    VirtualDeviceClient::
        VirtualDeviceClient()
        : m_impl(
            std::make_unique<Impl>()
        )
    {
    }

    VirtualDeviceClient::
        ~VirtualDeviceClient()
    {
        Close();
    }

    bool VirtualDeviceClient::Open(
        const std::wstring_view devicePath
    ) noexcept
    {
        Close();

        if (devicePath.empty())
        {
            m_impl->lastError =
                ERROR_INVALID_PARAMETER;

            return false;
        }

        const std::wstring path{
            devicePath
        };

        const HANDLE handle =
            ::CreateFileW(
                path.c_str(),
                GENERIC_READ |
                GENERIC_WRITE,
                FILE_SHARE_READ |
                FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );

        if (
            handle ==
            INVALID_HANDLE_VALUE
            )
        {
            m_impl->lastError =
                ::GetLastError();

            return false;
        }

        m_impl->handle =
            handle;

        m_impl->lastError =
            ERROR_SUCCESS;

        return true;
    }

    bool VirtualDeviceClient::
        OpenFirstAvailable() noexcept
    {
        Close();

        m_impl->lastError =
            ERROR_SUCCESS;

        const HDEVINFO deviceInfoSet =
            ::SetupDiGetClassDevsW(
                &DeviceInterfaceGuid,
                nullptr,
                nullptr,
                DIGCF_PRESENT |
                DIGCF_DEVICEINTERFACE
            );

        if (
            deviceInfoSet ==
            INVALID_HANDLE_VALUE
            )
        {
            m_impl->lastError =
                ::GetLastError();

            return false;
        }

        bool opened =
            false;

        DWORD interfaceIndex =
            0;

        for (
            ;;
            ++interfaceIndex
            )
        {
            SP_DEVICE_INTERFACE_DATA
                interfaceData{};

            interfaceData.cbSize =
                sizeof(
                    SP_DEVICE_INTERFACE_DATA
                    );

            if (
                !::SetupDiEnumDeviceInterfaces(
                    deviceInfoSet,
                    nullptr,
                    &DeviceInterfaceGuid,
                    interfaceIndex,
                    &interfaceData
                )
                )
            {
                const DWORD error =
                    ::GetLastError();

                if (
                    error ==
                    ERROR_NO_MORE_ITEMS
                    )
                {
                    break;
                }

                m_impl->lastError =
                    error;

                break;
            }

            DWORD requiredSize =
                0;

            ::SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet,
                &interfaceData,
                nullptr,
                0,
                &requiredSize,
                nullptr
            );

            const DWORD detailError =
                ::GetLastError();

            if (
                requiredSize == 0 ||
                detailError !=
                ERROR_INSUFFICIENT_BUFFER
                )
            {
                m_impl->lastError =
                    detailError;

                continue;
            }

            std::vector<std::byte>
                detailBuffer(
                    requiredSize
                );

            auto* detailData =
                reinterpret_cast<
                PSP_DEVICE_INTERFACE_DETAIL_DATA_W
                >(
                    detailBuffer.data()
                    );

            detailData->cbSize =
                sizeof(
                    SP_DEVICE_INTERFACE_DETAIL_DATA_W
                    );

            if (
                !::SetupDiGetDeviceInterfaceDetailW(
                    deviceInfoSet,
                    &interfaceData,
                    detailData,
                    requiredSize,
                    nullptr,
                    nullptr
                )
                )
            {
                m_impl->lastError =
                    ::GetLastError();

                continue;
            }

            if (
                Open(
                    detailData->DevicePath
                )
                )
            {
                opened =
                    true;

                break;
            }
        }

        ::SetupDiDestroyDeviceInfoList(
            deviceInfoSet
        );

        if (
            !opened &&
            m_impl->lastError ==
            ERROR_SUCCESS
            )
        {
            m_impl->lastError =
                ERROR_FILE_NOT_FOUND;
        }

        return opened;
    }

    void VirtualDeviceClient::Close()
        noexcept
    {
        if (
            m_impl->handle ==
            INVALID_HANDLE_VALUE
            )
        {
            return;
        }

        ::CloseHandle(
            m_impl->handle
        );

        m_impl->handle =
            INVALID_HANDLE_VALUE;
    }

    bool VirtualDeviceClient::IsOpen()
        const noexcept
    {
        return
            m_impl->handle !=
            INVALID_HANDLE_VALUE;
    }

    std::uint32_t
        VirtualDeviceClient::LastError()
        const noexcept
    {
        return
            m_impl->lastError;
    }

    bool VirtualDeviceClient::SubmitReport(
        const std::uint16_t deviceId,
        const InputReportV1& report
    ) noexcept
    {
        if (!IsOpen())
        {
            m_impl->lastError =
                ERROR_INVALID_HANDLE;

            return false;
        }

        SubmitReportRequestV1
            request;

        request.protocolVersion =
            ProtocolVersion;

        request.deviceId =
            deviceId;

        request.reserved =
            0;

        request.report =
            report;

        DWORD bytesReturned =
            0;

        const BOOL result =
            ::DeviceIoControl(
                m_impl->handle,
                static_cast<DWORD>(
                    IoctlSubmitReport
                    ),
                &request,
                static_cast<DWORD>(
                    sizeof(request)
                    ),
                nullptr,
                0,
                &bytesReturned,
                nullptr
            );

        if (!result)
        {
            m_impl->lastError =
                ::GetLastError();

            return false;
        }

        m_impl->lastError =
            ERROR_SUCCESS;

        return true;
    }
}