//
// main: start DOOM and run the game loop forever.
//
// The platform (DG_* functions, see doomgeneric.h) is not defined here; the
// host provides it. A host that cannot block, such as a web page, can skip
// main() and call doomgeneric_Create() once and doomgeneric_Tick() per
// frame itself.
//

#include "doomgeneric.h"

int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    for (;;)
    {
        doomgeneric_Tick();
    }

    return 0;
}
