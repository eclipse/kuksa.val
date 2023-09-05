/********************************************************************************
* Copyright (c) 2023 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License 2.0 which is available at
* http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

use lazy_static::lazy_static;
use regex::Regex;

#[derive(Debug)]
pub enum Error {
    RegexError,
}

pub fn to_regex_string(glob: &str) -> String {
    // Construct regular expression

    // Make sure we match the whole thing
    // Start from the beginning
    let mut re = String::from("^");

    if glob.eq("*") {
        return re.replace('^', "\x00");
    } else if glob.is_empty() {
        return re;
    }

    let mut regex_string = glob.replace('.', "\\.");

    if !glob.contains('*') {
        regex_string += "(?:\\..+)?";
    } else {
        if glob.starts_with("**.") {
            regex_string = regex_string.replace("**\\.", ".+\\.");
        } else if glob.starts_with("*.") {
            regex_string = regex_string.replace("*\\.", "[^.]+\\.");
        }

        if glob.ends_with(".*") {
            regex_string = regex_string.replace("\\.*", "\\.[^.]+");
        } else if glob.ends_with(".**") {
            regex_string = regex_string.replace("\\.**", "\\..*");
        }

        if glob.contains(".**.") {
            regex_string = regex_string.replace("\\.**", "[\\.[A-Z][a-zA-Z0-9]*]*");
        }

        if glob.contains(".*.") {
            regex_string = regex_string.replace("\\.*", "\\.[^.]+");
        }
    }

    re.push_str(regex_string.as_str());

    // And finally, make sure we match until EOL
    re.push('$');

    re
}

pub fn to_regex(glob: &str) -> Result<Regex, Error> {
    let re = to_regex_string(glob);
    Regex::new(&re).map_err(|_err| Error::RegexError)
}

lazy_static! {
    static ref REGEX_VALID_PATTERN: regex::Regex = regex::Regex::new(
        r"(?x)
        ^  # anchor at start (only match full paths)
        # At least one subpath (consisting of either of three):
        (?:
            [A-Z][a-zA-Z0-9]*  # An alphanumeric name (first letter capitalized)
            |
            \*                 # An asterisk
            |
            \*\*               # A double asterisk
        )
        # which can be followed by ( separator + another subpath )
        # repeated any number of times
        (?:
            \. # Separator, literal dot
            (?:
                [A-Z][a-zA-Z0-9]*  # Alphanumeric name (first letter capitalized)
                |
                \*                 # An asterisk
                |
                \*\*               # A double asterisk
            )
        )*
        $  # anchor at end (to match only full paths)
        ",
    )
    .expect("regex compilation (of static pattern) should always succeed");
}

pub fn is_valid_pattern(input: &str) -> bool {
    REGEX_VALID_PATTERN.is_match(input)
}

#[cfg(test)]
mod tests {
    use super::*;

    static ALL_SIGNALS: &[&str] = &[
        "Vehicle.ADAS.ABS.IsEnabled",
        "Vehicle.ADAS.ABS.IsEngaged",
        "Vehicle.ADAS.ABS.IsError",
        "Vehicle.ADAS.ActiveAutonomyLevel",
        "Vehicle.ADAS.CruiseControl.IsActive",
        "Vehicle.ADAS.CruiseControl.IsEnabled",
        "Vehicle.ADAS.CruiseControl.IsError",
        "Vehicle.ADAS.CruiseControl.SpeedSet",
        "Vehicle.ADAS.DMS.IsEnabled",
        "Vehicle.ADAS.DMS.IsError",
        "Vehicle.ADAS.DMS.IsWarning",
        "Vehicle.ADAS.EBA.IsEnabled",
        "Vehicle.ADAS.EBA.IsEngaged",
        "Vehicle.ADAS.EBA.IsError",
        "Vehicle.ADAS.EBD.IsEnabled",
        "Vehicle.ADAS.EBD.IsEngaged",
        "Vehicle.ADAS.EBD.IsError",
        "Vehicle.ADAS.ESC.IsEnabled",
        "Vehicle.ADAS.ESC.IsEngaged",
        "Vehicle.ADAS.ESC.IsError",
        "Vehicle.ADAS.ESC.IsStrongCrossWindDetected",
        "Vehicle.ADAS.ESC.RoadFriction.LowerBound",
        "Vehicle.ADAS.ESC.RoadFriction.MostProbable",
        "Vehicle.ADAS.ESC.RoadFriction.UpperBound",
        "Vehicle.ADAS.LaneDepartureDetection.IsEnabled",
        "Vehicle.ADAS.LaneDepartureDetection.IsError",
        "Vehicle.ADAS.LaneDepartureDetection.IsWarning",
        "Vehicle.ADAS.ObstacleDetection.IsEnabled",
        "Vehicle.ADAS.ObstacleDetection.IsError",
        "Vehicle.ADAS.ObstacleDetection.IsWarning",
        "Vehicle.ADAS.PowerOptimizeLevel",
        "Vehicle.ADAS.SupportedAutonomyLevel",
        "Vehicle.ADAS.TCS.IsEnabled",
        "Vehicle.ADAS.TCS.IsEngaged",
        "Vehicle.ADAS.TCS.IsError",
        "Vehicle.Acceleration.Lateral",
        "Vehicle.Acceleration.Longitudinal",
        "Vehicle.Acceleration.Vertical",
        "Vehicle.AngularVelocity.Pitch",
        "Vehicle.AngularVelocity.Roll",
        "Vehicle.AngularVelocity.Yaw",
        "Vehicle.AverageSpeed",
        "Vehicle.Body.BodyType",
        "Vehicle.Body.Hood.IsOpen",
        "Vehicle.Body.Horn.IsActive",
        "Vehicle.Body.Lights.Backup.IsDefect",
        "Vehicle.Body.Lights.Backup.IsOn",
        "Vehicle.Body.Lights.Beam.High.IsDefect",
        "Vehicle.Body.Lights.Beam.High.IsOn",
        "Vehicle.Body.Lights.Beam.Low.IsDefect",
        "Vehicle.Body.Lights.Beam.Low.IsOn",
        "Vehicle.Body.Lights.Brake.IsActive",
        "Vehicle.Body.Lights.Brake.IsDefect",
        "Vehicle.Body.Lights.DirectionIndicator.Left.IsDefect",
        "Vehicle.Body.Lights.DirectionIndicator.Left.IsSignaling",
        "Vehicle.Body.Lights.DirectionIndicator.Right.IsDefect",
        "Vehicle.Body.Lights.DirectionIndicator.Right.IsSignaling",
        "Vehicle.Body.Lights.Fog.Front.IsDefect",
        "Vehicle.Body.Lights.Fog.Front.IsOn",
        "Vehicle.Body.Lights.Fog.Rear.IsDefect",
        "Vehicle.Body.Lights.Fog.Rear.IsOn",
        "Vehicle.Body.Lights.Hazard.IsDefect",
        "Vehicle.Body.Lights.Hazard.IsSignaling",
        "Vehicle.Body.Lights.IsHighBeamSwitchOn",
        "Vehicle.Body.Lights.LicensePlate.IsDefect",
        "Vehicle.Body.Lights.LicensePlate.IsOn",
        "Vehicle.Body.Lights.LightSwitch",
        "Vehicle.Body.Lights.Parking.IsDefect",
        "Vehicle.Body.Lights.Parking.IsOn",
        "Vehicle.Body.Lights.Running.IsDefect",
        "Vehicle.Body.Lights.Running.IsOn",
        "Vehicle.Body.Mirrors.DriverSide.IsHeatingOn",
        "Vehicle.Body.Mirrors.DriverSide.Pan",
        "Vehicle.Body.Mirrors.DriverSide.Tilt",
        "Vehicle.Body.Mirrors.PassengerSide.IsHeatingOn",
        "Vehicle.Body.Mirrors.PassengerSide.Pan",
        "Vehicle.Body.Mirrors.PassengerSide.Tilt",
        "Vehicle.Body.PowerOptimizeLevel",
        "Vehicle.Body.Raindetection.Intensity",
        "Vehicle.Body.RearMainSpoilerPosition",
        "Vehicle.Body.RefuelPosition",
        "Vehicle.Body.Trunk.Front.IsLightOn",
        "Vehicle.Body.Trunk.Front.IsLocked",
        "Vehicle.Body.Trunk.Front.IsOpen",
        "Vehicle.Body.Trunk.Rear.IsLightOn",
        "Vehicle.Body.Trunk.Rear.IsLocked",
        "Vehicle.Body.Trunk.Rear.IsOpen",
        "Vehicle.Body.Windshield.Front.IsHeatingOn",
        "Vehicle.Body.Windshield.Front.WasherFluid.IsLevelLow",
        "Vehicle.Body.Windshield.Front.WasherFluid.Level",
        "Vehicle.Body.Windshield.Front.Wiping.Intensity",
        "Vehicle.Body.Windshield.Front.Wiping.IsWipersWorn",
        "Vehicle.Body.Windshield.Front.Wiping.Mode",
        "Vehicle.Body.Windshield.Front.Wiping.System.ActualPosition",
        "Vehicle.Body.Windshield.Front.Wiping.System.DriveCurrent",
        "Vehicle.Body.Windshield.Front.Wiping.System.Frequency",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsBlocked",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsEndingWipeCycle",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsOverheated",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsPositionReached",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsWiperError",
        "Vehicle.Body.Windshield.Front.Wiping.System.IsWiping",
        "Vehicle.Body.Windshield.Front.Wiping.System.Mode",
        "Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition",
        "Vehicle.Body.Windshield.Front.Wiping.WiperWear",
        "Vehicle.Body.Windshield.Rear.IsHeatingOn",
        "Vehicle.Body.Windshield.Rear.WasherFluid.IsLevelLow",
        "Vehicle.Body.Windshield.Rear.WasherFluid.Level",
        "Vehicle.Body.Windshield.Rear.Wiping.Intensity",
        "Vehicle.Body.Windshield.Rear.Wiping.IsWipersWorn",
        "Vehicle.Body.Windshield.Rear.Wiping.Mode",
        "Vehicle.Body.Windshield.Rear.Wiping.System.ActualPosition",
        "Vehicle.Body.Windshield.Rear.Wiping.System.DriveCurrent",
        "Vehicle.Body.Windshield.Rear.Wiping.System.Frequency",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsBlocked",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsEndingWipeCycle",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsOverheated",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsPositionReached",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsWiperError",
        "Vehicle.Body.Windshield.Rear.Wiping.System.IsWiping",
        "Vehicle.Body.Windshield.Rear.Wiping.System.Mode",
        "Vehicle.Body.Windshield.Rear.Wiping.System.TargetPosition",
        "Vehicle.Body.Windshield.Rear.Wiping.WiperWear",
        "Vehicle.Cabin.Convertible.Status",
        "Vehicle.Cabin.Door.Row1.DriverSide.IsChildLockActive",
        "Vehicle.Cabin.Door.Row1.DriverSide.IsLocked",
        "Vehicle.Cabin.Door.Row1.DriverSide.IsOpen",
        "Vehicle.Cabin.Door.Row1.DriverSide.Shade.Position",
        "Vehicle.Cabin.Door.Row1.DriverSide.Shade.Switch",
        "Vehicle.Cabin.Door.Row1.DriverSide.Window.IsOpen",
        "Vehicle.Cabin.Door.Row1.DriverSide.Window.Position",
        "Vehicle.Cabin.Door.Row1.DriverSide.Window.Switch",
        "Vehicle.Cabin.Door.Row1.PassengerSide.IsChildLockActive",
        "Vehicle.Cabin.Door.Row1.PassengerSide.IsLocked",
        "Vehicle.Cabin.Door.Row1.PassengerSide.IsOpen",
        "Vehicle.Cabin.Door.Row1.PassengerSide.Shade.Position",
        "Vehicle.Cabin.Door.Row1.PassengerSide.Shade.Switch",
        "Vehicle.Cabin.Door.Row1.PassengerSide.Window.IsOpen",
        "Vehicle.Cabin.Door.Row1.PassengerSide.Window.Position",
        "Vehicle.Cabin.Door.Row1.PassengerSide.Window.Switch",
        "Vehicle.Cabin.Door.Row2.DriverSide.IsChildLockActive",
        "Vehicle.Cabin.Door.Row2.DriverSide.IsLocked",
        "Vehicle.Cabin.Door.Row2.DriverSide.IsOpen",
        "Vehicle.Cabin.Door.Row2.DriverSide.Shade.Position",
        "Vehicle.Cabin.Door.Row2.DriverSide.Shade.Switch",
        "Vehicle.Cabin.Door.Row2.DriverSide.Window.IsOpen",
        "Vehicle.Cabin.Door.Row2.DriverSide.Window.Position",
        "Vehicle.Cabin.Door.Row2.DriverSide.Window.Switch",
        "Vehicle.Cabin.Door.Row2.PassengerSide.IsChildLockActive",
        "Vehicle.Cabin.Door.Row2.PassengerSide.IsLocked",
        "Vehicle.Cabin.Door.Row2.PassengerSide.IsOpen",
        "Vehicle.Cabin.Door.Row2.PassengerSide.Shade.Position",
        "Vehicle.Cabin.Door.Row2.PassengerSide.Shade.Switch",
        "Vehicle.Cabin.Door.Row2.PassengerSide.Window.IsOpen",
        "Vehicle.Cabin.Door.Row2.PassengerSide.Window.Position",
        "Vehicle.Cabin.Door.Row2.PassengerSide.Window.Switch",
        "Vehicle.Cabin.DoorCount",
        "Vehicle.Cabin.DriverPosition",
        "Vehicle.Cabin.HVAC.AmbientAirTemperature",
        "Vehicle.Cabin.HVAC.IsAirConditioningActive",
        "Vehicle.Cabin.HVAC.IsFrontDefrosterActive",
        "Vehicle.Cabin.HVAC.IsRearDefrosterActive",
        "Vehicle.Cabin.HVAC.IsRecirculationActive",
        "Vehicle.Cabin.HVAC.PowerOptimizeLevel",
        "Vehicle.Cabin.HVAC.Station.Row1.Driver.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row1.Driver.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row1.Driver.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row1.Passenger.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row1.Passenger.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row1.Passenger.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row2.Driver.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row2.Driver.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row2.Driver.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row2.Passenger.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row2.Passenger.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row2.Passenger.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row3.Driver.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row3.Driver.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row3.Driver.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row3.Passenger.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row3.Passenger.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row3.Passenger.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row4.Driver.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row4.Driver.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row4.Driver.Temperature",
        "Vehicle.Cabin.HVAC.Station.Row4.Passenger.AirDistribution",
        "Vehicle.Cabin.HVAC.Station.Row4.Passenger.FanSpeed",
        "Vehicle.Cabin.HVAC.Station.Row4.Passenger.Temperature",
        "Vehicle.Cabin.Infotainment.HMI.Brightness",
        "Vehicle.Cabin.Infotainment.HMI.CurrentLanguage",
        "Vehicle.Cabin.Infotainment.HMI.DateFormat",
        "Vehicle.Cabin.Infotainment.HMI.DayNightMode",
        "Vehicle.Cabin.Infotainment.HMI.DisplayOffDuration",
        "Vehicle.Cabin.Infotainment.HMI.DistanceUnit",
        "Vehicle.Cabin.Infotainment.HMI.EVEconomyUnits",
        "Vehicle.Cabin.Infotainment.HMI.FontSize",
        "Vehicle.Cabin.Infotainment.HMI.FuelEconomyUnits",
        "Vehicle.Cabin.Infotainment.HMI.FuelVolumeUnit",
        "Vehicle.Cabin.Infotainment.HMI.IsScreenAlwaysOn",
        "Vehicle.Cabin.Infotainment.HMI.LastActionTime",
        "Vehicle.Cabin.Infotainment.HMI.TemperatureUnit",
        "Vehicle.Cabin.Infotainment.HMI.TimeFormat",
        "Vehicle.Cabin.Infotainment.HMI.TirePressureUnit",
        "Vehicle.Cabin.Infotainment.Media.Action",
        "Vehicle.Cabin.Infotainment.Media.DeclinedURI",
        "Vehicle.Cabin.Infotainment.Media.Played.Album",
        "Vehicle.Cabin.Infotainment.Media.Played.Artist",
        "Vehicle.Cabin.Infotainment.Media.Played.PlaybackRate",
        "Vehicle.Cabin.Infotainment.Media.Played.Source",
        "Vehicle.Cabin.Infotainment.Media.Played.Track",
        "Vehicle.Cabin.Infotainment.Media.Played.URI",
        "Vehicle.Cabin.Infotainment.Media.SelectedURI",
        "Vehicle.Cabin.Infotainment.Media.Volume",
        "Vehicle.Cabin.Infotainment.Navigation.DestinationSet.Latitude",
        "Vehicle.Cabin.Infotainment.Navigation.DestinationSet.Longitude",
        "Vehicle.Cabin.Infotainment.Navigation.GuidanceVoice",
        "Vehicle.Cabin.Infotainment.Navigation.Mute",
        "Vehicle.Cabin.Infotainment.Navigation.Volume",
        "Vehicle.Cabin.Infotainment.PowerOptimizeLevel",
        "Vehicle.Cabin.Infotainment.SmartphoneProjection.Active",
        "Vehicle.Cabin.Infotainment.SmartphoneProjection.Source",
        "Vehicle.Cabin.Infotainment.SmartphoneProjection.SupportedMode",
        "Vehicle.Cabin.IsWindowChildLockEngaged",
        "Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Color",
        "Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.Intensity",
        "Vehicle.Cabin.Light.AmbientLight.Row1.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.AmbientLight.Row1.PassengerSide.Color",
        "Vehicle.Cabin.Light.AmbientLight.Row1.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.AmbientLight.Row1.PassengerSide.IsLightOn",
        "Vehicle.Cabin.Light.AmbientLight.Row2.DriverSide.Color",
        "Vehicle.Cabin.Light.AmbientLight.Row2.DriverSide.Intensity",
        "Vehicle.Cabin.Light.AmbientLight.Row2.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.AmbientLight.Row2.PassengerSide.Color",
        "Vehicle.Cabin.Light.AmbientLight.Row2.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.AmbientLight.Row2.PassengerSide.IsLightOn",
        "Vehicle.Cabin.Light.InteractiveLightBar.Color",
        "Vehicle.Cabin.Light.InteractiveLightBar.Effect",
        "Vehicle.Cabin.Light.InteractiveLightBar.Intensity",
        "Vehicle.Cabin.Light.InteractiveLightBar.IsLightOn",
        "Vehicle.Cabin.Light.IsDomeOn",
        "Vehicle.Cabin.Light.IsGloveBoxOn",
        "Vehicle.Cabin.Light.PerceivedAmbientLight",
        "Vehicle.Cabin.Light.Spotlight.Row1.DriverSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row1.DriverSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row1.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row1.PassengerSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row1.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row1.PassengerSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row2.DriverSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row2.DriverSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row2.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row2.PassengerSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row2.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row2.PassengerSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row3.DriverSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row3.DriverSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row3.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row3.PassengerSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row3.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row3.PassengerSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row4.DriverSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row4.DriverSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row4.DriverSide.IsLightOn",
        "Vehicle.Cabin.Light.Spotlight.Row4.PassengerSide.Color",
        "Vehicle.Cabin.Light.Spotlight.Row4.PassengerSide.Intensity",
        "Vehicle.Cabin.Light.Spotlight.Row4.PassengerSide.IsLightOn",
        "Vehicle.Cabin.PowerOptimizeLevel",
        "Vehicle.Cabin.RearShade.Position",
        "Vehicle.Cabin.RearShade.Switch",
        "Vehicle.Cabin.RearviewMirror.DimmingLevel",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Headrest.Height",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Heating",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Height",
        "Vehicle.Cabin.Seat.Row1.DriverSide.IsBelted",
        "Vehicle.Cabin.Seat.Row1.DriverSide.IsOccupied",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Massage",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Position",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Seating.Length",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.SideBolster.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Backrest.SideBolster.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.DriverSide.Tilt",
        "Vehicle.Cabin.Seat.Row1.Middle.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row1.Middle.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row1.Middle.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row1.Middle.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row1.Middle.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row1.Middle.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row1.Middle.Headrest.Height",
        "Vehicle.Cabin.Seat.Row1.Middle.Heating",
        "Vehicle.Cabin.Seat.Row1.Middle.Height",
        "Vehicle.Cabin.Seat.Row1.Middle.IsBelted",
        "Vehicle.Cabin.Seat.Row1.Middle.IsOccupied",
        "Vehicle.Cabin.Seat.Row1.Middle.Massage",
        "Vehicle.Cabin.Seat.Row1.Middle.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row1.Middle.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row1.Middle.Position",
        "Vehicle.Cabin.Seat.Row1.Middle.Seating.Length",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.SideBolster.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Backrest.SideBolster.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.Middle.Tilt",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Headrest.Height",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Heating",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Height",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.IsBelted",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.IsOccupied",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Massage",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Position",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Seating.Length",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.SideBolster.",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Backrest.SideBolster.",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row1.PassengerSide.Tilt",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Headrest.Height",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Heating",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Height",
        "Vehicle.Cabin.Seat.Row2.DriverSide.IsBelted",
        "Vehicle.Cabin.Seat.Row2.DriverSide.IsOccupied",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Massage",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Position",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Seating.Length",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.SideBolster.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Backrest.SideBolster.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.DriverSide.Tilt",
        "Vehicle.Cabin.Seat.Row2.Middle.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row2.Middle.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row2.Middle.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row2.Middle.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row2.Middle.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row2.Middle.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row2.Middle.Headrest.Height",
        "Vehicle.Cabin.Seat.Row2.Middle.Heating",
        "Vehicle.Cabin.Seat.Row2.Middle.Height",
        "Vehicle.Cabin.Seat.Row2.Middle.IsBelted",
        "Vehicle.Cabin.Seat.Row2.Middle.IsOccupied",
        "Vehicle.Cabin.Seat.Row2.Middle.Massage",
        "Vehicle.Cabin.Seat.Row2.Middle.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row2.Middle.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row2.Middle.Position",
        "Vehicle.Cabin.Seat.Row2.Middle.Seating.Length",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.SideBolster.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Backrest.SideBolster.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.Middle.Tilt",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Airbag.IsDeployed",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Backrest.Lumbar.Height",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Backrest.Lumbar.Support",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Backrest.Recline",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Backrest.SideBolster.Support",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Headrest.Angle",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Headrest.Height",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Heating",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Height",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.IsBelted",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.IsOccupied",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Massage",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Occupant.Identifier.Issuer",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Occupant.Identifier.Subject",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Position",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Seating.Length",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.IsReclineBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.IsReclineForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.Lumbar.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.Lumbar.IsLessSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.Lumbar.IsMoreSupportEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.Lumbar.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.SideBolster.",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Backrest.SideBolster.",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Headrest.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Headrest.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Headrest.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Headrest.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsCoolerEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsDownEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsTiltBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsTiltForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsUpEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.IsWarmerEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Massage.IsDecreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Massage.IsIncreaseEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Seating.IsBackwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Switch.Seating.IsForwardEngaged",
        "Vehicle.Cabin.Seat.Row2.PassengerSide.Tilt",
        "Vehicle.Cabin.SeatPosCount",
        "Vehicle.Cabin.SeatRowCount",
        "Vehicle.Cabin.Sunroof.Position",
        "Vehicle.Cabin.Sunroof.Shade.Position",
        "Vehicle.Cabin.Sunroof.Shade.Switch",
        "Vehicle.Cabin.Sunroof.Switch",
        "Vehicle.CargoVolume",
        "Vehicle.Chassis.Accelerator.PedalPosition",
        "Vehicle.Chassis.Axle.Row1.AxleWidth",
        "Vehicle.Chassis.Axle.Row1.SteeringAngle",
        "Vehicle.Chassis.Axle.Row1.TireAspectRatio",
        "Vehicle.Chassis.Axle.Row1.TireDiameter",
        "Vehicle.Chassis.Axle.Row1.TireWidth",
        "Vehicle.Chassis.Axle.Row1.TrackWidth",
        "Vehicle.Chassis.Axle.Row1.TreadWidth",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.FluidLevel",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.IsBrakesWorn",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.IsFluidLevelLow",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.PadWear",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Speed",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.IsPressureLow",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.Pressure",
        "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.Temperature",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.FluidLevel",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.IsBrakesWorn",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.IsFluidLevelLow",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.PadWear",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Speed",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.IsPressureLow",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.Pressure",
        "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.Temperature",
        "Vehicle.Chassis.Axle.Row1.WheelCount",
        "Vehicle.Chassis.Axle.Row1.WheelDiameter",
        "Vehicle.Chassis.Axle.Row1.WheelWidth",
        "Vehicle.Chassis.Axle.Row2.AxleWidth",
        "Vehicle.Chassis.Axle.Row2.SteeringAngle",
        "Vehicle.Chassis.Axle.Row2.TireAspectRatio",
        "Vehicle.Chassis.Axle.Row2.TireDiameter",
        "Vehicle.Chassis.Axle.Row2.TireWidth",
        "Vehicle.Chassis.Axle.Row2.TrackWidth",
        "Vehicle.Chassis.Axle.Row2.TreadWidth",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Brake.FluidLevel",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Brake.IsBrakesWorn",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Brake.IsFluidLevelLow",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Brake.PadWear",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Speed",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.IsPressureLow",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.Pressure",
        "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.Temperature",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Brake.FluidLevel",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Brake.IsBrakesWorn",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Brake.IsFluidLevelLow",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Brake.PadWear",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Speed",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.IsPressureLow",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.Pressure",
        "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.Temperature",
        "Vehicle.Chassis.Axle.Row2.WheelCount",
        "Vehicle.Chassis.Axle.Row2.WheelDiameter",
        "Vehicle.Chassis.Axle.Row2.WheelWidth",
        "Vehicle.Chassis.AxleCount",
        "Vehicle.Chassis.Brake.IsDriverEmergencyBrakingDetected",
        "Vehicle.Chassis.Brake.PedalPosition",
        "Vehicle.Chassis.ParkingBrake.IsAutoApplyEnabled",
        "Vehicle.Chassis.ParkingBrake.IsEngaged",
        "Vehicle.Chassis.SteeringWheel.Angle",
        "Vehicle.Chassis.SteeringWheel.Extension",
        "Vehicle.Chassis.SteeringWheel.Tilt",
        "Vehicle.Chassis.Wheelbase",
        "Vehicle.Connectivity.IsConnectivityAvailable",
        "Vehicle.CurbWeight",
        "Vehicle.CurrentLocation.Altitude",
        "Vehicle.CurrentLocation.GNSSReceiver.FixType",
        "Vehicle.CurrentLocation.GNSSReceiver.MountingPosition.X",
        "Vehicle.CurrentLocation.GNSSReceiver.MountingPosition.Y",
        "Vehicle.CurrentLocation.GNSSReceiver.MountingPosition.Z",
        "Vehicle.CurrentLocation.Heading",
        "Vehicle.CurrentLocation.HorizontalAccuracy",
        "Vehicle.CurrentLocation.Latitude",
        "Vehicle.CurrentLocation.Longitude",
        "Vehicle.CurrentLocation.Timestamp",
        "Vehicle.CurrentLocation.VerticalAccuracy",
        "Vehicle.CurrentOverallWeight",
        "Vehicle.Driver.AttentiveProbability",
        "Vehicle.Driver.DistractionLevel",
        "Vehicle.Driver.FatigueLevel",
        "Vehicle.Driver.HeartRate",
        "Vehicle.Driver.Identifier.Issuer",
        "Vehicle.Driver.Identifier.Subject",
        "Vehicle.Driver.IsEyesOnRoad",
        "Vehicle.Driver.IsHandsOnWheel",
        "Vehicle.EmissionsCO2",
        "Vehicle.Exterior.AirTemperature",
        "Vehicle.Exterior.Humidity",
        "Vehicle.Exterior.LightIntensity",
        "Vehicle.GrossWeight",
        "Vehicle.Height",
        "Vehicle.IsBrokenDown",
        "Vehicle.IsMoving",
        "Vehicle.Length",
        "Vehicle.LowVoltageBattery.CurrentCurrent",
        "Vehicle.LowVoltageBattery.CurrentVoltage",
        "Vehicle.LowVoltageBattery.NominalCapacity",
        "Vehicle.LowVoltageBattery.NominalVoltage",
        "Vehicle.LowVoltageSystemState",
        "Vehicle.MaxTowBallWeight",
        "Vehicle.MaxTowWeight",
        "Vehicle.OBD.AbsoluteLoad",
        "Vehicle.OBD.AcceleratorPositionD",
        "Vehicle.OBD.AcceleratorPositionE",
        "Vehicle.OBD.AcceleratorPositionF",
        "Vehicle.OBD.AirStatus",
        "Vehicle.OBD.AmbientAirTemperature",
        "Vehicle.OBD.BarometricPressure",
        "Vehicle.OBD.Catalyst.Bank1.Temperature1",
        "Vehicle.OBD.Catalyst.Bank1.Temperature2",
        "Vehicle.OBD.Catalyst.Bank2.Temperature1",
        "Vehicle.OBD.Catalyst.Bank2.Temperature2",
        "Vehicle.OBD.CommandedEGR",
        "Vehicle.OBD.CommandedEVAP",
        "Vehicle.OBD.CommandedEquivalenceRatio",
        "Vehicle.OBD.ControlModuleVoltage",
        "Vehicle.OBD.CoolantTemperature",
        "Vehicle.OBD.DTCList",
        "Vehicle.OBD.DistanceSinceDTCClear",
        "Vehicle.OBD.DistanceWithMIL",
        "Vehicle.OBD.DriveCycleStatus.DTCCount",
        "Vehicle.OBD.DriveCycleStatus.IgnitionType",
        "Vehicle.OBD.DriveCycleStatus.IsMILOn",
        "Vehicle.OBD.EGRError",
        "Vehicle.OBD.EVAPVaporPressure",
        "Vehicle.OBD.EVAPVaporPressureAbsolute",
        "Vehicle.OBD.EVAPVaporPressureAlternate",
        "Vehicle.OBD.EngineLoad",
        "Vehicle.OBD.EngineSpeed",
        "Vehicle.OBD.EthanolPercent",
        "Vehicle.OBD.FreezeDTC",
        "Vehicle.OBD.FuelInjectionTiming",
        "Vehicle.OBD.FuelLevel",
        "Vehicle.OBD.FuelPressure",
        "Vehicle.OBD.FuelRailPressureAbsolute",
        "Vehicle.OBD.FuelRailPressureDirect",
        "Vehicle.OBD.FuelRailPressureVac",
        "Vehicle.OBD.FuelRate",
        "Vehicle.OBD.FuelStatus",
        "Vehicle.OBD.FuelType",
        "Vehicle.OBD.HybridBatteryRemaining",
        "Vehicle.OBD.IntakeTemp",
        "Vehicle.OBD.IsPTOActive",
        "Vehicle.OBD.LongTermFuelTrim1",
        "Vehicle.OBD.LongTermFuelTrim2",
        "Vehicle.OBD.LongTermO2Trim1",
        "Vehicle.OBD.LongTermO2Trim2",
        "Vehicle.OBD.LongTermO2Trim3",
        "Vehicle.OBD.LongTermO2Trim4",
        "Vehicle.OBD.MAF",
        "Vehicle.OBD.MAP",
        "Vehicle.OBD.MaxMAF",
        "Vehicle.OBD.O2.Sensor1.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor1.Voltage",
        "Vehicle.OBD.O2.Sensor2.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor2.Voltage",
        "Vehicle.OBD.O2.Sensor3.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor3.Voltage",
        "Vehicle.OBD.O2.Sensor4.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor4.Voltage",
        "Vehicle.OBD.O2.Sensor5.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor5.Voltage",
        "Vehicle.OBD.O2.Sensor6.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor6.Voltage",
        "Vehicle.OBD.O2.Sensor7.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor7.Voltage",
        "Vehicle.OBD.O2.Sensor8.ShortTermFuelTrim",
        "Vehicle.OBD.O2.Sensor8.Voltage",
        "Vehicle.OBD.O2WR.Sensor1.Current",
        "Vehicle.OBD.O2WR.Sensor1.Lambda",
        "Vehicle.OBD.O2WR.Sensor1.Voltage",
        "Vehicle.OBD.O2WR.Sensor2.Current",
        "Vehicle.OBD.O2WR.Sensor2.Lambda",
        "Vehicle.OBD.O2WR.Sensor2.Voltage",
        "Vehicle.OBD.O2WR.Sensor3.Current",
        "Vehicle.OBD.O2WR.Sensor3.Lambda",
        "Vehicle.OBD.O2WR.Sensor3.Voltage",
        "Vehicle.OBD.O2WR.Sensor4.Current",
        "Vehicle.OBD.O2WR.Sensor4.Lambda",
        "Vehicle.OBD.O2WR.Sensor4.Voltage",
        "Vehicle.OBD.O2WR.Sensor5.Current",
        "Vehicle.OBD.O2WR.Sensor5.Lambda",
        "Vehicle.OBD.O2WR.Sensor5.Voltage",
        "Vehicle.OBD.O2WR.Sensor6.Current",
        "Vehicle.OBD.O2WR.Sensor6.Lambda",
        "Vehicle.OBD.O2WR.Sensor6.Voltage",
        "Vehicle.OBD.O2WR.Sensor7.Current",
        "Vehicle.OBD.O2WR.Sensor7.Lambda",
        "Vehicle.OBD.O2WR.Sensor7.Voltage",
        "Vehicle.OBD.O2WR.Sensor8.Current",
        "Vehicle.OBD.O2WR.Sensor8.Lambda",
        "Vehicle.OBD.O2WR.Sensor8.Voltage",
        "Vehicle.OBD.OBDStandards",
        "Vehicle.OBD.OilTemperature",
        "Vehicle.OBD.OxygenSensorsIn2Banks",
        "Vehicle.OBD.OxygenSensorsIn4Banks",
        "Vehicle.OBD.PidsA",
        "Vehicle.OBD.PidsB",
        "Vehicle.OBD.PidsC",
        "Vehicle.OBD.RelativeAcceleratorPosition",
        "Vehicle.OBD.RelativeThrottlePosition",
        "Vehicle.OBD.RunTime",
        "Vehicle.OBD.RunTimeMIL",
        "Vehicle.OBD.ShortTermFuelTrim1",
        "Vehicle.OBD.ShortTermFuelTrim2",
        "Vehicle.OBD.ShortTermO2Trim1",
        "Vehicle.OBD.ShortTermO2Trim2",
        "Vehicle.OBD.ShortTermO2Trim3",
        "Vehicle.OBD.ShortTermO2Trim4",
        "Vehicle.OBD.Speed",
        "Vehicle.OBD.Status.DTCCount",
        "Vehicle.OBD.Status.IgnitionType",
        "Vehicle.OBD.Status.IsMILOn",
        "Vehicle.OBD.ThrottleActuator",
        "Vehicle.OBD.ThrottlePosition",
        "Vehicle.OBD.ThrottlePositionB",
        "Vehicle.OBD.ThrottlePositionC",
        "Vehicle.OBD.TimeSinceDTCCleared",
        "Vehicle.OBD.TimingAdvance",
        "Vehicle.OBD.WarmupsSinceDTCClear",
        "Vehicle.PowerOptimizeLevel",
        "Vehicle.Powertrain.AccumulatedBrakingEnergy",
        "Vehicle.Powertrain.CombustionEngine.AspirationType",
        "Vehicle.Powertrain.CombustionEngine.Bore",
        "Vehicle.Powertrain.CombustionEngine.CompressionRatio",
        "Vehicle.Powertrain.CombustionEngine.Configuration",
        "Vehicle.Powertrain.CombustionEngine.DieselExhaustFluid.Capacity",
        "Vehicle.Powertrain.CombustionEngine.DieselExhaustFluid.IsLevelLow",
        "Vehicle.Powertrain.CombustionEngine.DieselExhaustFluid.Level",
        "Vehicle.Powertrain.CombustionEngine.DieselExhaustFluid.Range",
        "Vehicle.Powertrain.CombustionEngine.DieselParticulateFilter.DeltaPressure",
        "Vehicle.Powertrain.CombustionEngine.DieselParticulateFilter.InletTemperature",
        "Vehicle.Powertrain.CombustionEngine.DieselParticulateFilter.OutletTemperature",
        "Vehicle.Powertrain.CombustionEngine.Displacement",
        "Vehicle.Powertrain.CombustionEngine.ECT",
        "Vehicle.Powertrain.CombustionEngine.EOP",
        "Vehicle.Powertrain.CombustionEngine.EOT",
        "Vehicle.Powertrain.CombustionEngine.EngineCode",
        "Vehicle.Powertrain.CombustionEngine.EngineCoolantCapacity",
        "Vehicle.Powertrain.CombustionEngine.EngineHours",
        "Vehicle.Powertrain.CombustionEngine.EngineOilCapacity",
        "Vehicle.Powertrain.CombustionEngine.EngineOilLevel",
        "Vehicle.Powertrain.CombustionEngine.IdleHours",
        "Vehicle.Powertrain.CombustionEngine.IsRunning",
        "Vehicle.Powertrain.CombustionEngine.MAF",
        "Vehicle.Powertrain.CombustionEngine.MAP",
        "Vehicle.Powertrain.CombustionEngine.MaxPower",
        "Vehicle.Powertrain.CombustionEngine.MaxTorque",
        "Vehicle.Powertrain.CombustionEngine.NumberOfCylinders",
        "Vehicle.Powertrain.CombustionEngine.NumberOfValvesPerCylinder",
        "Vehicle.Powertrain.CombustionEngine.OilLifeRemaining",
        "Vehicle.Powertrain.CombustionEngine.Power",
        "Vehicle.Powertrain.CombustionEngine.Speed",
        "Vehicle.Powertrain.CombustionEngine.StrokeLength",
        "Vehicle.Powertrain.CombustionEngine.TPS",
        "Vehicle.Powertrain.CombustionEngine.Torque",
        "Vehicle.Powertrain.ElectricMotor.CoolantTemperature",
        "Vehicle.Powertrain.ElectricMotor.EngineCode",
        "Vehicle.Powertrain.ElectricMotor.MaxPower",
        "Vehicle.Powertrain.ElectricMotor.MaxRegenPower",
        "Vehicle.Powertrain.ElectricMotor.MaxRegenTorque",
        "Vehicle.Powertrain.ElectricMotor.MaxTorque",
        "Vehicle.Powertrain.ElectricMotor.Power",
        "Vehicle.Powertrain.ElectricMotor.Speed",
        "Vehicle.Powertrain.ElectricMotor.Temperature",
        "Vehicle.Powertrain.ElectricMotor.Torque",
        "Vehicle.Powertrain.FuelSystem.AbsoluteLevel",
        "Vehicle.Powertrain.FuelSystem.AverageConsumption",
        "Vehicle.Powertrain.FuelSystem.ConsumptionSinceStart",
        "Vehicle.Powertrain.FuelSystem.HybridType",
        "Vehicle.Powertrain.FuelSystem.InstantConsumption",
        "Vehicle.Powertrain.FuelSystem.IsEngineStopStartEnabled",
        "Vehicle.Powertrain.FuelSystem.IsFuelLevelLow",
        "Vehicle.Powertrain.FuelSystem.Range",
        "Vehicle.Powertrain.FuelSystem.RelativeLevel",
        "Vehicle.Powertrain.FuelSystem.SupportedFuel",
        "Vehicle.Powertrain.FuelSystem.SupportedFuelTypes",
        "Vehicle.Powertrain.FuelSystem.TankCapacity",
        "Vehicle.Powertrain.PowerOptimizeLevel",
        "Vehicle.Powertrain.Range",
        "Vehicle.Powertrain.TractionBattery.AccumulatedChargedEnergy",
        "Vehicle.Powertrain.TractionBattery.AccumulatedChargedThroughput",
        "Vehicle.Powertrain.TractionBattery.AccumulatedConsumedEnergy",
        "Vehicle.Powertrain.TractionBattery.AccumulatedConsumedThroughput",
        "Vehicle.Powertrain.TractionBattery.CellVoltage.Max",
        "Vehicle.Powertrain.TractionBattery.CellVoltage.Min",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeCurrent.DC",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeCurrent.Phase1",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeCurrent.Phase2",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeCurrent.Phase3",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeLimit",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargePlugType",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargePortFlap",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeRate",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeVoltage.DC",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeVoltage.Phase1",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeVoltage.Phase2",
        "Vehicle.Powertrain.TractionBattery.Charging.ChargeVoltage.Phase3",
        "Vehicle.Powertrain.TractionBattery.Charging.IsCharging",
        "Vehicle.Powertrain.TractionBattery.Charging.IsChargingCableConnected",
        "Vehicle.Powertrain.TractionBattery.Charging.IsChargingCableLocked",
        "Vehicle.Powertrain.TractionBattery.Charging.IsDischarging",
        "Vehicle.Powertrain.TractionBattery.Charging.MaximumChargingCurrent.DC",
        "Vehicle.Powertrain.TractionBattery.Charging.MaximumChargingCurrent.Phase1",
        "Vehicle.Powertrain.TractionBattery.Charging.MaximumChargingCurrent.Phase2",
        "Vehicle.Powertrain.TractionBattery.Charging.MaximumChargingCurrent.Phase3",
        "Vehicle.Powertrain.TractionBattery.Charging.Mode",
        "Vehicle.Powertrain.TractionBattery.Charging.PowerLoss",
        "Vehicle.Powertrain.TractionBattery.Charging.StartStopCharging",
        "Vehicle.Powertrain.TractionBattery.Charging.Temperature",
        "Vehicle.Powertrain.TractionBattery.Charging.TimeToComplete",
        "Vehicle.Powertrain.TractionBattery.Charging.Timer.Mode",
        "Vehicle.Powertrain.TractionBattery.Charging.Timer.Time",
        "Vehicle.Powertrain.TractionBattery.CurrentCurrent",
        "Vehicle.Powertrain.TractionBattery.CurrentPower",
        "Vehicle.Powertrain.TractionBattery.CurrentVoltage",
        "Vehicle.Powertrain.TractionBattery.DCDC.PowerLoss",
        "Vehicle.Powertrain.TractionBattery.DCDC.Temperature",
        "Vehicle.Powertrain.TractionBattery.GrossCapacity",
        "Vehicle.Powertrain.TractionBattery.Id",
        "Vehicle.Powertrain.TractionBattery.IsGroundConnected",
        "Vehicle.Powertrain.TractionBattery.IsPowerConnected",
        "Vehicle.Powertrain.TractionBattery.MaxVoltage",
        "Vehicle.Powertrain.TractionBattery.NetCapacity",
        "Vehicle.Powertrain.TractionBattery.NominalVoltage",
        "Vehicle.Powertrain.TractionBattery.PowerLoss",
        "Vehicle.Powertrain.TractionBattery.ProductionDate",
        "Vehicle.Powertrain.TractionBattery.Range",
        "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current",
        "Vehicle.Powertrain.TractionBattery.StateOfCharge.CurrentEnergy",
        "Vehicle.Powertrain.TractionBattery.StateOfCharge.Displayed",
        "Vehicle.Powertrain.TractionBattery.StateOfHealth",
        "Vehicle.Powertrain.TractionBattery.Temperature.Average",
        "Vehicle.Powertrain.TractionBattery.Temperature.Max",
        "Vehicle.Powertrain.TractionBattery.Temperature.Min",
        "Vehicle.Powertrain.Transmission.ClutchEngagement",
        "Vehicle.Powertrain.Transmission.ClutchWear",
        "Vehicle.Powertrain.Transmission.CurrentGear",
        "Vehicle.Powertrain.Transmission.DiffLockFrontEngagement",
        "Vehicle.Powertrain.Transmission.DiffLockRearEngagement",
        "Vehicle.Powertrain.Transmission.DriveType",
        "Vehicle.Powertrain.Transmission.GearChangeMode",
        "Vehicle.Powertrain.Transmission.GearCount",
        "Vehicle.Powertrain.Transmission.IsElectricalPowertrainEngaged",
        "Vehicle.Powertrain.Transmission.IsLowRangeEngaged",
        "Vehicle.Powertrain.Transmission.IsParkLockEngaged",
        "Vehicle.Powertrain.Transmission.PerformanceMode",
        "Vehicle.Powertrain.Transmission.SelectedGear",
        "Vehicle.Powertrain.Transmission.Temperature",
        "Vehicle.Powertrain.Transmission.TorqueDistribution",
        "Vehicle.Powertrain.Transmission.TravelledDistance",
        "Vehicle.Powertrain.Transmission.Type",
        "Vehicle.Powertrain.Type",
        "Vehicle.RoofLoad",
        "Vehicle.Service.DistanceToService",
        "Vehicle.Service.IsServiceDue",
        "Vehicle.Service.TimeToService",
        "Vehicle.Speed",
        "Vehicle.StartTime",
        "Vehicle.Trailer.IsConnected",
        "Vehicle.TraveledDistance",
        "Vehicle.TraveledDistanceSinceStart",
        "Vehicle.TripDuration",
        "Vehicle.TripMeterReading",
        "Vehicle.VehicleIdentification.AcrissCode",
        "Vehicle.VehicleIdentification.BodyType",
        "Vehicle.VehicleIdentification.Brand",
        "Vehicle.VehicleIdentification.DateVehicleFirstRegistered",
        "Vehicle.VehicleIdentification.KnownVehicleDamages",
        "Vehicle.VehicleIdentification.MeetsEmissionStandard",
        "Vehicle.VehicleIdentification.Model",
        "Vehicle.VehicleIdentification.OptionalExtras",
        "Vehicle.VehicleIdentification.ProductionDate",
        "Vehicle.VehicleIdentification.PurchaseDate",
        "Vehicle.VehicleIdentification.VIN",
        "Vehicle.VehicleIdentification.VehicleConfiguration",
        "Vehicle.VehicleIdentification.VehicleInteriorColor",
        "Vehicle.VehicleIdentification.VehicleInteriorType",
        "Vehicle.VehicleIdentification.VehicleModelDate",
        "Vehicle.VehicleIdentification.VehicleSeatingCapacity",
        "Vehicle.VehicleIdentification.VehicleSpecialUsage",
        "Vehicle.VehicleIdentification.WMI",
        "Vehicle.VehicleIdentification.Year",
        "Vehicle.VersionVSS.Label",
        "Vehicle.VersionVSS.Major",
        "Vehicle.VersionVSS.Minor",
        "Vehicle.VersionVSS.Patch",
        "Vehicle.Width",
    ];

    struct TestSignals<'a> {
        glob: &'a TestGlob<'a>,
        signals: &'a [&'a str],
    }

    struct TestGlob<'a> {
        raw: &'a str,
        re: regex::Regex,
    }

    impl<'a> TestSignals<'a> {
        fn new(glob: &'a TestGlob, signals: &'a [&str]) -> Self {
            Self { glob, signals }
        }

        fn should_match_signals(&self, signals: &[&str]) -> bool {
            let mut should_have_matched = Vec::new();
            let mut should_not_have_matched = Vec::new();
            for signal in self.signals {
                if signals.contains(signal) {
                    if !self.glob.is_match(signal) {
                        should_have_matched.push(signal);
                    }
                } else if self.glob.is_match(signal) {
                    should_not_have_matched.push(signal);
                }
            }

            for signal in &should_have_matched {
                println!(
                    "glob '{}' should have matched signal '{}' but didn't",
                    self.glob.raw, signal
                );
            }

            for signal in &should_not_have_matched {
                println!(
                    "glob '{}' should not match signal '{}' but did",
                    self.glob.raw, signal
                );
            }

            if !should_not_have_matched.is_empty() || !should_have_matched.is_empty() {
                println!(
                    "glob '{}' (represented by regex: '{}') did not match the expected signals",
                    self.glob.raw,
                    self.glob.re.as_str()
                );
                false
            } else {
                true
            }
        }

        fn should_match_signal(&self, signal: &str) -> bool {
            self.should_match_signals(&[signal])
        }

        fn should_match_no_signals(&self) -> bool {
            self.should_match_signals(&[])
        }
    }

    impl<'a> TestGlob<'a> {
        fn new(glob: &'a str) -> Self {
            Self {
                raw: glob,
                re: to_regex(glob).unwrap(),
            }
        }

        fn with_signals(&self, signals: &'a [&str]) -> TestSignals {
            TestSignals::new(self, signals)
        }

        fn is_match(&self, haystack: &str) -> bool {
            self.re.is_match(haystack)
        }

        fn should_equal_regex_pattern(&self, regex_pattern: &str) -> bool {
            if self.re.as_str() != regex_pattern {
                println!(
                    "glob '{}' should translate to regex pattern '{}': instead got '{}'",
                    self.raw,
                    regex_pattern,
                    self.re.as_str()
                );
                false
            } else {
                true
            }
        }
    }

    fn using_glob(glob: &str) -> TestGlob {
        TestGlob::new(glob)
    }

    #[test]
    fn test_matches_empty_glob() {
        assert!(using_glob("")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(ALL_SIGNALS));
    }

    #[test]
    fn test_matches_only_multi_level_wildcard() {
        assert!(using_glob("**")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(ALL_SIGNALS));
    }

    #[test]
    fn test_matches_only_single_level_wildcard() {
        assert!(using_glob("*")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
    }

    #[test]
    fn test_matches_only_two_single_level_wildcard() {
        assert!(using_glob("*.*")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.AverageSpeed",
                "Vehicle.CargoVolume",
                "Vehicle.CurbWeight",
                "Vehicle.CurrentOverallWeight",
                "Vehicle.EmissionsCO2",
                "Vehicle.GrossWeight",
                "Vehicle.Height",
                "Vehicle.IsBrokenDown",
                "Vehicle.IsMoving",
                "Vehicle.Length",
                "Vehicle.LowVoltageSystemState",
                "Vehicle.MaxTowBallWeight",
                "Vehicle.MaxTowWeight",
                "Vehicle.PowerOptimizeLevel",
                "Vehicle.RoofLoad",
                "Vehicle.Speed",
                "Vehicle.StartTime",
                "Vehicle.TraveledDistance",
                "Vehicle.TraveledDistanceSinceStart",
                "Vehicle.TripDuration",
                "Vehicle.TripMeterReading",
                "Vehicle.Width",
            ]));
    }

    #[test]
    fn test_matches_shared_prefix() {
        assert!(using_glob("Vehicle.TraveledDistance")
            .with_signals(ALL_SIGNALS)
            .should_match_signal("Vehicle.TraveledDistance"));
    }

    #[ignore]
    #[test]
    fn test_regex_single_subpath() {
        assert!(using_glob("Vehicle").should_equal_regex_pattern("^Vehicle(?:\\..+)?$"));
    }

    #[test]
    fn test_matches_trailing_dot() {
        assert!(using_glob("Vehicle.")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals())
    }

    #[test]
    fn test_matches_leading_dot() {
        assert!(using_glob(".Speed")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals())
    }

    #[test]
    fn test_matches_single_subpath() {
        assert!(using_glob("Vehic")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
        assert!(using_glob("Vehicle")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(ALL_SIGNALS));
    }

    #[ignore]
    #[test]
    fn test_regex_matches_multi_subpath() {
        assert!(using_glob("Vehicle.Cabin.Sunroof")
            .should_equal_regex_pattern(r"^Vehicle\.Cabin\.Sunroof(?:\..+)?$",));
    }

    #[test]
    fn test_matches_something() {
        assert!(using_glob("Vehicle.*.Weight")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
    }
    #[test]
    fn test_matches_multi_subpath() {
        using_glob("Vehicle.Cabin.Sunroof")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.Cabin.Sunroof.Position",
                "Vehicle.Cabin.Sunroof.Shade.Position",
                "Vehicle.Cabin.Sunroof.Shade.Switch",
                "Vehicle.Cabin.Sunroof.Switch",
            ]);

        // Make sure partial "last component" doesn't match
        assert!(using_glob("Vehicle.Cabin.Sunroof")
            .with_signals(&["Vehicle.Cabin.SunroofThing"])
            .should_match_no_signals());
    }

    #[test]
    fn test_matches_full_path_of_signal() {
        // Make sure it matches a full signal.
        assert!(using_glob("Vehicle.Cabin.Sunroof.Shade.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signal("Vehicle.Cabin.Sunroof.Shade.Position"));
    }

    #[ignore]
    #[test]
    fn test_regex_wildcard_at_end() {
        assert!(using_glob("Vehicle.Cabin.Sunroof.*")
            .should_equal_regex_pattern(r"^Vehicle\.Cabin\.Sunroof\.[^.]+$"));
    }

    #[test]
    fn test_matches_wildcard_at_end() {
        assert!(using_glob("Vehicle.Cabin.Sunroof.*")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.Cabin.Sunroof.Position",
                "Vehicle.Cabin.Sunroof.Switch",
            ],));
    }

    #[ignore]
    #[test]
    fn test_regex_single_wildcard_in_middle() {
        assert!(using_glob("Vehicle.Cabin.Sunroof.*.Position")
            .should_equal_regex_pattern(r"^Vehicle\.Cabin\.Sunroof\.[^.]+\.Position$"));
    }

    #[test]
    fn test_matches_single_wildcard_in_middle() {
        assert!(using_glob("Vehicle.Cabin.Sunroof.*.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signal("Vehicle.Cabin.Sunroof.Shade.Position"));
    }

    #[test]
    fn test_matches_multiple_single_wildcard_in_middle() {
        assert!(using_glob("Vehicle.Cabin.*.*.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&["Vehicle.Cabin.Sunroof.Shade.Position"]));
        assert!(using_glob("Vehicle.*.Sunroof.*.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&["Vehicle.Cabin.Sunroof.Shade.Position"]));
    }

    #[test]
    fn test_matches_combination_of_multiple_wildcard_and_single_wildcard() {
        assert!(using_glob("**.*.*.*.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.Cabin.Door.Row2.PassengerSide.Shade.Position",
                "Vehicle.Cabin.Seat.Row2.PassengerSide.Position",
                "Vehicle.Cabin.Seat.Row1.Middle.Position",
                "Vehicle.Cabin.Door.Row2.DriverSide.Shade.Position",
                "Vehicle.Cabin.Seat.Row1.PassengerSide.Position",
                "Vehicle.Cabin.Door.Row1.DriverSide.Window.Position",
                "Vehicle.Cabin.Door.Row1.DriverSide.Shade.Position",
                "Vehicle.Cabin.Door.Row1.PassengerSide.Window.Position",
                "Vehicle.Cabin.Door.Row2.DriverSide.Window.Position",
                "Vehicle.Cabin.Door.Row2.PassengerSide.Window.Position",
                "Vehicle.Cabin.Seat.Row2.Middle.Position",
                "Vehicle.Cabin.Seat.Row1.DriverSide.Position",
                "Vehicle.Cabin.Door.Row1.PassengerSide.Shade.Position",
                "Vehicle.Cabin.Seat.Row2.DriverSide.Position",
                "Vehicle.Cabin.Sunroof.Shade.Position",
            ]));
        /*
               It doesn't match for example:
                   "Vehicle.Cabin.RearShade.Position",
                   "Vehicle.Cabin.Sunroof.Position",
        */
    }

    #[test]
    fn test_matches_double_wildcard_and_multiple_single() {
        assert!(using_glob("**.Door.*.*.Shade.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.Cabin.Door.Row1.DriverSide.Shade.Position",
                "Vehicle.Cabin.Door.Row1.PassengerSide.Shade.Position",
                "Vehicle.Cabin.Door.Row2.DriverSide.Shade.Position",
                "Vehicle.Cabin.Door.Row2.PassengerSide.Shade.Position",
            ]));
    }

    #[test]
    fn test_matches_double_wildcard_in_middle() {
        assert!(using_glob("Vehicle.Cabin.Sunroof.**.Position")
            .with_signals(ALL_SIGNALS)
            .should_match_signals(&[
                "Vehicle.Cabin.Sunroof.Position",
                "Vehicle.Cabin.Sunroof.Shade.Position",
            ],));
    }

    #[ignore]
    #[test]
    fn test_regex_double_wildcard_at_beginning() {
        assert!(using_glob("**.Sunroof").should_equal_regex_pattern(r"^.+\.Sunroof$"));
    }

    #[test]
    fn test_matches_double_wildcard_at_beginning() {
        assert!(using_glob("**.Sunroof")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());

        assert!(using_glob("**.Sunroof")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
    }

    #[ignore]
    #[test]
    fn test_regex_single_wildcard_at_the_beginning() {
        assert!(using_glob("*.Sunroof").should_equal_regex_pattern(r"^[^.]+\.Sunroof$"));
    }

    #[test]
    fn test_matches_single_wildcard_at_the_beginning() {
        assert!(using_glob("*.Sunroof")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
    }

    #[ignore]
    #[test]
    fn test_regex_single_non_matching_literal() {
        assert!(using_glob("Sunroof").should_equal_regex_pattern(r"^Sunroof(?:\..+)?$"));
    }

    #[test]
    fn test_matches_single_non_matching_literal() {
        assert!(using_glob("Sunroof")
            .with_signals(ALL_SIGNALS)
            .should_match_no_signals());
    }

    #[test]
    fn test_is_valid_pattern() {
        assert!(is_valid_pattern("String.*"));
        assert!(is_valid_pattern("String.**"));
        assert!(is_valid_pattern("Vehicle.Chassis.Axle.Row2.Wheel.*"));
        assert!(is_valid_pattern("String.String.String.String.*"));
        assert!(is_valid_pattern("String.String.String.String.**"));
        assert!(is_valid_pattern("String.String.String.String"));
        assert!(is_valid_pattern("String.String.String.String.String.**"));
        assert!(is_valid_pattern("String.String.String.*.String"));
        assert!(is_valid_pattern("String.String.String.**.String"));
        assert!(is_valid_pattern(
            "String.String.String.String.String.**.String"
        ));
        assert!(is_valid_pattern(
            "String.String.String.String.*.String.String"
        ));

        assert!(is_valid_pattern("String.*.String.String"));
        assert!(is_valid_pattern("String.String.**.String.String"));
        assert!(is_valid_pattern("String.**.String.String"));
        assert!(is_valid_pattern("**.String.String.String.**"));
        assert!(is_valid_pattern("**.String.String.String.*"));
        assert!(is_valid_pattern("**.String"));
        assert!(is_valid_pattern("*.String.String.String"));
        assert!(is_valid_pattern("*.String"));
        assert!(!is_valid_pattern("String.String.String."));
        assert!(!is_valid_pattern("String.String.String.String.."));
        assert!(!is_valid_pattern("String.*.String.String.."));
        assert!(!is_valid_pattern("*.String.String.String.."));
    }

    #[test]
    fn test_valid_regex_path() {
        assert_eq!(to_regex_string("String.*"), "^String\\.[^.]+$");
        assert_eq!(to_regex_string("String.**"), "^String\\..*$");
        assert_eq!(
            to_regex_string("String.String.String.String.*"),
            "^String\\.String\\.String\\.String\\.[^.]+$"
        );
        assert_eq!(
            to_regex_string("String.String.String.String.**"),
            "^String\\.String\\.String\\.String\\..*$"
        );
        assert_eq!(
            to_regex_string("String.String.String.String"),
            "^String\\.String\\.String\\.String(?:\\..+)?$"
        );
        assert_eq!(
            to_regex_string("String.String.String.String.String.**"),
            "^String\\.String\\.String\\.String\\.String\\..*$"
        );
        assert_eq!(
            to_regex_string("String.String.String.*.String"),
            "^String\\.String\\.String\\.[^.]+\\.String$"
        );
        assert_eq!(
            to_regex_string("String.String.String.**.String"),
            "^String\\.String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String$"
        );
        assert_eq!(
            to_regex_string("String.String.String.String.String.**.String"),
            "^String\\.String\\.String\\.String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String$"
        );
        assert_eq!(
            to_regex_string("String.String.String.String.*.String.String"),
            "^String\\.String\\.String\\.String\\.[^.]+\\.String\\.String$"
        );
        assert_eq!(
            to_regex_string("String.*.String.String"),
            "^String\\.[^.]+\\.String\\.String$"
        );
        assert_eq!(
            to_regex_string("String.String.**.String.String"),
            "^String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String\\.String$"
        );
        assert_eq!(
            to_regex_string("String.**.String.String"),
            "^String[\\.[A-Z][a-zA-Z0-9]*]*\\.String\\.String$"
        );
        assert_eq!(
            to_regex_string("**.String.String.String.**"),
            "^.+\\.String\\.String\\.String\\..*$"
        );
        assert_eq!(
            to_regex_string("**.String.String.String.*"),
            "^.+\\.String\\.String\\.String\\.[^.]+$"
        );
        assert_eq!(to_regex_string("**.String"), "^.+\\.String$");
        assert_eq!(to_regex_string("*.String"), "^[^.]+\\.String$");
        assert_eq!(
            to_regex_string("*.String.String.String"),
            "^[^.]+\\.String\\.String\\.String$"
        );
    }
}
