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
#include "Base/Builder.h"
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
        double  latency;
        long    receivedPackets;
        long    sentPackets;
        double  throughput;
        double  reliability;
        double  per;
    };

    listOfCriteria listOfCriteria80211;
    listOfCriteria listOfCriteria80215;



  // throughput
  simtime_t startTime;    // start time
  unsigned int batchSize;    // number of packets in a batch
  simtime_t maxInterval;    // max length of measurement interval (measurement ends
  // if either batchSize or maxInterval is reached, whichever
  // is reached first)
  // global statistics
  unsigned long numPackets;
  unsigned long numBits;

  // current measurement interval
  simtime_t intvlStartTime;
  simtime_t intvlLastPkTime;
  unsigned long intvlNumPackets;
  unsigned long intvlNumBits;
  std::string allPathsCriteriaValues;

  protected:

    virtual void initialize();
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, double value, cObject *details);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details);


    // 802.11
    Ieee80211Radio* mRadio80211 = nullptr;
    inet::ieee80211::Ieee80211Mac* mLinkLayer80211=nullptr;

    // 802.15
    CSMA* mLinkLayerNarrowBand=nullptr;
    FlatRadioBase* mRadioNarrowBand=nullptr;

    // Lte

    LtePhyVUeMode4* mRadioLte=nullptr;

    void computeThroughput(simtime_t now, unsigned long bits, double& throughput);
    void beginNewInterval(simtime_t now, double& throughput);
    void recordStats(simsignal_t signal,cObject* msg, listOfCriteria& l);

    std::vector<double> convertStatsToVector(double cri_alter0, double cri_alter1);

};

#endif
