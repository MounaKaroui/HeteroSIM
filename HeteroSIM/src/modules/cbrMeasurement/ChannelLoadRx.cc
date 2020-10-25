#include "../cbrMeasurement/ChannelLoadRx.h"

#include <cmath>

using namespace omnetpp;


const simsignal_t ChannelLoadRx::ChannelLoadSignal = cComponent::registerSignal("ChannelLoad");

Define_Module(ChannelLoadRx)

ChannelLoadRx::ChannelLoadRx()
{
}

ChannelLoadRx::~ChannelLoadRx()
{

}

void ChannelLoadRx::initialize(int stage)
{
    Rx::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        mCbrWithTx = par("cbrWithTx");
    }
}


void ChannelLoadRx::recomputeMediumFree()
{
    Rx::recomputeMediumFree();

    using ReceptionState = inet::physicallayer::IRadio::ReceptionState;
    if (mCbrWithTx) {
        mChannelLoadSampler.busy(!mediumFree);
    } else {
        mChannelLoadSampler.busy(!mediumFree && receptionState > ReceptionState::RECEPTION_STATE_IDLE);
    }
}
