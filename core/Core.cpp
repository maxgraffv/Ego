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


void EgoCore::setPersonality(APersonality& personality)
{
    this->_personality = personality;
}