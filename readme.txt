This is a very basic C++ library for Arduino, Implementing SEMP "Simple Energy Management Protocol" from SMA

Installation
--------------------------------------------------------------------------------

To install this library, just place this entire folder as a subfolder in your
Arduino/lib/targets/libraries folder.

When installed, this library should look like:

Arduino/lib/targets/libraries/uSEMP              (this library's folder)
Arduino/lib/targets/libraries/uSEMP/uSEMP.cpp     (the library implementation file)
Arduino/lib/targets/libraries/uSEMP/uSEMP.h       (the library description file)
Arduino/lib/targets/libraries/uSEMP/keywords.txt (the syntax coloring file)
Arduino/lib/targets/libraries/uSEMP/examples     (the examples in the "open" menu)
Arduino/lib/targets/libraries/uSEMP/readme.txt   (this file)

Building
--------------------------------------------------------------------------------

After this library is installed, you just have to start the Arduino application.
You may see a few warning messages as it's built.

To use this library in a sketch, go to the Sketch | Import Library menu and
select uSEMP.  This will add a corresponding line to the top of your sketch:
#include <uSEMP.h>

To stop using this library, delete that line from your sketch.

Geeky information:
After a successful build of this library, a new file named "uSEMP.o" will appear
in "Arduino/lib/targets/libraries/uSEMP". This file is the built/compiled library
code.

If you choose to modify the code for this library (i.e. "uSEMP.cpp" or "uSEMP.h"),
then you must first 'unbuild' this library by deleting the "uSEMP.o" file. The
new "uSEMP.o" with your code will appear after the next press of "verify"

