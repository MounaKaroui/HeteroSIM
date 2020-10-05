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
#include "common/LteCommon.h"


Define_Module(CollectStats);
static const  simsignal_t receivedPacketFromUpperLayerLteSignal = cComponent::registerSignal("receivedPacketFromUpperLayer");
static const simsignal_t  droppedPacketsLteSignal=cComponent::registerSignal("macBufferOverflowD2D");

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


        if (result[1] == "80211" || result[1] == "80215") {

            std::string macModuleName= "^.wlan["+ result[0]+"].mac";
            std::string radioModuleName= "^.wlan["+ result[0]+"].radio";

            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetSentToLowerSignal);

            //packet drop signals
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_PACKET_DROP);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_LINK_BREAK);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetFromUpperDroppedSignal); //TODO seek into replacing this signal by NF_PACKET_DROP --> modification of CSMA.cc

        }
        else if(result[1]=="mode4")
        {
            std::string pdcpRrcModuleName = "^.lteNic.pdcpRrc";
            std::string macModuleName = "^.lteNic.mac";
            std::string phyModuleName = "^.lteNic.phy";
            bool mode4 = getAncestorPar("withMode4");
            if (mode4) {

                    subscribeToSignal<LtePdcpRrcUeD2D>(pdcpRrcModuleName,receivedPacketFromUpperLayerLteSignal);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::sentToLowerLayerSignal);
                    subscribeToSignal<LteMacUeRealistic>(macModuleName,droppedPacketsLteSignal);
            }
        }

    }
}




void CollectStats::computeEffectiveTransmissionRate(int interfaceId,cMessage* msg, double interval)
{
    double effectiveRate= (listOfCriteriaByInterfaceId[interfaceId]->sentPacketsToLower)*(PK(msg)->getBitLength())/interval; // bps
    listOfCriteriaByInterfaceId[interfaceId]->effectiveTransmissionRate.push_back(effectiveRate);
}




void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{
    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
        listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});


    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  sourceName==string("mac") ){ //when packet enter to MAC layer

        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;
        printMsg("Inserting",msg);

    } else if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //when the packet comes out of the radio layer --> transmission duration already elapsed

        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        printMsg("Reading",msg);
        simtime_t macAndRadioDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];

        RadioFrame *radioFrame = check_and_cast<RadioFrame *>(msg);
        ASSERT(radioFrame && radioFrame->getDuration() != 0) ;
        /* TODO Uncomment the next line in case of the need of the macDelay only*/
//        simtime_t macDelay = macAndRadioDelay -radioFrame->getDuration();

        //Delay metric
        listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(SIMTIME_DBL(macAndRadioDelay));
        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());

        //Successful transmission metric
        listOfCriteriaByInterfaceId[interfaceId]->sentPacketsToLower++;
        double successfulTransmissionRate = getCurrentInterfaceSuccessfulTransmissionRate(interfaceId);
        listOfCriteriaByInterfaceId[interfaceId]->successfulTransmissionRate.push_back(successfulTransmissionRate);

        //Transmission rate metric
        computeEffectiveTransmissionRate(interfaceId, msg,  (radioFrame->getDuration()).dbl());

        //TODO record time stamp
//        listOfCriteriaByInterfaceId[interfaceId]->timeStamp=NOW;
        return ;
    }

    if (comingSignal== NF_PACKET_DROP || comingSignal== NF_LINK_BREAK || comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal) // packet drop related calculations
    {
        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        printMsg("Reading",msg);
        simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];

        if(macDelay == 0){ //case of packet drop due to queue overflow
            //In case of full queue 802.15.4 interface sends "packetFromUpperDroppedSignal" signal
            //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
            //so check the following assertion
            ASSERT(comingSignal == NF_PACKET_DROP  || comingSignal == LayeredProtocolBase::packetFromUpperDroppedSignal);
            //TODO add delay penalties to consider in case of packet drop
            throw cRuntimeError("Packet drop due to queue overflow not supported yet");

        }else { //case of packet drop due failing CSMA process

            //Successful transmission metric
            listOfCriteriaByInterfaceId[interfaceId]->droppedPackets++;
            double currentSuccessfulTransmissionRate = getCurrentInterfaceSuccessfulTransmissionRate(interfaceId);
            listOfCriteriaByInterfaceId[interfaceId]->successfulTransmissionRate.push_back(currentSuccessfulTransmissionRate);

            //Delay metric
            listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(SIMTIME_DBL(macDelay));
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());

            //TODO record time stamp

        }

    }


