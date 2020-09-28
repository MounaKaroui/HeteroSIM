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
#include <numeric>
#include "../../Modules/DecisionMaker/DecisionMaker.h"
#include<boost/lexical_cast.hpp>



#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>

Define_Module(CollectStats);

void CollectStats::initialize()
{

    interfaceToProtocolMapping =par("interfaceToProtocolMapping").stringValue();
    dltMin=par("dltMin").doubleValue();
    registerSignals();
}


template<typename T>
void CollectStats::subscribeToSignal(std::string moduleName, simsignal_t sigName)
{
    cModule* module = getModuleByPath(moduleName.c_str());
    T * mLinkLayer=dynamic_cast<T*>(module);
    mLinkLayer->subscribe(sigName, this);
}



void CollectStats::registerSignals()
{

    cStringTokenizer tokenizer(interfaceToProtocolMapping.c_str(),string(";").c_str());
    while (tokenizer.hasMoreTokens()){

        std::string mappingEntry= tokenizer.nextToken();
        vector<string> result;
        boost::split(result, mappingEntry, boost::is_any_of(":"));
        interfaceToProtocolMap.insert({stoi(result[0]),result[1]});

        std::string radioModuleName= "^.wlan["+ result[0]+"].radio";
        std::string macModuleName= "^.wlan["+ result[0]+"].mac";


        if (result[1] == "80211" || result[1] == "80215") {
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromLowerSignal);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
        }
        else if(result[1]=="mode4")
        {
            std::string moduleName = "^.lteNic.phy"; // mac packet received from lower
            bool mode4 = getAncestorPar("withMode4");
            if (mode4) {
//                subscribeToSignal<LtePhyVUeMode4>(moduleName,LtePhyVUeMode4::sentMsgSignal);
//                subscribeToSignal<LtePhyVUeMode4>(moduleName, LtePhyVUeMode4::rcvdMsgSignal);
//                subscribeToSignal<LtePhyVUeMode4>(moduleName, LtePhyVUeMode4::interPacketDelay);

            }
        }

    }
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

void CollectStats::recordThroughputStats(simsignal_t comingSignal,simsignal_t sigName, cMessage* msg,  listOfCriteria* l)
{
       double th=0;
       double rcvd=0;
       if (comingSignal == sigName)
       {
           // Throughput
           computeThroughput(simTime(), PK(msg)->getBitLength(),th);
           l->throughput.push_back(th);
           // rcvd packets
           rcvd++;
           l->receivedPackets.push_back(rcvd);
       }

}


void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{
    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
        listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    // local variables
    double macDelay=0;
    double sent=0;
    //    if (LayeredProtocolBase::packetReceivedFromLowerSignal) {
    //        std::map<int, simtime_t>::iterator jt =
    //                wlanPreviousTransmissionTimes.find(msg->getSenderGateId());
    //        if (jt != wlanPreviousTransmissionTimes.end()) {
    //            simtime_t elapsed_time = NOW - jt->second;
    //            l->interPacketGap.push_back(elapsed_time.dbl());
    //        }
    //        wlanPreviousTransmissionTimes[msg->getSenderGateId()] = NOW;
    //    }

    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  sourceName=="mac"){

        std::cout<< simTime()<<", Inserting id=" << msg->getName() << ", Msg name=" << msg->getName()
                                          << ", Class Name=" << msg->getClassName()
                                          << ", Owner=" << msg->getOwner()->getName()<< endl;

        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;

    } else if (comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal && sourceName=="radio") {

        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        macDelay = (NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]).dbl();

        std::cout<< simTime()<<", Reading id=" << msg->getName() <<", Msg name=" << msg->getName()
                                          << ", Class Name=" << msg->getClassName()
                                          << ", Owner=" << msg->getOwner()->getName()<< endl;

        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
        listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(macDelay);

        //               sent++;
        //                listOfCriteriaByInterfaceId[interfaceId]->sentPackets.push_back(sent);
    }
    //    recordThroughputStats(comingSignal,
    //            LayeredProtocolBase::packetReceivedFromLowerSignal, msg, l);
}


void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg,  listOfCriteria* l)
{

//    double sent=0;
//    // TODO add delay
//    if (comingSignal == LtePhyVUeMode4::sentMsgSignal) {
//        sent++;
//        l->sentPackets.push_back(sent);
//
//    }
//    recordThroughputStats(comingSignal,LtePhyVUeMode4::rcvdMsgSignal,msg,l);
}



double CollectStats::updateDLT(double x)
{
    return exp(-1*x) + dltMin;
}

void CollectStats::prepareNetAttributes()
{
    std::string pathsCriteriaValues = "";
    std::vector<std::string> criteriaStr;
    std::string critValuesPerPathStr = "";
    for(int i=0; i<3;i++) // Fixme
    {
        criteriaStr.push_back( boost::lexical_cast<std::string>(
        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->throughput)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(
        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->interPacketGap)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(
                Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->reliability)));
    }
    for (int a = 0; a < criteriaStr.size(); ++a) {
        if (a == 0) {
            pathsCriteriaValues = pathsCriteriaValues + criteriaStr[a];
        } else {
            pathsCriteriaValues = pathsCriteriaValues + ","
                    + criteriaStr[a];
        }}
    allPathsCriteriaValues = allPathsCriteriaValues + pathsCriteriaValues
            + ",";
    criteriaStr.clear();
     // prepare data
    /// return string that contains alternatives and call EMA
    // calculate dtl;
    // organize the data per alternative
}


void CollectStats::printMapElements()
{

       for (auto itr = listOfCriteriaByInterfaceId.begin(); itr != listOfCriteriaByInterfaceId.end(); ++itr) {
           cout << "\n KEY\tELEMENT\n";
           cout << itr->first <<" ";
           if(itr->second->delay.size()>0)
           {
               cout << "\n Elements in my map are\n";
               std::cout << '\t' <<  itr->second->delay.at(0)<< '\n';
           }

       }
}

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, double value,cObject *details)
{
    std::string moduleName = source->getParentModule()->getName();
    if(moduleName=="lteNic")
    {
//    if(signal== LtePhyVUeMode4::interPacketDelay)
//    {
//        listOfCriteriabyInterfaceId.insert({ MODE4, new listOfCriteria() });
//        listOfCriteriabyInterfaceId[MODE4]->interPacketGap.push_back(value);
//    }
    }

}

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{

    std::string moduleName = source->getParentModule()->getName();
    int interfaceId=0;
    auto packet=dynamic_cast<cMessage*>(msg);
    std::string msgName=packet->getName();
    bool b = (msgName.find("hetNets") == 0);
       if(b){ // To ignore any control traffic
        if(moduleName=="wlan")
        {
            std::string fullModuleName = source->getParentModule()->getFullName();
            interfaceId = Utilities::extractNumber(fullModuleName);
            recordStatsForWlan(signal,source->getName(),packet,interfaceId );
        }
        else if(moduleName=="lteNic")
        {
            interfaceId=MODE4;
           if(listOfCriteriaByInterfaceId.find(interfaceId)== listOfCriteriaByInterfaceId.end())
                listOfCriteriaByInterfaceId.insert({interfaceId,new listOfCriteria()});
            recordStatsForLte(signal, packet, listOfCriteriaByInterfaceId[interfaceId]);
        }
    }
    // TODO call prepareNetAttributes with DLT adaptation ;
    //prepareNetAttributes();
}





