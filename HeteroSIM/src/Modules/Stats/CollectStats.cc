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



static const simsignal_t packetSentToLowerSignal = cComponent::registerSignal("packetSentToLower");
static const simsignal_t packetReceivedFromLowerSignal = cComponent::registerSignal("packetReceivedFromLower");
static const  simsignal_t packetErrorRateSignal = cComponent::registerSignal("packetErrorRate"); // reliability

Define_Module(CollectStats);

void CollectStats::initialize()
{


    cModule* host = inet::getContainingNode(this);
    // 802.11
    mLinkLayer80211 = inet::findModuleFromPar<inet::ieee80211::Ieee80211Mac>(par("macModule80211"), host);
    mRadio80211 = inet::findModuleFromPar<inet::physicallayer::Ieee80211Radio>(par("radioModule80211"), host);

    // subscription to signals
    mLinkLayer80211->subscribe(packetSentToLowerSignal, this);
    mLinkLayer80211->subscribe(packetReceivedFromLowerSignal, this);
    mRadio80211->subscribe(packetErrorRateSignal, this);

    // 802.15
    mLinkLayerNarrowBand=inet::findModuleFromPar<inet::CSMA>(par("macModule80215"), host);
    mRadioNarrowBand= inet::findModuleFromPar<inet::physicallayer::FlatRadioBase>(par("radioModule80215"), host);

    mLinkLayerNarrowBand->subscribe(packetSentToLowerSignal, this);
    mLinkLayerNarrowBand->subscribe(packetReceivedFromLowerSignal, this);
    mRadioNarrowBand->subscribe(packetErrorRateSignal, this);





    // For throughput Measurement
    batchSize=10;
    startTime=0;
    maxInterval=1;
    numPackets = numBits = 0;
    intvlStartTime = intvlLastPkTime = 0;
    intvlNumPackets = intvlNumBits = 0;

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

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, double value,cObject *details)
{

    std::string ModuleName=source->getParentModule()->getFullName();
    int interfaceId=Builder::extractNumber(ModuleName);

    if(interfaceId==0)
    {
    if(signal==packetErrorRateSignal)
    {
        listOfCriteria80211.per=value;
    }
    }else if(interfaceId==1)
    {
       if(signal==packetErrorRateSignal)
            {
                listOfCriteria80215.per=value;
            }
    }
    else
    {
     EV_INFO<< " wrong interface Id "<<endl;
    }

}

void CollectStats::recordStats(simsignal_t signal,cObject* msg, listOfCriteria& l)
{
    if(signal==packetSentToLowerSignal)
       {
           l.sentPackets++;
       }

    if (signal == packetReceivedFromLowerSignal)
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

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{

      std::string ModuleName=source->getParentModule()->getFullName();
      int interfaceId=Builder::extractNumber(ModuleName);

      if(interfaceId==0)
      {
          recordStats(signal,msg,listOfCriteria80211);
          double latency= listOfCriteria80211.latency;
          double throughput=listOfCriteria80211.throughput;
      }
      else if(interfaceId==1)
      {
          recordStats(signal,msg,listOfCriteria80215);
      }
      else
      {
       EV_INFO<< " wrong interface Id "<<endl;
      }

}



