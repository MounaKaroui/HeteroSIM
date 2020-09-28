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
            //subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetFromUpperDroppedSignal);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromLowerSignal);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            //NF_LINK_BREAK dropped signal for CSMA FixMe
            //NF_PACKET_DROP dropped signal // FixMe
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

void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName()<< endl;
}



void CollectStats::recordStatsForWlan(simsignal_t comingSignal, cMessage* msg,  listOfCriteria* l)
{
    // local variables
    double macDelay=0;
    double sent=0;
    double dropped=0;
    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  msg->getOwner()->getName()==string("mac") ){

        printMsg("Inserting",msg);

        packetFromUpperTimeStamps[msg->getTreeId()]=NOW; // FixMe

    } else if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal && msg->getOwner()->getName()==string("radio")) {

        //ASSERT(l->packetFromUpperTimeStamps.find(msg->getTreeId()) != l->packetFromUpperTimeStamps.end());
        macDelay = (NOW - packetFromUpperTimeStamps[msg->getTreeId()]).dbl();
        printMsg("Reading",msg);
        packetFromUpperTimeStamps.erase(msg->getTreeId());
        l->delay.push_back(macDelay);
        sent++;
        l->sentPackets.push_back(sent);
    }

    if(comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal)
    {
        dropped++;
        l->droppedPackets.push_back(dropped);
    }
    // for Throughput calculation
    recordThroughputStats(comingSignal,
            LayeredProtocolBase::packetReceivedFromLowerSignal, msg, l);

}



void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg,  listOfCriteria* l)
{
    double sent=0;
    double macDelay=0;
     map<long,simtime_t> packetFromUpperTimeStamps;
     if ( comingSignal == LteMacBase::receivedPacketFromUpperLayer){

           printMsg("Inserting",msg);
           packetFromUpperTimeStamps[msg->getTreeId()]=NOW; //

       } else if ( comingSignal ==  LtePhyVUeMode4::rcvdFromUpperLayerSignal) {
           macDelay = (NOW - packetFromUpperTimeStamps[msg->getTreeId()]).dbl();
           printMsg("Reading",msg);
           packetFromUpperTimeStamps.erase(msg->getTreeId());
           l->delay.push_back(macDelay);
           sent++;
           l->sentPackets.push_back(sent);
       }
    recordThroughputStats(comingSignal,LteMacBase::receivedPacketFromLowerLayer,msg,l);
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
        Utilities::calculateMeanVec(listOfCriteriabyInterfaceId[i]->throughput)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(
        Utilities::calculateMeanVec(listOfCriteriabyInterfaceId[i]->delay)));

        criteriaStr.push_back( boost::lexical_cast<std::string>(
                Utilities::calculateMeanVec(listOfCriteriabyInterfaceId[i]->reliability)));
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
        b = msgName == "hetNets" ;
        if(b){
            std::string fullModuleName = source->getParentModule()->getFullName();
            interfaceId = Utilities::extractNumber(fullModuleName);
            if(listOfCriteriabyInterfaceId.find(interfaceId) == listOfCriteriabyInterfaceId.end())
                listOfCriteriabyInterfaceId.insert({ interfaceId, new listOfCriteria()});
            recordStatsForWlan(signal, packet,listOfCriteriabyInterfaceId[interfaceId]); // FixMe
        }
    }else if(moduleName=="lteNic")
    {
        // Fixme : which message name to consider as mine...
        std::vector<int> result;
        bool searchResult=Utilities::findKeyByValue(result, interfaceToProtocolMap, string("mode4"));
        if(searchResult)
        {
            interfaceId=result.at(0);
            if(listOfCriteriabyInterfaceId.find(interfaceId)== listOfCriteriabyInterfaceId.end())
                listOfCriteriabyInterfaceId.insert({interfaceId,new listOfCriteria()});
            recordStatsForLte(signal, packet, listOfCriteriabyInterfaceId[interfaceId]);
        }
       }
    // TODO call prepareNetAttributes with DLT adaptation ;
    //prepareNetAttributes();
}





