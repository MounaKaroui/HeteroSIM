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
        mChannelReportInterval = simtime_t(par("asyncChannelReportInterval").doubleValue());
        mChannelReportTrigger = new cMessage("report CL");

        if (par("asyncChannelReport").boolValue()) {

            scheduleAt(simTime() + mChannelReportInterval,
                    mChannelReportTrigger);
        } else {
            double cycle = simTime() / mChannelReportInterval;
            scheduleAt((1.0 + std::ceil(cycle)) * mChannelReportInterval,
                    mChannelReportTrigger);
        }

        omnetpp::createWatch("channelLoadSampler", mChannelLoadSampler);
        omnetpp::createWatch("asyncChannelLoadSampler", mAsyncChannelLoadSampler);

    }
}

void ChannelLoadRx::handleMessage(cMessage* msg)
{
    if (msg == mChannelReportTrigger) {
        emit(ChannelLoadSignal, mAsyncChannelLoadSampler.cbr());
        scheduleAt(simTime() + mChannelReportInterval, mChannelReportTrigger);
    } else {
        Rx::handleMessage(msg);
    }
}


void ChannelLoadRx::recomputeMediumFree()
{
    Rx::recomputeMediumFree();

    using ReceptionState = inet::physicallayer::IRadio::ReceptionState;
    if (mCbrWithTx) {
        mChannelLoadSampler.busy(!mediumFree);
        mAsyncChannelLoadSampler.busy(!mediumFree);
    } else {
        mChannelLoadSampler.busy(!mediumFree && receptionState > ReceptionState::RECEPTION_STATE_IDLE);
        mAsyncChannelLoadSampler.busy(!mediumFree && receptionState > ReceptionState::RECEPTION_STATE_IDLE);
    }
}
