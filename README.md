# umsdk

SDK for Sensapex uMx devices.

* uMp - Micro manipulator
* uMs - Microscope motorization
* uMc - Pressure control
* uMa - Amplifier

### Build and Installation

#### Building the sdk

Build umsdk with example projects.

``` bash
cmake -B ./build -DCMAKE_BUILD_TYPE=release
cmake --build ./build --config=release
```
Example applications and libum can be found from `./build/bin` directory.
* `./build/bin/shared` - runtime loadable version of um-sdk library
* `./build/bin/static` - statically linked version of um-sdk library
* `./build/bin` - executables

Run 'sample'-application
``` bash
./build/bin/sample --help
```

Run 'cppsample'-application
``` bash
./build/bin/cppsample --help
```

### Install

Install umsdk to system directories

``` bash
cmake --install ./build/src --config=release
```

### Testing

Build the application with tests included
``` bash
cmake -B ./build -DBUILD_TESTS=ON
cmake --build ./build --config=release
```

Run 'libum_test'-application with basic test set
``` bash
./build/bin/libum_test --gtest_filter="LibumTestBasicC*"
```

Please see the documentation and instructions in test/README.md

### Documentation

Generate doxygen documentation and examine doc/html/index.html for details.

Precompiled Doxygen documentation can be found http://dist.sensapex.com/misc/um-sdk/latest/doc/

### Changelog

#### Version 1.602

- Add device discovery over multiple LinkLocal networks
- Sample application: high frequency scan steps with um_goto_position_ext()
- bugfix: LibUm::takeStep() incorrectly used integer arguments for position

#### Version 1.601

- Enable use of parallel SDK instances on Linux

#### Version 1.600

- Configurable axis drive order
- uMc-INJ pressure range -100kPa..500kPa
- Asynchronous operations and notifications
- Sample application: oscillate using um_take_step()

#### Version 1.504

- Initial support for uMa devices

#### Version 1.400

- New API to read ump handedness configuration
- Bug fixes
- Maintenance improvements

#### Older versions

umsdk is new major development branch that is based on the legacy ump-sdk. Main differences are:

- function names are changed from `ump_foo_bar` to `um_foo_bar` if function is usable for multiple
  type of Sensapex devices i.e. not only manipulators (uMp).

- positions are floating point numbers in µm instead of integers in nm.

- major efforts have been done to simplify and harmonize the SDK, which includes removing
  alternative function variants and simplified API version. There is now dev_id argument for all
  functions except for the functions not related to certain uMx device
  (e.g. um_get_version for obtaining SDK version).

- includes support for uMs microscope motorization products and uMc automated pressure controller
  products.

### TODO

There are matlab and labview example scripts for the legacy ump-sdk SDK, but not - yet - for this
new one (some incompatibilities exist).
