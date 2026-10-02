#pragma once

#include "Game/Domain/TelemetryCommon.h"

#include <cstdint>
#include <string>
#include <vector>

namespace srh::engine::game
{
    struct GameState
    {
        std::string providerId;
        std::string providerVersion;

        std::string gameId;
        std::string gameName;
        std::string gameVersion;

        GameRunState runState{
            GameRunState::Unknown
        };

        bool connected{ false };
    };

    struct DriverIdentity
    {
        std::string firstName;
        std::string lastName;
        std::string shortName;

        std::string nationality;

        std::string category;
        std::string license;

        std::int32_t rating{ 0 };

        std::uint64_t
            providerDriverId{ 0 };
    };

    struct VehicleIdentity
    {
        std::string manufacturer;
        std::string model;
        std::string className;

        std::string teamName;

        std::string carNumber;

        std::uint64_t
            providerVehicleId{ 0 };
    };

    struct SessionIdentity
    {
        std::string eventName;
        std::string serverName;

        std::uint64_t
            providerEventId{ 0 };

        std::uint64_t
            providerSessionId{ 0 };
    };

    struct SessionState
    {
        SessionIdentity identity;

        SessionType type{
            SessionType::Unknown
        };

        SessionPhase phase{
            SessionPhase::Unknown
        };

        TelemetryValue<
            TelemetryTimestampUs
        > sessionTimeUs;

        TelemetryValue<
            TelemetryTimestampUs
        > remainingTimeUs;

        TelemetryValue<std::int32_t>
            completedLaps;

        TelemetryValue<std::int32_t>
            totalLaps;

        TelemetryValue<std::int32_t>
            playerOverallPosition;

        TelemetryValue<std::int32_t>
            playerClassPosition;

        TelemetryValue<std::int32_t>
            participantCount;

        bool multiplayer{ false };
        bool timedSession{ false };
        bool lapLimitedSession{ false };
    };

    struct TrackGeometryState
    {
        enum class Source
        {
            Unavailable,

            Provider,
            Learned,
            Cache
        };

        Source source{
            Source::Unavailable
        };

        std::vector<Vec3>
            centerLine;

        std::vector<Vec3>
            leftBoundary;

        std::vector<Vec3>
            rightBoundary;
    };

    struct TrackState
    {
        std::string name;
        std::string configuration;

        TelemetryValue<double>
            lengthMeters;

        TelemetryValue<double>
            altitudeMeters;

        TelemetryValue<double>
            pitSpeedLimitMps;

        std::vector<double>
            sectorStartNormalizedPositions;

        TelemetryValue<double>
            pitEntryNormalizedPosition;

        TelemetryValue<double>
            pitExitNormalizedPosition;

        TrackGeometryState geometry;
    };

    struct EnvironmentState
    {
        TelemetryValue<double>
            ambientTemperatureC;

        TelemetryValue<double>
            trackTemperatureC;

        TelemetryValue<double>
            airPressurePa;

        TelemetryValue<double>
            humidity01;

        TelemetryValue<double>
            rainIntensity01;

        TelemetryValue<double>
            trackWetness01;

        TelemetryValue<double>
            trackGrip01;

        TelemetryValue<Vec3>
            windVelocityMps;

        TelemetryValue<double>
            visibilityMeters;

        TelemetryValue<double>
            cloudCoverage01;

        TelemetryValue<double>
            timeOfDaySeconds;
    };

    struct WeatherForecastEntry
    {
        TelemetryValue<double>
            secondsFromNow;

        TelemetryValue<double>
            ambientTemperatureC;

        TelemetryValue<double>
            trackTemperatureC;

        TelemetryValue<double>
            rainIntensity01;

        TelemetryValue<double>
            cloudCoverage01;
    };

    struct ControlsState
    {
        TelemetryValue<double>
            throttleInput01;

        TelemetryValue<double>
            throttleOutput01;

        TelemetryValue<double>
            brakeInput01;

