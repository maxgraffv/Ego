#include "Core.h"
#include "PRealsenseViewer.h"
#include "APersonality.h"


/**
 * Main
 */
int main(int argc, char** argv)
{

    PRealsenseViewer prv;
    EgoCore ego(prv);


    ego.setPersonality(prv);
    ego.start();

    return 0;
}