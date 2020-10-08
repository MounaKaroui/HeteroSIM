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
#include "stack/mac/packet/BufferOccupancyIndication_m.h"
#include <numeric>
#include "../../Modules/DecisionMaker/DecisionMaker.h"
#include<boost/lexical_cast.hpp>
#include "stack/pdcp_rrc/layer/LtePdcpRrcUeD2D.h"
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include "common/LteCommon.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"


Define_Module(CollectStats);
static const simsignal_t receivedPacketFromUpperLayerLteSignal = cComponent::registerSignal("receivedPacketFromUpperLayer");


void CollectStats::initialize()
{

    interfaceToProtocolMapping =par("interfaceToProtocolMapping").stringValue();
    dlt=par("dlt").doubleValue();
    averageMethod=par("averageMethod").stringValue();
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
            std::string phyModuleName = "^.lteNic.phy";
            std::string macModuleName = "^.lteNic.mac";

            bool mode4 = getAncestorPar("withMode4");
            if (mode4)
            {
                    subscribeToSignal<LtePdcpRrcUeD2D>(pdcpRrcModuleName,receivedPacketFromUpperLayerLteSignal);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::sentToLowerLayerSignal);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::cbrSignal);
                    subscribeToSignal<LteMacVUeMode4>(macModuleName,LteMacVUeMode4::bufferOccupancySignal);
            }
        }

    }
}






void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{

    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  sourceName==string("mac")){ //when packet enter to MAC layer
        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;
        printMsg("Inserting",msg);
        return ;
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delay, transmissionRate, cbr, queueVacancy;

    if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //when the packet comes out of the radio layer --> transmission duration already elapsed

        //Delay
        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
        printMsg("Reading",msg);
        simtime_t macAndRadioDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];
        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
        delay = SIMTIME_DBL(macAndRadioDelay);

        //Transmission rate
        RadioFrame *radioFrame = check_and_cast<RadioFrame *>(msg);
        ASSERT(radioFrame && radioFrame->getDuration() != 0) ;
        transmissionRate = getTransmissionRate((PK(msg)->getBitLength()),(radioFrame->getDuration()).dbl());


    } else if (comingSignal== NF_PACKET_DROP || comingSignal== NF_LINK_BREAK || comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal) // packet drop related calculations
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

                //Delay
                delay = SIMTIME_DBL(macDelay);
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());

                //Transmission rate
                transmissionRate = getTransmissionRate(0,delay); //consider 0 bits are transmitted


            }
        }

    using namespace inet::ieee80211;
    if(comingSignal==Hcf::queueVacancySignal || comingSignal==CSMA::queueVacancyCSMASignal)
    {
        if (strcmp(msg->getName(), "QueueVacancyIndication") == 0)
        {
            QueueVacancyIndication*  queueVacancyMsg=dynamic_cast<QueueVacancyIndication*>(msg);
            queueVacancy=queueVacancyMsg->getValue();
        }
    }
    recordStatTuple(interfaceId, delay, transmissionRate,cbr, queueVacancy) ;
}




void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{

    double delay, transmissionRate,cbr, queueVacancy;

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
        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
        if(Utilities::checkLteCtrlInfo(lteInfo)){
            //delay
            std::string msgFlag = to_string(lteInfo->getMsgFlag());
            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msgFlag)!= packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
            simtime_t lteInterLayerDelay = lteAirFrame->getDuration()+(NOW- packetFromUpperTimeStampsByInterfaceId[interfaceId][msgFlag]);
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgFlag);
            delay = SIMTIME_DBL(lteInterLayerDelay);

            //Transmission rate
            transmissionRate = getTransmissionRate((PK(msg)->getBitLength()),  (lteAirFrame->getDuration()).dbl());
        }
    }

    if(comingSignal==LtePhyVUeMode4::cbrSignal)
    {
        if (strcmp(msg->getName(), "CBR") == 0)
        {
            Cbr* cbrPkt=dynamic_cast<Cbr*>(msg);
            cbr=cbrPkt->getCbr();
        }
    }

    if(comingSignal==LteMacVUeMode4::bufferOccupancySignal)
    {
        if (strcmp(msg->getName(), "BufferOccupancyMsg") == 0)
        {
            BufferOccupancyIndication* bufferPkt=dynamic_cast<BufferOccupancyIndication*>(msg);
            queueVacancy=bufferPkt->getBufferVacancy();
        }

    }

    recordStatTuple(interfaceId,delay,transmissionRate,cbr,queueVacancy) ;


}





double CollectStats::updateDLT(double x)
{
    return exp(-1*x); // Fixme : adjust the function to consider or not dtlMax
}

