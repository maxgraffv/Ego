#include "Core.h"
#include "PQBot.h"

#include <csignal>
#include <cstdlib>
#include <iostream>

static EgoCore* g_ego = nullptr;

static void onSignal(int sig)
{
    std::cout << "\n[main] Sygnał " << sig << " — zatrzymuję...\n";
    if (g_ego) g_ego->stop();
    std::exit(0);
}

int main(int argc, char** argv)
{
    PQBot pqbot;
    EgoCore ego(pqbot);
    g_ego = &ego;

    std::signal(SIGTERM, onSignal);
    std::signal(SIGINT,  onSignal);

    ego.start();

    return 0;
}
