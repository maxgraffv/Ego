#ifndef EGO_CORE_H
#define EGO_CORE_H

#include "APersonality.h"
#include "DefaultPersonality.h"
#include "PRealsenseViewer.h"

class EgoCore
{
    private:
        APersonality& _personality;


    public:
        EgoCore(APersonality &p);
        ~EgoCore();

        void setPersonality(APersonality &personality);
        void start();



};




#endif