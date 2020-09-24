//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __HETEROSIM_COLLECTSTATS_H_
#define __HETEROSIM_COLLECTSTATS_H_
#include "inet/common/LayeredProtocolBase.h"
#include "../../Modules/messages/HeterogeneousMessage_m.h"
#include "inet/common/misc/ThruputMeter.h"
#include "inet/physicallayer/common/packetlevel/Radio.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include <inet/linklayer/ieee80211/mac/Ieee80211Mac.h>
#include "Base/mcda/MCDM.h"
#include "inet/linklayer/csma/CSMA.h"

#include <omnetpp.h>

#include "../../Base/Utilities.h"
#include "stack/phy/layer/LtePhyVUeMode4.h"


using namespace omnetpp;
using namespace inet;
using namespace inet::physicallayer;
/**
 * TODO - Generated class
 */
class CollectStats : public cListener, public cSimpleModule
{
public:

    struct listOfCriteria{
        std::vector<double>  latency;
        std::vector<double>     receivedPackets;
        std::vector<double>    sentPackets;
        std::vector<double>   throughput;
        std::vector<double>  reliability;
    };

    double sent=0;
    double rcvd=0;

    listOfCriteria listOfCriteria80211;
    listOfCriteria listOfCriteria80215;
    listOfCriteria listOfCriteriaLte;
    std::string allPathsCriteriaValues;
    double dtlMin;

    // throughput calculation related variables

    simtime_t startTime=0;    // start time
    unsigned int batchSize=1;    // number of packets in a batch
    simtime_t maxInterval=1;    // max length of measurement interval
    //(measurement ends if either batchSize or maxInterval is reached, whichever is reached first)
    unsigned long numPackets=0;
    unsigned long numBits=0;
    // current measurement interval
    simtime_t intvlStartTime=0;
    simtime_t intvlLastPkTime=0;
    unsigned long intvlNumPackets=0;
    unsigned long intvlNumBits=0;

    simsignal_t delay80211;
    simsignal_t delay80215;
    simsignal_t delayLte;
    simsignal_t criteriaListSignal;

protected:

    //virtual void handleMessage(cMessage *msg);

    virtual void initialize();
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details);
    void computeThroughput(simtime_t now, unsigned long bits, double& throughput);
    void recordStats(simsignal_t comingSignal,simsignal_t signalSent, simsignal_t signalRcv,cObject* msg, listOfCriteria& l);
    double prepareNetAttributes();
    std::vector<double> prepareData(std::vector<double> v1, std::vector<double> v2, std::vector<double> v3);
    double calculateCofficientOfVariation(std::vector<double> v);
    double getBeta(double n);
    double updateDTL(double x);
    void virtual finish();


};

#endif
