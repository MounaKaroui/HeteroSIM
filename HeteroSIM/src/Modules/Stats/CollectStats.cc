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

#include "CollectStats.h"
#include <inet/common/ModuleAccess.h>
#include "stack/phy/packet/cbr_m.h"



static const simsignal_t packetSentToLowerSignal = cComponent::registerSignal("packetSentToLower");
static const simsignal_t packetReceivedFromLowerSignal = cComponent::registerSignal("packetReceivedFromLower");

static const  simsignal_t lteSentMsg = cComponent::registerSignal("sentMsg"); // LTE sent Msg
static const  simsignal_t lteRcvdMsg = cComponent::registerSignal("receivedMsg"); // LTE received Msg


Define_Module(CollectStats);

void CollectStats::initialize()
{

    cModule* host = inet::getContainingNode(this);
    // 802.11
    inet::ieee80211::Ieee80211Mac*  mLinkLayer80211 = inet::findModuleFromPar<inet::ieee80211::Ieee80211Mac>(par("macModule80211"), host);
    mLinkLayer80211->subscribe(packetSentToLowerSignal, this);
    mLinkLayer80211->subscribe(packetReceivedFromLowerSignal, this);
    // 802.15
    int numRadios=getAncestorPar("numRadios");
    if(numRadios>1)
    {
        CSMA* mLinkLayerNarrowBand=inet::findModuleFromPar<inet::CSMA>(par("macModule80215"), host);
        mLinkLayerNarrowBand->subscribe(packetSentToLowerSignal, this);
        mLinkLayerNarrowBand->subscribe(packetReceivedFromLowerSignal, this);
    }

    // Lte
    bool mode4=getAncestorPar("withMode4");
    if(mode4)
    {
        LtePhyVUeMode4* mRadioLte= inet::findModuleFromPar<LtePhyVUeMode4>(par("radioModuleLte"), host);
        mRadioLte->subscribe(lteSentMsg,this);
        mRadioLte->subscribe(lteRcvdMsg, this);
    }

    dtlMin=par("dtlMin").doubleValue();
}




void CollectStats::computeThroughput(simtime_t now, unsigned long bits, double& throughput)
{
    numPackets++;
    numBits += bits;

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
    {
        simtime_t duration = now - intvlStartTime;
        // record measurements
        throughput=intvlNumBits / duration.dbl();// bps
        // restart counters
        intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
        intvlNumPackets = intvlNumBits = 0;
    }

    intvlNumPackets++;
    intvlNumBits += bits;
    intvlLastPkTime = now;

}



void CollectStats::recordStats(simsignal_t comingSignal, simsignal_t signalSent, simsignal_t signalRcv,cObject* msg, listOfCriteria& l)
{
    // local variables
    long sent=0;
    long rcv=0;
    double latency=0;
    double th=0;
    double rel=0;

    if(comingSignal==signalSent)
    {
        sent++;
        l.sentPackets.push_back(sent);
    }

    if (comingSignal == signalRcv)
    {
        // Latency
        auto hetMsg=dynamic_cast<cMessage*>(msg);
        latency=(simTime()-hetMsg->getTimestamp()).dbl();
        l.latency.push_back(latency);
        // Throughput
        computeThroughput(simTime(), PK(hetMsg)->getBitLength(),th);
        l.throughput.push_back(th);
        // rcved packets
        rcv++;
        l.receivedPackets.push_back(rcv);
    }

    if(sent!=0)
    {
        rel=(rcv/sent)*100;
        l.reliability.push_back(rel);
    }
}



void CollectStats::prepareNetAttributes(double cv, double dtl)
{

    // TODO: implement DTL adaptation
    // dtl=f(cv)

    // TODO: call EMA
    // return allPathsCriteriaValues for criteria matrix

}



void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{

    std::string ModuleName=source->getParentModule()->getFullName();
    int interfaceId=Utilities::extractNumber(ModuleName);

    // Wlan interfaces
    if(interfaceId==0)
    {
        recordStats(signal,packetSentToLowerSignal,packetReceivedFromLowerSignal,msg,listOfCriteria80211);

    }
    else if(interfaceId==1)
    {
        recordStats(signal,packetSentToLowerSignal,packetReceivedFromLowerSignal,msg,listOfCriteria80215);
    }
    else
    {
        EV_INFO<< "no valid interface" << endl;
    }

    // lte
    recordStats(signal,lteSentMsg,lteRcvdMsg,msg,listOfCriteriaLte);
    //double test=Utilities::calculateEWA_BiasCorrection(listOfCriteria80211.latency,0.3);
    //std::cout<< "test ema = " << test;
    // verif
    //std::cout << "latency= "  << listOfCriteria80211.latency;

    // TODO call prepareNetAttributes


}