        TelemetryValue<double>
            brakeOutput01;

        TelemetryValue<double>
            clutchInput01;

        TelemetryValue<double>
            clutchOutput01;

        TelemetryValue<double>
            steeringInput;

        TelemetryValue<double>
            steeringAngleRad;

        TelemetryValue<double>
            handbrake01;
    };

    struct MotionState
    {
        TelemetryValue<Vec3>
            worldPositionMeters;

        TelemetryValue<Vec3>
            worldVelocityMps;

        TelemetryValue<Vec3>
            worldAccelerationMps2;

        TelemetryValue<Vec3>
            localVelocityMps;

        TelemetryValue<Vec3>
            localAccelerationMps2;

        TelemetryValue<Quaternion>
            orientation;

        TelemetryValue<Vec3>
            angularVelocityRadPerSec;

        TelemetryValue<Vec3>
            angularAccelerationRadPerSec2;

        TelemetryValue<double>
            yawRad;

        TelemetryValue<double>
            pitchRad;

        TelemetryValue<double>
            rollRad;

        TelemetryValue<double>
            speedMps;

        TelemetryValue<double>
            lateralG;

        TelemetryValue<double>
            longitudinalG;

        TelemetryValue<double>
            verticalG;
    };

    struct ForceFeedbackState
    {
        TelemetryValue<double>
            force01;

        TelemetryValue<double>
            clipping01;

        TelemetryValue<double>
            road01;

        TelemetryValue<double>
            kerb01;

        TelemetryValue<double>
            slip01;

        TelemetryValue<double>
            abs01;
    };

    struct PowertrainState
    {
        TelemetryValue<double>
            engineRpm;

        TelemetryValue<double>
            engineRpmMax;

        TelemetryValue<double>
            engineRpmLimiter;

        TelemetryValue<double>
            engineTorqueNm;

        TelemetryValue<double>
            enginePowerW;

        TelemetryValue<double>
            oilTemperatureC;

        TelemetryValue<double>
            oilPressurePa;

        TelemetryValue<double>
            coolantTemperatureC;

        TelemetryValue<double>
            turboBoostPa;

        TelemetryValue<bool>
            engineRunning;

        TelemetryValue<bool>
            engineStalled;

        TelemetryValue<bool>
            limiterActive;
    };

    struct TransmissionState
    {
        TelemetryValue<std::int32_t>
            gear;

        TelemetryValue<std::int32_t>
            requestedGear;

        TelemetryValue<
            TransmissionGearState
        > gearState;

        TelemetryValue<double>
            gearRatio;

        TelemetryValue<double>
            finalDriveRatio;

        TelemetryValue<bool>
            shifting;

        TelemetryValue<double>
            clutchSlip01;

        TelemetryValue<double>
            driveshaftRpm;
    };

    struct DifferentialState
    {
        TelemetryValue<double>
            locking01;

        TelemetryValue<double>
            slip01;

        TelemetryValue<double>
            inputTorqueNm;

        TelemetryValue<double>
            outputTorqueNm;
    };

    struct TireState
    {
        std::string compound;

        TelemetryValue<double>
            pressurePa;

        TelemetryValue<double>
            surfaceTemperatureC;

        TelemetryValue<double>
            coreTemperatureC;

        TelemetryValue<double>
            innerTemperatureC;

        TelemetryValue<double>
            middleTemperatureC;

        TelemetryValue<double>
            outerTemperatureC;

        TelemetryValue<double>
            wearFraction01;

        TelemetryValue<double>
            gripHealth01;

        TelemetryValue<double>
            dirt01;

        TelemetryValue<bool>
            punctured;

        TelemetryValue<bool>
            detached;
    };

    struct ContactPatchState
    {
        TelemetryValue<Vec3>
            worldPositionMeters;

        TelemetryValue<Vec3>
            normal;

        TelemetryValue<Vec3>
            heading;

        TelemetryValue<double>
            slipRatio;

