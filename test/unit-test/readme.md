# W3C-VISS Unit Test

### Requires Boost version > 1.59

Copy the vss signal tree json manually from [here](https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json) to the build directory once you have build the unit test binary using the following instaructions.

Enable set(UNIT_TEST ON) in CMakeLists.txt under w3v-visserver-api folder.

```

mkdir build

cd build

cmake .. -DBUILD_UNIT_TEST=ON

make

```
you can execute using

```
./w3c-unit-test
```

Obviously, there should be no errors :)
