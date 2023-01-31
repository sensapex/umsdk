# umsdk

SDK for Sensapex uMx devices.

* uMp - Micromanipulators
* uMs - Microscope motorization
* uMc - Pressure control

### Installation

#### Linux and MacOS

Build and install umsdk with example projects.

```
cd scripts
./build.sh
```

Install umlib in your system

```
cd scripts
sudo ./install.sh
```

Example applications and libum can be found from build directory.

Eg. Run 'sample'-application

```
cd build/sample
./sample

```

#### Windows

Precompiled windows binaries can be found in
http://dist.sensapex.com/misc/um-sdk/latest/

### Documentation

Generate doxygen documentation and examine doc/html/index.html for details. There is a sample
application in examples/sample/ . and precompiled Doxygen documentation in
http://dist.sensapex.com/misc/um-sdk/latest/doc/ .

### Changelog

umsdk is new major development branch that is based on the legacy ump-sdk. Main differences are:

- Support form CMake

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