        TelemetryValue<double>
            slipAngleRad;

        TelemetryValue<double>
            longitudinalForceN;

        TelemetryValue<double>
            lateralForceN;

        TelemetryValue<double>
            aligningTorqueNm;

        TelemetryValue<double>
            loadedRadiusMeters;
    };

    struct BrakeState
    {
        TelemetryValue<double>
            pressurePa;

        TelemetryValue<double>
            temperatureC;

        TelemetryValue<double>
            wearFraction01;

        TelemetryValue<double>
            padLife01;

        TelemetryValue<double>
            discLife01;

        TelemetryValue<bool>
            absActive;
    };

    struct SuspensionState
    {
        TelemetryValue<double>
            travelMeters;

        TelemetryValue<double>
            velocityMps;

        TelemetryValue<double>
            rideHeightMeters;

        TelemetryValue<double>
            springForceN;

        TelemetryValue<double>
            damperForceN;

        TelemetryValue<double>
            bumpStopForceN;
    };

    struct WheelState
    {
        WheelPosition position{
            WheelPosition::Unknown
        };

        TelemetryValue<double>
            angularVelocityRadPerSec;

        TelemetryValue<double>
            speedMps;

        TelemetryValue<double>
            loadN;

        TelemetryValue<double>
            slipRatio;

        TelemetryValue<double>
            slipAngleRad;

        TelemetryValue<double>
            camberRad;

        TelemetryValue<double>
            toeRad;

        TelemetryValue<bool>
            locking;

        TelemetryValue<bool>
            spinning;

        TelemetryValue<SurfaceType>
            surface;

        TireState tire;
        ContactPatchState contactPatch;
        BrakeState brake;
        SuspensionState suspension;
    };

    struct BrakeSystemState
    {
        TelemetryValue<double>
            frontBias01;

        TelemetryValue<double>
            migration01;
    };

    struct AeroState
    {
        TelemetryValue<double>
            frontDownforceN;

        TelemetryValue<double>
            rearDownforceN;

        TelemetryValue<double>
            dragN;

        TelemetryValue<double>
            frontRideHeightMeters;

        TelemetryValue<double>
            rearRideHeightMeters;

        TelemetryValue<double>
            frontWingAngleRad;

        TelemetryValue<double>
            rearWingAngleRad;

        TelemetryValue<bool>
            drsAvailable;

        TelemetryValue<bool>
            drsActive;

        TelemetryValue<bool>
            pushToPassAvailable;

        TelemetryValue<bool>
            pushToPassActive;
    };

    struct ElectronicsState
    {
        TelemetryValue<std::int32_t>
            tractionControlLevel;

        TelemetryValue<std::int32_t>
            tractionControlCutLevel;

        TelemetryValue<std::int32_t>
            absLevel;

        TelemetryValue<std::int32_t>
            engineMap;

        TelemetryValue<std::int32_t>
            fuelMap;

        TelemetryValue<std::int32_t>
            brakeMap;

        TelemetryValue<double>
            brakeBias01;

        TelemetryValue<bool>
            pitLimiterActive;

        TelemetryValue<bool>
            tractionControlActive;

        TelemetryValue<bool>
            absActive;

        TelemetryValue<bool>
            ignitionOn;

        TelemetryValue<bool>
            starterActive;
    };

    struct DriverControl
    {
        std::string id;
        std::string displayName;

        TelemetryValue<double>
            numericValue;

        TelemetryValue<std::int32_t>
            discreteValue;

        std::string displayValue;
    };

    struct InstrumentationState
    {
        TelemetryValue<std::int32_t>
            dashPage;

        TelemetryValue<double>
            shiftLightStartRpm;

        TelemetryValue<double>
            shiftLightEndRpm;

        TelemetryValue<bool>
            shiftLightActive;

        TelemetryValue<bool>
            warningLightActive;

        std::vector<std::string>
            activeWarnings;
    };

