#include "Core.h"
#include "PRealsenseViewer.h"
#include "PMicReader.h"
#include "APersonality.h"


/**
 * Main
 */
int main(int argc, char** argv)
{

    PRealsenseViewer prv;
    PMicReader pmr;
    EgoCore ego(pmr);


    // ego.setPersonality();
    ego.start();

    return 0;
}