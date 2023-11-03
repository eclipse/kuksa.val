Feature: Reading and writing values of a VSS Data Entry

  Rule: Accessing unregistered Data Entries fails

    Background:
      Given a running Databroker server

    Scenario: Setting the current value of an unregistered Data Entry fails
      When a client sets the current value of No.Such.Path of type float to 13.4
      Then setting the value for No.Such.Path fails with error code 404

    Scenario: Reading the current value of an unregistered Data Entry fails
      When a client gets the current value of No.Such.Path
      Then the current value is not found

    Scenario: Setting the target value of an unregistered Data Entry fails
      When a client sets the target value of No.Such.Path of type float to 13.4
      Then setting the value for No.Such.Path fails with error code 404

    Scenario: Reading the target value of an unregistered Data Entry fails
      When a client gets the target value of No.Such.Path
      Then the target value is not found

  Rule: Target values can only be set on Actuators

    Background:
      Given a running Databroker server with the following Data Entries registered
        | path                                    | data type | change type | type      |
        | Vehicle.Powertrain.Range                | uint32    | Continuous  | Sensor    |
        | Vehicle.Width                           | uint16    | Static      | Attribute |

    Scenario: Setting the target value of an Attribute fails
      When a client sets the target value of Vehicle.Width of type uint16 to 13
      Then the operation fails with status code 3

    Scenario: Setting the target value of a Sensor fails
      When a client sets the target value of Vehicle.Powertrain.Range of type uint32 to 13455
      Then the operation fails with status code 3

  Rule: Accessing registered Data Entries works

    Background:
      Given a running Databroker server with the following Data Entries registered
        | path                                    | data type | change type | type      |
        | Vehicle.Cabin.Lights.AmbientLight       | uint8     | OnChange    | Actuator  |
        | Vehicle.Cabin.Sunroof.Position          | int8      | OnChange    | Actuator  |
        | Vehicle.CurrentLocation.Longitude       | double    | Continuous  | Sensor    |
        | Vehicle.IsMoving                        | bool      | OnChange    | Sensor    |
        | Vehicle.Powertrain.ElectricMotor.Power  | int16     | Continuous  | Sensor    |
        | Vehicle.Powertrain.ElectricMotor.Speed  | int32     | Continuous  | Sensor    |
        | Vehicle.Powertrain.Range                | uint32    | Continuous  | Sensor    |
        | Vehicle.Speed                           | float     | Continuous  | Sensor    |
        | Vehicle.TraveledDistanceHighRes         | uint64    | Continuous  | Sensor    |
        | Vehicle.Width                           | uint16    | Static      | Attribute |

    Scenario: Reading the current value of an unset Data Entry yields an empty result
      When a client gets the current value of Vehicle.Width
      Then the current value for Vehicle.Width is not specified

    Scenario Outline: Reading the current value works
      Given a Data Entry <path> of type <type> having current value <value>
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

    Scenario Outline: Setting a Data Entry's current value using a wrong type fails
      When a client sets the current value of Vehicle.Speed of type <type> to <value>
      Then setting the value for Vehicle.Speed fails with error code 400

      Examples:
        | type   |        value      |
        | bool   |              true |
        | double |        145.023544 |
        | uint8  |                15 |
        | uint16 |             13648 |
        | uint32 |        4294967295 |
        | uint64 | 24234543543535354 |
        | int8   |               -35 |
        | int16  |             -7295 |
        | int32  |          -2552565 |
        | int64  |  -255256525864925 |

    Scenario: Reading the target value of an unset Data Entry yields an empty result
      When a client gets the target value of Vehicle.Cabin.Sunroof.Position
      Then the target value for Vehicle.Cabin.Sunroof.Position is not specified

    Scenario Outline: Reading the target value of Actuators works
      Given a Data Entry <path> of type <type> having target value <value>
      When a client gets the target value of <path>
      Then the target value for <path> is <value> having type <type>

      Examples:
        | path                                   | type   |        value      |
        | Vehicle.Cabin.Lights.AmbientLight      | uint8  |               255 |
        | Vehicle.Cabin.Sunroof.Position         | int8   |              -128 |

    Scenario Outline: Setting an Actuator's target value using a wrong type fails
      When a client sets the target value of Vehicle.Cabin.Sunroof.Position of type <type> to <value>
      Then setting the value for Vehicle.Cabin.Sunroof.Position fails with error code 400

      Examples:
        | type   |        value      |
        | bool   |              true |
        | double | -285625145.023544 |
        | float  |        145.023544 |
        | uint8  |                15 |
        | uint16 |             13648 |
        | uint32 |        4294967295 |
        | uint64 | 24234543543535354 |
        | int16  |             -7295 |
        | int32  |          -2552565 |
        | int64  |  -255256525864925 |