    struct LightingWiperState
    {
        TelemetryValue<bool>
            headlights;

        TelemetryValue<bool>
            highBeam;

        TelemetryValue<bool>
            leftIndicator;

        TelemetryValue<bool>
            rightIndicator;

        TelemetryValue<bool>
            hazardLights;

        TelemetryValue<bool>
            rainLight;

        TelemetryValue<std::int32_t>
            wiperStage;
    };

    struct EnergyState
    {
        TelemetryValue<double>
            fuelLiters;

        TelemetryValue<double>
            fuelCapacityLiters;

        TelemetryValue<double>
            fuelMassKg;

        TelemetryValue<double>
            fuelUsedLiters;

        TelemetryValue<double>
            fuelPerLapLiters;

        TelemetryValue<double>
            estimatedFuelLaps;

        TelemetryValue<double>
            batteryStateOfCharge01;

        TelemetryValue<double>
            batteryEnergyJ;

        TelemetryValue<double>
            electricPowerW;

        TelemetryValue<double>
            deploymentPowerW;

        TelemetryValue<double>
            regenerationPowerW;

        TelemetryValue<double>
            deploymentRemaining01;

        TelemetryValue<bool>
            hybridActive;
    };

    struct DamageState
    {
        TelemetryValue<double>
            engineDamage01;

        TelemetryValue<double>
            gearboxDamage01;

        TelemetryValue<double>
            frontAeroDamage01;

        TelemetryValue<double>
            rearAeroDamage01;

        TelemetryValue<double>
            bodyDamage01;

        std::vector<
            TelemetryValue<double>
        > suspensionDamage01;

        std::vector<
            TelemetryValue<double>
        > wheelDamage01;
    };

    struct TimingState
    {
        TelemetryValue<
            TelemetryTimestampUs
        > currentLapTimeUs;

        TelemetryValue<
            TelemetryTimestampUs
        > lastLapTimeUs;

        TelemetryValue<
            TelemetryTimestampUs
        > bestLapTimeUs;

        std::vector<
            TelemetryValue<
            TelemetryTimestampUs
            >
        > currentSectorTimesUs;

        std::vector<
            TelemetryValue<
            TelemetryTimestampUs
            >
        > bestSectorTimesUs;

        TelemetryValue<std::int64_t>
            deltaToBestUs;

        TelemetryValue<std::int64_t>
            deltaToLeaderUs;

        TelemetryValue<std::int64_t>
            gapAheadUs;

        TelemetryValue<std::int64_t>
            gapBehindUs;

        TelemetryValue<bool>
            currentLapValid;
    };

    struct PitState
    {
        TelemetryValue<bool>
            requested;

        TelemetryValue<bool>
            inPitLane;

        TelemetryValue<bool>
            inPitBox;

        TelemetryValue<bool>
            limiterRequired;

        TelemetryValue<double>
            speedLimitMps;

        TelemetryValue<bool>
            refueling;

        TelemetryValue<bool>
            changingTires;

        TelemetryValue<bool>
            repairing;

        TelemetryValue<
            TelemetryTimestampUs
        > pitStopTimeUs;

        TelemetryValue<
            TelemetryTimestampUs
        > driverStintRemainingUs;
    };

    struct StrategyState
    {
        TelemetryValue<double>
            plannedFuelLiters;

        std::string plannedTireCompound;

        TelemetryValue<std::int32_t>
            plannedTireSet;

        std::vector<
            TelemetryValue<double>
        > plannedTirePressuresPa;

        TelemetryValue<bool>
            mandatoryStopRequired;

        TelemetryValue<std::int32_t>
            remainingMandatoryStops;
    };

    struct VehicleState
    {
        DriverIdentity driver;
        VehicleIdentity identity;

        ControlsState controls;
        MotionState motion;
        ForceFeedbackState forceFeedback;

        PowertrainState powertrain;
        TransmissionState transmission;
        DifferentialState differential;

        std::vector<WheelState>
            wheels;

