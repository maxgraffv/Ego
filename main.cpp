#include "Core.h"
#include "PQBot.h"


/**
 * Main
 */
int main(int argc, char** argv)
{
    PQBot pqbot;
    EgoCore ego(pqbot);

    ego.start();

    return 0;
}