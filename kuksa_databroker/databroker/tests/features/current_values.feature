Feature: Writing and reading the current value of a VSS Data Entry

  Background:
    Given a running Databroker server

  Rule: Accessing unregistered Data Entries fails
 
    Scenario: Setting the current value of an unregistered Data Entry fails
      When a client sets the current value of No.Such.Path of type float to 13.4
      Then setting the value for No.Such.Path fails with error code 404

    Scenario: Reading the current value of an unregistered Data Entry fails
      When a client gets the current value of No.Such.Path
      Then the current value is not found

  Rule: Accessing registered Data Entries works

    Scenario: Read the current value of an unset Data Entry works
      When a client gets the current value of Vehicle.Width
      Then the current value for Vehicle.Width is not specified

    Scenario Outline: Reading the current value works
      Given a Data Entry <path> of type <type> having value <value>
      When a client gets the current value of <path>
      Then the current value for <path> is <value> having type <type>
      
      Examples:
        | path                                   | type   |        value      |
        | Vehicle.Cabin.Sunroof.Position         | int8   |              -128 |
        | Vehicle.Powertrain.ElectricMotor.Power | int16  |            -32768 |
        | Vehicle.Powertrain.ElectricMotor.Speed | int32  |       -2147483648 |
        | Vehicle.Cabin.Lights.AmbientLight      | uint8  |               255 |
        | Vehicle.Width                          | uint16 |             65535 |
        | Vehicle.Powertrain.Range               | uint32 |        4294967295 |
        | Vehicle.TraveledDistanceHighRes        | uint64 | 23425462462563924 |
        | Vehicle.CurrentLocation.Longitude      | double |        145.023544 |
        | Vehicle.Speed                          | float  |              45.5 |
        | Vehicle.IsMoving                       | bool   |              true |

    Scenario Outline: Setting current value of wrong type fails
      When a client sets the current value of <path> of type <type> to <value>
      Then setting the value for <path> fails with error code 400

      Examples:
        | path                                   | type   |        value      |
        | Vehicle.Cabin.Sunroof.Position         | bool   |              true |
        | Vehicle.Powertrain.ElectricMotor.Power | uint8  |                15 |
        | Vehicle.Powertrain.ElectricMotor.Speed | uint16 |             13648 |
        | Vehicle.Cabin.Lights.AmbientLight      | int32  |          -2552565 |
        | Vehicle.Width                          | int8   |               -35 |
        | Vehicle.Powertrain.Range               | int16  |             -7295 |
        | Vehicle.TraveledDistanceHighRes        | float  |           -6.3924 |
        | Vehicle.CurrentLocation.Longitude      | uint16 |             14502 |
        | Vehicle.Speed                          | bool   |             false |
        | Vehicle.IsMoving                       | uint8  |                 4 |
