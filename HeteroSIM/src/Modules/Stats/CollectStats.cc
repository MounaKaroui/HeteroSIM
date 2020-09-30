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
#include "stack/pdcp_rrc/layer/LtePdcpRrcUeD2D.h"
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>

Define_Module(CollectStats);
static const  simsignal_t receivedPacketFromUpperLayerLteSignal = cComponent::registerSignal("receivedPacketFromUpperLayer");
//static const  simsignal_t macPacketLossD2D=cComponent::registerSignal("macPacketLossD2D");

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
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetSentToLowerSignal);

//            cModule* module = getModuleByPath(macModuleName.c_str());
//            module->isSubscribed(signalID, listener)
            //NF_LINK_BREAK dropped signal for CSMA Todo
            //NF_PACKET_DROP dropped signal // Todo
        }
        else if(result[1]=="mode4")
        {
            std::string pdcpRrcModuleName = "^.lteNic.pdcpRrc";
            std::string macModuleName = "^.lteNic.mac";
            std::string phyModuleName = "^.lteNic.phy";
            bool mode4 = getAncestorPar("withMode4");
            if (mode4) {

                    subscribeToSignal<LtePdcpRrcUeD2D>(pdcpRrcModuleName,receivedPacketFromUpperLayerLteSignal);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::rcvdFromUpperLayerSignal);
                    //subscribeToSignal<LteMacVUeMode4>(macModuleName, macPacketLossD2D);

            }
        }

    }
}


void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName() << endl;
}


void CollectStats::recordThroughputStats(int interfaceId,cMessage* msg, double interval)
{
    double th = (listOfCriteriaByInterfaceId[interfaceId]->sentPackets)*(PK(msg)->getBitLength())/interval; // bps
    listOfCriteriaByInterfaceId[interfaceId]->effectiveTransmissionRate.push_back(th);
}




void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{
    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
        listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    // local variables
    simtime_t macDelay;
    simtime_t transmissionDurattion;

    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  sourceName==string("mac") ){

        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;
        printMsg("Inserting",msg);

    } else if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) {

        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        printMsg("Reading",msg);
        macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];

        RadioFrame *radioFrame = check_and_cast<RadioFrame *>(msg);
        ASSERT(radioFrame && radioFrame->getDuration() != 0) ;
        transmissionDurattion = radioFrame->getDuration() ;

        simtime_t delay = macDelay+transmissionDurattion;  // delay.inUnit(SIMTIME_MS)
        listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(SIMTIME_DBL(delay));
        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
        listOfCriteriaByInterfaceId[interfaceId]->sentPackets++;
    }

    if(comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal) // for packet drop calculation
    {
        listOfCriteriaByInterfaceId[interfaceId]->droppedPackets++;
    }

//    recordThroughputStats(interfaceId, msg, delay);
}


void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{

    simtime_t macDelay;
    if(listOfCriteriaByInterfaceId.find(interfaceId)== listOfCriteriaByInterfaceId.end())
                    listOfCriteriaByInterfaceId.insert({interfaceId,new listOfCriteria()});

     if ( comingSignal ==receivedPacketFromUpperLayerLteSignal){
         std::string msgName=PK(msg)->getName();
         if((msgName.find("hetNets")==0))
         {
         FlowControlInfoNonIp* lteInfo = check_and_cast<FlowControlInfoNonIp*>(PK(msg)->getControlInfo());
         packetFromUpperTimeStampsByInterfaceId[interfaceId][to_string(lteInfo->getMsgFlag())]=NOW;
         }
     }
     if ( comingSignal ==  LtePhyVUeMode4::rcvdFromUpperLayerSignal) {
         if(msg->getClassName()==string("LteMacPdu"))
         {
             LteMacPdu* pkt=dynamic_cast<LteMacPdu*>(msg);
             UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(pkt->getControlInfo());
             std::string msgFlag=to_string(lteInfo->getMsgFlag());
             ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msgFlag) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
             macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msgFlag];
             listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(macDelay.dbl());
             packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgFlag);
             listOfCriteriaByInterfaceId[interfaceId]->sentPackets++;
             recordThroughputStats(interfaceId,msg,macDelay.dbl());
         }

       }
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

    for(int i=0; i<3;i++) // Fixme 3 is the number of alternatives
    {
        // TODO change Calculate Mean with EMA and DTL adaptation staff
//        criteriaStr.push_back( boost::lexical_cast<std::string>(
//        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->effectiveTransmissionRate)));
//
//        criteriaStr.push_back( boost::lexical_cast<std::string>(
//
//        Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->delay)));
//
//        criteriaStr.push_back( boost::lexical_cast<std::string>(
//                Utilities::calculateMeanVec(listOfCriteriaByInterfaceId[i]->reliability)));
    }
//    for (unsigned int a = 0; a < criteriaStr.size(); ++a) {
//        if (a == 0) {
//            pathsCriteriaValues = pathsCriteriaValues + criteriaStr[a];
//        } else {
//            pathsCriteriaValues = pathsCriteriaValues + ","
//                    + criteriaStr[a];
//        }}
//    allPathsCriteriaValues = allPathsCriteriaValues + pathsCriteriaValues
//            + ",";
//    criteriaStr.clear();
     // prepare data
    /// return string that contains alternatives and call EMA
    // calculate dtl;
    // organize the data per alternative
}


void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, double value,cObject *details)
{
//    std::string moduleName = source->getParentModule()->getName();
//    if (moduleName == "lteNic") {
//
//        std::vector<int> result;
//        bool searchResult = Utilities::findKeyByValue(result,
//                interfaceToProtocolMap, string("mode4"));
//        if (searchResult) {
//            int interfaceId = result.at(0);
//            if (signal == macPacketLossD2D) {
//
//                listOfCriteriaByInterfaceId[interfaceId]->droppedPackets++;
//            }
//        }
//    }

}

void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{
    std::string moduleName = source->getParentModule()->getName();
    int interfaceId=0;
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
          recordStatsForWlan(signal,source->getName(),packet,interfaceId);

        }
    }else if(moduleName=="lteNic")
    {

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





