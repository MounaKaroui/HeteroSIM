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
    batchSize=par("batchSize").intValue();
    maxInterval=SimTime(par("maxInterval").doubleValue());
    registerSignals();
}



void CollectStats::registerSignals()
{

    cStringTokenizer tokenizer(interfaceToProtocolMapping.c_str(),string(";").c_str());
    while (tokenizer.hasMoreTokens()){

        std::string mappingEntry= tokenizer.nextToken();
        vector<string> result;
        boost::split(result, mappingEntry, boost::is_any_of(":"));
        interfaceToProtocolMap.insert({stoi(result[0]),result[1]});

        std::string macModuleName= "^.wlan["+ result[0]+"].mac";
        std::string radioModuleName= "^.wlan["+ result[0]+"].radio";

        // TODO: subscribe to the drop signals

        if (result[1] == "80211" || result[1] == "80215") {
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetFromUpperDroppedSignal);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromLowerSignal);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            //NF_LINK_BREAK dropped signal for CSMA Todo
            //NF_PACKET_DROP dropped signal // Todo
        }
        else if(result[1]=="mode4")
        {
            std::string macModuleName = "^.lteNic.mac";
            std::string phyModuleName = "^.lteNic.phy";
            bool mode4 = getAncestorPar("withMode4");
            if (mode4) {
                    subscribeToSignal<LteMacVUeMode4>(macModuleName,LteMacBase::receivedPacketFromLowerLayer);
                    subscribeToSignal<LteMacVUeMode4>(macModuleName,LteMacBase::receivedPacketFromUpperLayer);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::rcvdFromUpperLayerSignal);

            }
        }

    }
}


void  CollectStats::computeThroughput(simtime_t now,unsigned long bits,double& th)
{
    // FixMe
    //(measurement ends if either batchSize or maxInterval is reached, whichever is reached first)
//
//    numPackets++;
//    numBits += bits;
//
//    // packet should be counted to new interval
//    if (th.intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
//    {
//        simtime_t duration = now - intvlStartTime;
//        // record measurements
//        throughput=intvlNumBits / duration.dbl();// bps ( sum of bits)
//        // restart counters
//        intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
//        intvlNumPackets = intvlNumBits = 0;
//    }
}

void CollectStats::recordThroughputStats(simsignal_t comingSignal,simsignal_t sigName, cMessage* msg, int interfaceId)
{
       double th=0;
       double rcvd=0;
       if (comingSignal == sigName)
       {
           // Throughput
           computeThroughput(simTime(), PK(msg)->getBitLength(),th); // FixMe
           listOfCriteriaByInterfaceId[interfaceId]->throughput.push_back(th);
           // rcvd packets
           rcvd++;
           listOfCriteriaByInterfaceId[interfaceId]->receivedPackets.push_back(rcvd);
       }

}

void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName()<< endl;
}



void CollectStats::recordStatsForWlan(simsignal_t comingSignal, cMessage* msg,  int interfaceId)
{
    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
        listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    // local variables
    double macDelay=0;
    double sent=0;
    double dropped=0;

    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  msg->getOwner()->getName()==string("mac") ){

        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;
        printMsg("Inserting",msg);

    } else if (comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal && msg->getOwner()->getName()==string("radio")) {

        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        macDelay = (NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]).dbl();
        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
        listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(macDelay);
        sent++;
        listOfCriteriaByInterfaceId[interfaceId]->sentPackets.push_back(sent);
        printMsg("Reading",msg);
    }

    if(comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal) // for packet drop calculation
    {
        dropped++;
        listOfCriteriaByInterfaceId[interfaceId]->droppedPackets.push_back(dropped);
    }

    recordThroughputStats(comingSignal,LayeredProtocolBase::packetReceivedFromLowerSignal, msg, interfaceId);
}


void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{
    double sent=0;
    double macDelay=0;

    if(listOfCriteriaByInterfaceId.find(interfaceId)== listOfCriteriaByInterfaceId.end())
                    listOfCriteriaByInterfaceId.insert({interfaceId,new listOfCriteria()});

     if ( comingSignal == LteMacBase::receivedPacketFromUpperLayer){

           //printMsg("Inserting",msg);
           // FixMe: Lte delay

       }

     if ( comingSignal ==  LtePhyVUeMode4::rcvdFromUpperLayerSignal) {

       // TODO LTE delay
           sent++;
           listOfCriteriaByInterfaceId[interfaceId]->sentPackets.push_back(sent);
       }
    recordThroughputStats(comingSignal,LteMacBase::receivedPacketFromLowerLayer,msg,interfaceId);
    // Todo
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
        // TODO change Calculate Mean with EMA and DTL adaptation staff
        criteriaStr.push_back( boost::lexical_cast<std::string>(
        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->throughput)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(

        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->delay)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(
                Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->reliability)));
    }
    for (unsigned int a = 0; a < criteriaStr.size(); ++a) {
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




void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{
    std::string moduleName = source->getParentModule()->getName();
    int interfaceId=0;
    bool b=false;
    auto packet=dynamic_cast<cMessage*>(msg);
    std::string msgName=packet->getName();
    if(moduleName=="wlan")
    {
        std::string msgName=packet->getName();
        bool b = (msgName.find("hetNets") == 0);
        if(b)
        {
          std::string fullModuleName = source->getParentModule()->getFullName();
          interfaceId = Utilities::extractNumber(fullModuleName);
          recordStatsForWlan(signal,packet,interfaceId);

        }
    }else if(moduleName=="lteNic")
    {
        // Fixme : which message name to consider as mine...
        std::vector<int> result;
        bool searchResult=Utilities::findKeyByValue(result, interfaceToProtocolMap, string("mode4"));
        if(searchResult)
        {
            interfaceId=result.at(0);
            recordStatsForLte(signal, packet, interfaceId);
        }
       }
    // TODO call prepareNetAttributes with DLT adaptation ;
    //prepareNetAttributes();
}