        BrakeSystemState brakes;
        AeroState aero;

        ElectronicsState electronics;

        std::vector<DriverControl>
            driverControls;

        InstrumentationState
            instrumentation;

        LightingWiperState
            lightingWipers;

        EnergyState energy;
        DamageState damage;

        TimingState timing;
        PitState pit;
        StrategyState strategy;
    };

    struct ParticipantState
    {
        std::uint64_t
            participantId{ 0 };

        std::int32_t
            providerCarIndex{ -1 };

        std::int32_t
            providerDriverIndex{ -1 };

        DriverIdentity driver;
        VehicleIdentity vehicle;

        bool isPlayer{ false };
        bool active{ false };
        bool connected{ false };

        TrackLocation trackLocation{
            TrackLocation::Unknown
        };

        TelemetryValue<std::int32_t>
            overallPosition;

        TelemetryValue<std::int32_t>
            classPosition;

        TelemetryValue<std::int32_t>
            trackPosition;

        TelemetryValue<std::int32_t>
            completedLaps;

        TelemetryValue<double>
            normalizedTrackPosition;

        ParticipantPositionSource
            positionSource{
                ParticipantPositionSource::
                    Unavailable
        };

        TelemetryValue<Vec3>
            worldPositionMeters;

        TelemetryValue<Vec3>
            worldVelocityMps;

        TelemetryValue<Quaternion>
            orientation;

        TelemetryValue<double>
            headingRad;

        TelemetryValue<double>
            speedMps;

        TelemetryValue<std::int32_t>
            gear;

        TelemetryValue<double>
            engineRpm;

        TelemetryValue<double>
            steeringAngleRad;

        std::string tireCompound;

        TimingState timing;
    };

    struct PenaltyState
    {
        PenaltyType type{
            PenaltyType::Unknown
        };

        std::string description;

        TelemetryValue<
            TelemetryTimestampUs
        > timePenaltyUs;

        TelemetryValue<std::int32_t>
            lapsRemaining;

        bool served{ false };
    };

    struct RaceControlState
    {
        RaceFlag globalFlag{
            RaceFlag::Unknown
        };

        RaceFlag localFlag{
            RaceFlag::Unknown
        };

        std::vector<RaceFlag>
            sectorFlags;

        bool safetyCar{ false };
        bool virtualSafetyCar{ false };
        bool formationLap{ false };

        TelemetryValue<std::int32_t>
            incidents;

        TelemetryValue<std::int32_t>
            incidentLimit;

        TelemetryValue<std::int32_t>
            trackLimitWarnings;

        std::vector<PenaltyState>
            penalties;
    };

    struct RelativeParticipantState
    {
        std::uint64_t
            participantId{ 0 };

        TelemetryValue<double>
            rightMeters;

        TelemetryValue<double>
            forwardMeters;

        TelemetryValue<double>
            verticalMeters;

        TelemetryValue<double>
            distanceMeters;

        TelemetryValue<double>
            relativeSpeedMps;
    };

    struct ProximityState
    {
        std::vector<
            RelativeParticipantState
        > participants;

        TelemetryValue<bool>
            carLeft;

        TelemetryValue<bool>
            carRight;

        TelemetryValue<bool>
            carLeftRight;
    };

    struct NetworkPerformanceState
    {
        TelemetryValue<double>
            latencyMs;

        TelemetryValue<double>
            quality01;

        TelemetryValue<double>
            packetLoss01;

        TelemetryValue<double>
            framesPerSecond;

        TelemetryValue<double>
            cpuUsage01;
    };

    struct TelemetryFrameMeta
    {
        std::uint64_t frameId{ 0 };

        TelemetryTimestampUs
            sourceTimestampUs{ 0 };

        TelemetryTimestampUs
            hostReceiveTimestampUs{ 0 };

        TelemetryTimestampUs
            ageUs{ 0 };

        double sourceRateHz{ 0.0 };

        bool stale{ false };
    };
}