# vlsi-cad
CAD algorithms for circuit layouts

### Source codes:

src/cse788_layout.c     - Code transform netlist output into magic file

src/cse788_netlist.c    - Implementation of optimzal netlist solver

src/cse788_gordian.c    - Implementation of gordian placement algorithm

src/cse788_floorplan.c  - Implementation of simulated annealing based floorplan algorithm.

src/cse788_display.c    - Graphic interface for the annealing and gordian.

test/hw4.c              - main function calling the modules

src/plus/*              - Helper functions used with the implementation.

### Binary:

The executable for Windows is located in bin/debug/hw4.out, the dll's should be placed together with the executable.

Usage: hw4.out <FUNCTION> <OUTPUT>

Example: hw4.out "~((A*B)*C)" "layout.mag"

###Build:

Environment: mingw/msys or any linux/unix distribution with GCC and GNU make. GSL1.1 (GNU Scientific Library) and SDL2 (Simple Direct media Layer) are required.

Compilation: "make" in the project folder

Run: "make run" to run with above example configuration.
