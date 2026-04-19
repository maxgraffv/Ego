#include "Core.h"


EgoCore::EgoCore( APersonality &p ) : _personality(p)
{

}

EgoCore::~EgoCore()
{

}

void EgoCore::start()
{
    _personality.run();
}


void EgoCore::stop()
{
    _personality.stop();
}

void EgoCore::setPersonality(APersonality& personality)
{
    this->_personality = personality;
}