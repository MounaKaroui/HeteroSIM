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
    mLinkLayer80211 = inet::findModuleFromPar<inet::ieee80211::Ieee80211Mac>(par("macModule80211"), host);
    mRadio80211 = inet::findModuleFromPar<inet::physicallayer::Ieee80211Radio>(par("radioModule80211"), host);
    mLinkLayer80211->subscribe(packetSentToLowerSignal, this);
    mLinkLayer80211->subscribe(packetReceivedFromLowerSignal, this);

    // 802.15
    int numRadios=getAncestorPar("numRadios");
    if(numRadios>1)
    {
    mLinkLayerNarrowBand=inet::findModuleFromPar<inet::CSMA>(par("macModule80215"), host);
    mRadioNarrowBand= inet::findModuleFromPar<inet::physicallayer::FlatRadioBase>(par("radioModule80215"), host);
    mLinkLayerNarrowBand->subscribe(packetSentToLowerSignal, this);
    mLinkLayerNarrowBand->subscribe(packetReceivedFromLowerSignal, this);
    }

    // Lte
    bool mode4=getAncestorPar("withMode4");
    if(mode4)
    {
    mRadioLte= inet::findModuleFromPar<LtePhyVUeMode4>(par("radioModuleLte"), host);
    mRadioLte->subscribe(lteSentMsg,this);
    mRadioLte->subscribe(lteRcvdMsg, this);
    }

    // For throughput Measurement
    batchSize=10;
    startTime=0;
    maxInterval=1;
    numPackets = numBits = 0;
    intvlStartTime = intvlLastPkTime = 0;
    intvlNumPackets = intvlNumBits = 0;

    // list of criteria initialization
    inializeCriteriaList(listOfCriteria80211);
    inializeCriteriaList(listOfCriteria80215);
    inializeCriteriaList(listOfCriteriaLte);

}


void CollectStats::inializeCriteriaList(listOfCriteria& l)
{

    l.receivedPackets=0;
    l.sentPackets=0;

    l.latency=0;
    l.reliability=0;
    l.throughput=0;

}

void CollectStats::computeThroughput(simtime_t now, unsigned long bits, double& throughput)
{
    numPackets++;
    numBits += bits;

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
        beginNewInterval(now, throughput);

    intvlNumPackets++;
    intvlNumBits += bits;
    intvlLastPkTime = now;

}


void CollectStats::beginNewInterval(simtime_t now, double& throughput)
{
    simtime_t duration = now - intvlStartTime;
    // record measurements
    throughput=intvlNumBits / duration.dbl();// bps
    // restart counters
    intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
    intvlNumPackets = intvlNumBits = 0;
}



void CollectStats::recordStats(simsignal_t comingSignal, simsignal_t signalSent, simsignal_t signalRcv,cObject* msg, listOfCriteria& l)
{
    if(comingSignal==signalSent)
       {
           l.sentPackets++;
       }

    if (comingSignal == signalRcv)
    {
       auto hetMsg=dynamic_cast<cMessage*>(msg);
       l.latency=(simTime()-hetMsg->getTimestamp()).dbl();
       computeThroughput(simTime(), PK(hetMsg)->getBitLength(),l.throughput);
       l.receivedPackets++;
    }

    if(l.sentPackets!=0)
    {
        l.reliability=(l.receivedPackets/l.sentPackets)*100;
    }
}


std::vector<double> CollectStats::convertStatsToVector(double cri_alter0, double cri_alter1, double cri_alter2)
{

    std::vector<double> criteriaList;
    criteriaList.push_back(cri_alter0);
    criteriaList.push_back(cri_alter1);
    criteriaList.push_back(cri_alter2);
    return criteriaList;
}

// TODO: implement stats adaptation

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{

      std::string ModuleName=source->getParentModule()->getFullName();
      int interfaceId=Builder::extractNumber(ModuleName);

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

      std::vector<double> thList=convertStatsToVector(listOfCriteria80211.throughput,
              listOfCriteria80215.throughput, listOfCriteriaLte.throughput);

      std::vector<double> delayList=convertStatsToVector(listOfCriteria80211.latency,
              listOfCriteria80215.latency, listOfCriteriaLte.latency);

      std::vector<double> relList=convertStatsToVector(listOfCriteria80211.reliability,
              listOfCriteria80215.reliability, listOfCriteriaLte.reliability);

      //allPathsCriteriaValues=McdaAlg::buildAllPathThreeCriteria(thList, delayList, relList);
}



