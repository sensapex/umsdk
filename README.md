# umsdk
SDK for Sensapex uMx devices (Beta).

umsdk is new major development branch that is based on the legacy ump-sdk. Main differences are:

- function names are changed from ump_foo_bar to um_foo_bar if function
is usable for multiple type of Sensapex devices i.e. not only manipulators (uMp).

- major efforts have been done to simplify and harmonize the SDK, which includes removing
revoming alternative variants of some functions. The simplified API supported by the old ump-sdk
has been also removed, which has not been essential. There is now dev_id argument for all
functions except for functions not related to certain uMx device e.g. um_get_version for obtaining SDK version.

- includes support for uMs microscope motorization products and uMc automated pressure controller products.

Generate doxygen documentation and examine doc/html/index.html for details.
There is a sample application in src/sample/ .

Precompiled windows binaries can be found in
http://dist.sensapex.com/misc/um-sdk/latest/

and precompiled Doxygen documentation in
http://dist.sensapex.com/misc/um-sdk/latest/doc/

There are matlab and labview example scripts for the legacy ump-sdk SDK,
but not - yet - for this new one (some incompatibilities exist).
