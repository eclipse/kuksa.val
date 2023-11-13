Feature: Reading and writing values of a VSS Data Entry

  Rule: Access with right permissions succeeds and fails with wrong/no permissions

    Background:
      Given a running Databroker server with authorization true with the following Data Entries registered
        | path                                    | data type | change type | type      |
        | Vehicle.Speed                           | float     | Static      | Attribute |

    Scenario: Write the current value of an unset Data Entry without authenticating fails
      When a client sets the current value of Vehicle.Width of type float to 13.4 with token ""
      Then the current value for Vehicle.Width can not be accessed because we are unauthorized

    Scenario: Read the current value of an unset Data Entry without authenticating fails
      When a client gets the current value of Vehicle.Width with token ""
      Then the current value for Vehicle.Width can not be accessed because we are unauthorized

    Scenario: Write the current value of a Data Entry without right permissions fails
    """
      The token comes from kuksa.val/jwt/read-vehicle-speed.token. Expire date is Thursday, 1 January 2026
    """
      When a client sets the current value of Vehicle.Speed of type float to 13.4 with token eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicmVhZDpWZWhpY2xlLlNwZWVkIn0.QyaKO-vjjzeimAAyUMjlM_jAvhlmYVguzXp76jAnW9UUiWowrXeYTa_ERVzZqxz21txq1QQmee8WtCO978w95irSRugTmlydWNjS6xPGAOCan6TEXBryrmR5FSmm6fPNrgLPhFEfcqFIQm07B286JTU98s-s2yeb6bJRpm7gOzhH5klCLxBPFYNncwdqqaT6aQD31xOcq4evooPOoV5KZFKI9WKkzOclDvto6TCLYIgnxu9Oey_IqyRAkDHybeFB7LR-1XexweRjWKleXGijpNweu6ATsbPakeVIfJ6k3IN-oKNvP6nawtBg7GeSEFYNksUUCVrIg8b0_tZi3c3E114MvdiDe7iWfRd5SDjO1f8nKiqDYy9P-hwejHntsaXLZbWSs_GNbWmi6J6pwgdM4XI3x4vx-0Otsb4zZ0tOmXjIL_tRsKA5dIlSBYEpIZq6jSXgQLN2_Ame0i--gGt4jTg1sYJHqE56BKc74HqkcvrQAZrZcoeqiKkwKcy6ofRpIQ57ghooZLkJ8BLWEV_zJgSffHBX140EEnMg1z8GAhrsbGvJ29TmXGmYyLrAhyTj_L6aNCZ1XEkbK0Yy5pco9sFGRm9nKTeFcNICogPuzbHg4xsGX_SZgUmla8Acw21AAXwuFY60_aFDUlCKZh93Z2SAruz1gfNIL43I0L8-wfw
      Then setting the value for Vehicle.Speed fails with error code 403

    Scenario: Write the current value of a Data Entry with right permissions succeeds
    """
      The token comes from kuksa.val/jwt/provide-vehicle-speed.token. Expire date is Thursday, 1 January 2026
    """
      When a client sets the current value of Vehicle.Speed of type float to 13.4 with token eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZTpWZWhpY2xlLlNwZWVkIn0.WlHkTSverOeprozFHG5Oo14c_Qr0NL9jv3ObAK4S10ddbqFRjWttkY9C0ehLqM6vXNUyI9uimbrM5FSPpw058mWGbOaEc8l1ImjS-DBKkDXyFkSlMoCPuWfhbamfFWTfY-K_K21kTs0hvr-FGRREC1znnZx0TFEi9HQO2YJvsSfJ7-6yo1Wfplvhf3NCa-sC5PrZEEbvYLkTB56C--0waqxkLZGx_SAo_XoRCijJ3s_LnrEbp61kT9CVYmNk017--mA9EEcjpHceOOtj1_UVjHpLKHOxitjpF-7LQNdq2kCY-Y2qv9vf8H6nAFVG8QKAUAaFb0CmYpDIdK8XSLRD7yLd6JnoRswBqmveFCUpmdrMYsSgut1JH4oCn5EnJ-c5UfZ4IRDgc7iBE5cqH9ao7j5PItsE9tYQJDAfygel3sYnIzuAd-DMYyPs1Jj9BzrAWEmI9s0PelA0KAEspmNufn9e-mjeC050e5NhhzJ4Vj_ffbOBzgx1vgLAaoMj5dOb4j3OpNC0XoUgGfR-YbTLi48h6uXEnxsXNGblOlSqTBmy2iZhYpfLBIsdvQTzKf2iYkw_TLo5LE5p9m4aUKFywcyGPMxzVcA8JIJ2g2Xp30RnIAxUlDTXcuYDGYRgKiGJb0rq1yQVl3RCnKaxTVHg8qqHkts_B-cbItlZP8bJA5M
      Then the set operation succeeds without an error

  Rule: Accessing unregistered Data Entries fails

    Background:
      Given a running Databroker server with authorization false

    Scenario: Setting the current value of an unregistered Data Entry fails
      When a client sets the current value of No.Such.Path of type float to 13.4 with token ""
      Then setting the value for No.Such.Path fails with error code 404

    Scenario: Reading the current value of an unregistered Data Entry fails
      When a client gets the current value of No.Such.Path with token ""
      Then the current value is not found

    Scenario: Setting the target value of an unregistered Data Entry fails
      When a client sets the target value of No.Such.Path of type float to 13.4 with token ""
      Then setting the value for No.Such.Path fails with error code 404

    Scenario: Reading the target value of an unregistered Data Entry fails
      When a client gets the target value of No.Such.Path with token ""
      Then the target value is not found

  Rule: Target values can only be set on Actuators

    Background:
      Given a running Databroker server with authorization false with the following Data Entries registered
        | path                                    | data type | change type | type      |
        | Vehicle.Powertrain.Range                | uint32    | Continuous  | Sensor    |
        | Vehicle.Width                           | uint16    | Static      | Attribute |

    Scenario: Setting the target value of an Attribute fails
      When a client sets the target value of Vehicle.Width of type uint16 to 13 with token ""
      Then the operation fails with status code 3

    Scenario: Setting the target value of a Sensor fails
      When a client sets the target value of Vehicle.Powertrain.Range of type uint32 to 13455 with token ""
      Then the operation fails with status code 3

  Rule: Accessing registered Data Entries works

    Background:
      Given a running Databroker server with authorization false with the following Data Entries registered
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
      When a client gets the current value of Vehicle.Width with token ""
      Then the current value for Vehicle.Width is not specified

    Scenario Outline: Reading the current value works
      Given a Data Entry <path> of type <type> having current value <value>
      When a client gets the current value of <path> with token ""
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
      When a client sets the current value of Vehicle.Speed of type <type> to <value> with token ""
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
      When a client gets the target value of Vehicle.Cabin.Sunroof.Position with token ""
      Then the target value for Vehicle.Cabin.Sunroof.Position is not specified

    Scenario Outline: Reading the target value of Actuators works
      Given a Data Entry <path> of type <type> having target value <value>
      When a client gets the target value of <path> with token ""
      Then the target value for <path> is <value> having type <type>

      Examples:
        | path                                   | type   |        value      |
        | Vehicle.Cabin.Lights.AmbientLight      | uint8  |               255 |
        | Vehicle.Cabin.Sunroof.Position         | int8   |              -128 |

    Scenario Outline: Setting an Actuator's target value using a wrong type fails
      When a client sets the target value of Vehicle.Cabin.Sunroof.Position of type <type> to <value> with token ""
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