//    recordThroughputStats(interfaceId, msg, delay);
}




void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{

    simtime_t lteInterLayerDelay;
    if(listOfCriteriaByInterfaceId.find(interfaceId)== listOfCriteriaByInterfaceId.end())
        listOfCriteriaByInterfaceId.insert({interfaceId,new listOfCriteria()});

    if ( comingSignal ==receivedPacketFromUpperLayerLteSignal){ // When a packet enter to PDCP_RRC layer
        std::string msgName=PK(msg)->getName();
        if((msgName.find("hetNets")==0))
        {
            FlowControlInfoNonIp* lteInfo = check_and_cast<FlowControlInfoNonIp*>(PK(msg)->getControlInfo());
            packetFromUpperTimeStampsByInterfaceId[interfaceId][to_string(lteInfo->getMsgFlag())]=NOW;
        }
    }

    if ( comingSignal ==  LtePhyVUeMode4::sentToLowerLayerSignal) { // when the  packet comes to the Phy layer for transmission
        LteAirFrame* lteAirFrame=dynamic_cast<LteAirFrame*>(msg);
        // TODO: check again if the LTE MAC PDU is corresponding to my data
        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
        if(Utilities::checkLteCtrlInfo(lteInfo)){
            std::string msgFlag = to_string(lteInfo->getMsgFlag());
            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msgFlag)!= packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
            lteInterLayerDelay = lteAirFrame->getDuration()+(NOW- packetFromUpperTimeStampsByInterfaceId[interfaceId][msgFlag]);
            listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(lteInterLayerDelay.dbl());
            listOfCriteriaByInterfaceId[interfaceId]->sentPacketsToLower++;
            computeEffectiveTransmissionRate(interfaceId, msg,  (lteAirFrame->getDuration()).dbl());
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgFlag);
        }
        listOfCriteriaByInterfaceId[interfaceId]->timeStamp=NOW;
    }

    if(comingSignal==droppedPacketsLteSignal)
    {
        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
        if(Utilities::checkLteCtrlInfo(lteInfo)){
            std::string msgFlag = to_string(lteInfo->getMsgFlag());
            simtime_t delay=(NOW- packetFromUpperTimeStampsByInterfaceId[interfaceId][msgFlag]);
            listOfCriteriaByInterfaceId[interfaceId]->delay.push_back(delay.dbl());
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgFlag);
        }
        listOfCriteriaByInterfaceId[interfaceId]->droppedPackets++;
    }

    double currentSuccessfulTransmissionRate= getCurrentInterfaceSuccessfulTransmissionRate(interfaceId);
    listOfCriteriaByInterfaceId[interfaceId]->successfulTransmissionRate.push_back(currentSuccessfulTransmissionRate);
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

    std::string interfaceName = Utilities::getInterfaceNameFromFullPath(source->getFullPath());
    int interfaceId=0;
    auto packet=dynamic_cast<cMessage*>(msg);
    std::string msgName=packet->getName();

    if(interfaceName.find("wlan")==0)
    {
        if((msgName.find("hetNets") == 0))
        {
          interfaceId = Utilities::extractNumber(interfaceName);
          recordStatsForWlan(signal,source->getName(),packet,interfaceId);
        }

    }else if(interfaceName.find("lteNic")==0)
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


void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName() << endl;
}

double CollectStats::getCurrentInterfaceSuccessfulTransmissionRate(int interfaceId) {
    double currentlySentPck = listOfCriteriaByInterfaceId[interfaceId]->sentPacketsToLower;
    double currentlyDroppedPck = listOfCriteriaByInterfaceId[interfaceId]->droppedPackets;
    return ((currentlySentPck - currentlyDroppedPck) / currentlySentPck)*100;
}