std::string CollectStats::convertListOfCriteriaToString(listAlternativeAttributes listOfAlternativeAttributes)
{

    std::string pathsCriteriaValues = "";
    std::vector<std::string> criteriaStr;
    std::string critValuesPerPathStr = "";
    std::string alternativeAttributesStr="";

    for (auto& x : listOfAlternativeAttributes.data)
    {
        criteriaStr.push_back( boost::lexical_cast<std::string>(x.second->transmissionRate));

        criteriaStr.push_back( boost::lexical_cast<std::string>(x.second->delay));

    }

    for (unsigned int a = 0; a < criteriaStr.size(); ++a) {
        if (a == 0) {
            pathsCriteriaValues = pathsCriteriaValues + criteriaStr[a];
        } else {
            pathsCriteriaValues = pathsCriteriaValues + ","
                    + criteriaStr[a];
        }}
    alternativeAttributesStr = alternativeAttributesStr + pathsCriteriaValues
            + ",";
    criteriaStr.clear();

    return alternativeAttributesStr;
}


map<int,CollectStats::listOfCriteria*> CollectStats::getSublistByDLT()
{
    map<int,CollectStats::listOfCriteria*> rMap;

    for (const auto &pair : listOfCriteriaByInterfaceId){
        int interfaceId = pair.first;
       rMap.insert({interfaceId, getSublistByDLT(interfaceId)}) ;
    }

    return rMap;
}

CollectStats::listOfCriteria* CollectStats::getSublistByDLT(int interfaceId){

    listOfCriteria* rList = new listOfCriteria();
    listOfCriteria* tmp = listOfCriteriaByInterfaceId[interfaceId];

    simtime_t historyBound = NOW - (SimTime(dlt));

    int recentStatIndex ;
    for(recentStatIndex=tmp->timeStamp.size()-1; recentStatIndex>=0; recentStatIndex--){

        if(tmp->timeStamp[recentStatIndex]<= historyBound)
            insertStatTuple(rList,tmp->timeStamp[recentStatIndex], tmp->delay[recentStatIndex], tmp->transmissionRate[recentStatIndex],tmp->cbr[recentStatIndex],tmp->queueVacancy[recentStatIndex]) ;
        else
            break ;
    }

    if (recentStatIndex<0)
        throw cRuntimeError("No available stats in DLT interval"); //TODO handle

    return rList ;
}


CollectStats::listAlternativeAttributes CollectStats::applyAverageMethod(map<int,listOfCriteria*> dataSet)
{
    listAlternativeAttributes myList;
    if(averageMethod==string("simple"))
    {
        for (auto& x : dataSet)
        {
            myList.data.at(x.first)->delay=Utilities::calculateMeanVec(x.second->delay);
            myList.data.at(x.first)->transmissionRate=Utilities::calculateMeanVec(x.second->transmissionRate);
            myList.data.at(x.first)->cbr=Utilities::calculateMeanVec(x.second->cbr);
            myList.data.at(x.first)->queueVacancy=Utilities::calculateMeanVec(x.second->queueVacancy);
        }
    }else if(averageMethod==string("ema"))
    {
        std::vector<double> emaOfDelay;
        std::vector<double> emaOfTransmissionRate;
        std::vector<double> emaOfsuccessfulTransmissionRatio;
        std::vector<double> emaOfcbr;
        std::vector<double> emaOfQueueVacancy;
        for (auto& x : dataSet)
        {
            myList.data.at(x.first)->delay=Utilities::calculateEMA(x.second->delay,emaOfDelay);
            myList.data.at(x.first)->transmissionRate=Utilities::calculateEMA(x.second->transmissionRate, emaOfTransmissionRate);
            myList.data.at(x.first)->cbr=Utilities::calculateEMA(x.second->cbr, emaOfcbr);
            myList.data.at(x.first)->queueVacancy=Utilities::calculateEMA(x.second->queueVacancy, emaOfQueueVacancy);
        }
    }
    return myList;
}

std::string CollectStats::prepareNetAttributes()
{
    // 1- get Data until NOW -DLT
    map<int,listOfCriteria*> dataSet= getSublistByDLT();
    // 2- Apply average method
    listAlternativeAttributes a=applyAverageMethod(dataSet);
    // 3- convert to string
    return convertListOfCriteriaToString(a);
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


}


void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName() << endl;
}

double CollectStats::getTransmissionRate(int64_t dataLength, double sendInterval)
{
    return dataLength/sendInterval; // bps
}


void CollectStats::recordStatTuple(int interfaceId, double delay, double transmissionRate, double cbr, double queueVacancy){

    insertStatTuple(listOfCriteriaByInterfaceId[interfaceId], NOW ,delay, transmissionRate, cbr, queueVacancy) ;
}

void CollectStats::insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delay, double transmissionRate, double cbr, double queueVacancy){
    list->timeStamp.push_back(timestamp);
    list->delay.push_back(delay);
    list->transmissionRate.push_back(transmissionRate) ;
    list->cbr.push_back(cbr) ;
    list->queueVacancy.push_back(queueVacancy) ;

}


