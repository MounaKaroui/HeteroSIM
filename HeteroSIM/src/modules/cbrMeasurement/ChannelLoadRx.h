#ifndef ARTERY_CHANNELLOADRX_H_C0YJMQTS
#define ARTERY_CHANNELLOADRX_H_C0YJMQTS

#include "../cbrMeasurement/util/ChannelLoadAccess.h"
#include "inet/linklayer/ieee80211/mac/Rx.h"

class ChannelLoadRx : public inet::ieee80211::Rx, public ChannelLoadAccess
{
public:
    ChannelLoadRx();
    ~ChannelLoadRx();

    static const omnetpp::simsignal_t ChannelLoadSignal;

protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage*) override;
    void recomputeMediumFree() override;

private:
    omnetpp::simtime_t mChannelReportInterval;
    omnetpp::cMessage* mChannelReportTrigger;
    artery::ChannelLoadSampler mAsyncChannelLoadSampler;
    bool mCbrWithTx = false;
};

#endif /* ARTERY_CHANNELLOADRX_H_C0YJMQTS */

