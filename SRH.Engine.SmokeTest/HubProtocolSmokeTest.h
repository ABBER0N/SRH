#pragma once

#include "HubConnectionSmokeTest.h"
#include "HubIngressSmokeTest.h"
#include "HubMessageSmokeTest.h"
#include "HubSerialTransportSmokeTest.h"
#include "TelemetryDomainSmokeTest.h"
#include "TelemetryServiceSmokeTest.h"

namespace srh::smoketest
{
    [[nodiscard]]
    bool RunHubProtocolSmokeTest();
}