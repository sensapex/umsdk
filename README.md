# umsdk
SDK for Sensapex uMx devices (Beta).

Generate doxygen documentation and examine doc/html/index.html for details.
There is a sample application in src/sample/ .

Precompiled windows binaries can be found in
http://dist.sensapex.com/misc/um-sdk/latest/

and precompiled Doxygen documentation in
http://dist.sensapex.com/misc/um-sdk/latest/doc/

There are matlab and labview example scripts for the older ump-sdk SDK,
but not - yet - for this new one.

umsdk is further develop of the older ump-sdk, differencies are:

- function name changed from ump_foo_bar to um_foo_bar if function
is usable for multiple type of devices i.e. not only manipulators (uMp).

- number of alternate functions reduced. The simplified API supported
by the old ump-sdk has been removed. There is now dev_id argument for all functions
except for function not related to certain uMx device e.g. um_get_version for obtaining SDK version.

- support for microscope stage (uMs) and pressure controller uMV).

