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

#include "../stats/CollectStats.h"

#include <inet/common/ModuleAccess.h>
//#include "stack/phy/packet/cbr_m.h"
#include <numeric>
#include<boost/lexical_cast.hpp>
#include "stack/pdcp_rrc/layer/LtePdcpRrcUeD2D.h"
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>

#include "../cbrMeasurement/util/ChannelLoadAccess.h"
#include "../decisionMaker/DecisionMaker.h"
#include "common/LteCommon.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"


Define_Module(CollectStats);
static const simsignal_t receivedPacketFromUpperLayerLteSignal = cComponent::registerSignal("receivedPacketFromUpperLayer");


void CollectStats::initialize()
{

    interfaceToProtocolMapping =par("interfaceToProtocolMapping").stringValue();
    averageMethod=par("averageMethod").stringValue();
    setInterfaceToProtocolMap();
    registerSignals();
    initializeDLT();

    tr0 =registerSignal("tr0");
    tr1 =registerSignal("tr1");

    delay0 =registerSignal("delay0");
    delay1 =registerSignal("delay1");

    cbr0 =registerSignal("cbr0");
    cbr1 =registerSignal("cbr1");


    gamma=par("gamma").intValue();
    sendInterval=par("sendPeriod").doubleValue();
}

void CollectStats::setInterfaceToProtocolMap()
{
    cStringTokenizer tokenizer(interfaceToProtocolMapping.c_str(),string(",").c_str());
      while (tokenizer.hasMoreTokens()){

          std::string mappingEntry= tokenizer.nextToken();
          vector<string> result;
          boost::split(result, mappingEntry, boost::is_any_of(":"));
          interfaceToProtocolMap.insert({stoi(result[0]),result[1]});
      }
}

void CollectStats::initializeDLT()
{
    for(auto const & x: interfaceToProtocolMap){
        dltByInterfaceIdByCriterion[x.first]["delayIndicator"]=0;
        dltByInterfaceIdByCriterion[x.first]["throughputIndicator"]=0;
        dltByInterfaceIdByCriterion[x.first]["reliabilityIndicator"]=0;
    }
}

void CollectStats::registerSignals()
{


    //Decider module signal
    subscribeToSignal<DecisionMaker>("^.decisionMaker",DecisionMaker::decisionSignal);


    //Decider transmission interface module signal
    for(auto const & x: interfaceToProtocolMap){

        if (x.second.find("WiFi") != std::string::npos) { //IEEE 802.11 case

            std::string macModuleName= "^.wlan["+ to_string(x.first) +"].mac";
            std::string radioModuleName= "^.wlan["+ to_string(x.first) +"].radio";

            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetSentToLowerSignal);

            //packet drop signals
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_PACKET_DROP);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_LINK_BREAK);
            //packet ack signals
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_PACKET_ACK_RECEIVED);
        }
//        else if(x.second=="mode4")
//        {
//            std::string pdcpRrcModuleName = "^.lteNic.pdcpRrc";
//            std::string phyModuleName = "^.lteNic.phy";
//            std::string macModuleName = "^.lteNic.mac";
//
//            bool mode4 = getAncestorPar("withMode4");
//            if (mode4)
//            {
//                    subscribeToSignal<LtePdcpRrcUeD2D>(pdcpRrcModuleName,receivedPacketFromUpperLayerLteSignal);
//                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::sentToLowerLayerSignal);
//            }
//        }

    }
}




double CollectStats::getThroughputIndicator(int64_t dataLength, double transmitTime)
{

    return dataLength/transmitTime;
}



void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{

    if ( comingSignal == DecisionMaker::decisionSignal){ //when packet leave decision maker toward transmission interface

        //For delay metric
        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;

        // For reliability metric
        std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(msg)->getByteLength();

        //utility
        lastTransmittedFramesByInterfaceId[interfaceId].insert({msg->getName(),msg});

        return ;
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delayInidicator, throughputIndicator, reliabilityIndicator;
    double throughputMesureInterval = dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"];

    //retrieve last transmitted packet from its name
    std::map<string,cMessage*>::iterator messageIt = lastTransmittedFramesByInterfaceId[interfaceId].find(msg->getName());

    if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //signal of packet coming out of the radio layer -> frame transmitter

        Ieee802Ctrl *controlInfo = dynamic_cast<Ieee802Ctrl*>(messageIt->second->getControlInfo());
        if (!controlInfo->getDestinationAddress().isMulticast()) { // record immediately statistics if transmitted frame do not require ACK

            //Delay metric
            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
            simtime_t macAndRadioDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]; //--> transmission duration already elapsed
            delayInidicator = SIMTIME_DBL(macAndRadioDelay);

            //Throughput metric
            std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(messageIt->second)->getByteLength();
            throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

            // Reliability metric
            reliabilityIndicator=1 ; // consider the maximum

            recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator);

            //purge
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
            lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
            lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
        }
        //else wait for ack or packet drop to record statistics


    } else if (comingSignal== NF_PACKET_DROP || comingSignal== NF_LINK_BREAK || comingSignal == NF_PACKET_ACK_RECEIVED) { //otherwise it is MAC error signal

            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
//            printMsg("Reading",msg);
            simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];


            if(macDelay == 0){ //case of packet drop due to queue overflow
                //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
                //so check the following assertion
                ASSERT(comingSignal == NF_PACKET_DROP);
                ASSERT2(false,"Packet drop due to queue overflow not supported yet"); //TODO add delay penalties to consider in case of packet drop


            }else { //case of packet drop due failing CSMA/CA process or previous ACK received

                //Delay metric
                delayInidicator = SIMTIME_DBL(macDelay);


                if (comingSignal == NF_PACKET_ACK_RECEIVED){
                    std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(messageIt->second)->getByteLength();
                }

                 //Throughput metric
                throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

                // Reliability metric
                reliabilityIndicator=std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])/std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);

                recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator) ;

                //purge
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
                lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
            }
        }
}

//void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
//{
//    double delay, availableBandwidth,cbr, queueVacancy;
//
//    if(listOfCriteriaByInterfaceId.find(interfaceId)== listOfCriteriaByInterfaceId.end())
//        listOfCriteriaByInterfaceId.insert({interfaceId,new listOfCriteria()});
//
//    if ( comingSignal ==receivedPacketFromUpperLayerLteSignal){ // When a packet enter to PDCP_RRC layer
//        std::string msgName=PK(msg)->getName();
//        if((msgName.find("hetNets")==0))
//        {
//            FlowControlInfoNonIp* lteInfo = check_and_cast<FlowControlInfoNonIp*>(PK(msg)->getControlInfo());
//            packetFromUpperTimeStampsByInterfaceId[interfaceId][to_string(lteInfo->getMsgFlag())]=NOW;
//        }
//    }
//
//
//    if ( comingSignal ==  LtePhyVUeMode4::sentToLowerLayerSignal) { // when the  packet comes to the Phy layer for transmission
//        LteAirFrame* lteAirFrame=dynamic_cast<LteAirFrame*>(msg);
//        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
//        if(Utilities::checkLteCtrlInfo(lteInfo)){
//            //delay
//            std::string msgFlag = to_string(lteInfo->getMsgFlag());
//            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msgFlag)!= packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
//            simtime_t lteInterLayerDelay = lteAirFrame->getDuration()+(NOW- packetFromUpperTimeStampsByInterfaceId[interfaceId][msgFlag]);
//            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgFlag);
//            delay = SIMTIME_DBL(lteInterLayerDelay);
//            // cbr
//            cbr=getLteCBR();
//            emit(cbr1,cbr);
//            availableBandwidth = getAvailableBandwidth(
//                    getCapacity(),
//                lteAirFrame->getDuration().dbl(), cbr);
//
//            // buffer vacancy
//            queueVacancy=extractLteBufferVacancy();
//            recordStatTuple(interfaceId,delay,availableBandwidth,queueVacancy) ;
//        }
//    }
//}



void CollectStats::updateDLT(listOfCriteria* list, int interfaceId)
{
    // update dtl according to coefficient of variation
    dltByInterfaceIdByCriterion[interfaceId]["delayIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->delayIndicator)));
    dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->throughputIndicator)));
    dltByInterfaceIdByCriterion[interfaceId]["reliabilityIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->reliabilityIndicator)));
}

double CollectStats::getDLT(double CofficientOfVariation) {
    return exp(-1 * CofficientOfVariation + log(gamma * sendInterval));
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

CollectStats::listOfCriteria* CollectStats::getSublistByDLT(int interfaceId) {

    listOfCriteria* rList = new listOfCriteria();

    rList->delayIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->delayIndicator,dltByInterfaceIdByCriterion[interfaceId]["delayIndicator"] );
    rList->throughputIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->throughputIndicator,dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"] );
    rList->reliabilityIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->reliabilityIndicator,dltByInterfaceIdByCriterion[interfaceId]["reliabilityIndicator"] );

    //update DLT
    updateDLT(rList, interfaceId);

    //purge stats history
    delete listOfCriteriaByInterfaceId[interfaceId]->delayIndicator;
    delete listOfCriteriaByInterfaceId[interfaceId]->throughputIndicator;
    delete listOfCriteriaByInterfaceId[interfaceId]->reliabilityIndicator;
    delete listOfCriteriaByInterfaceId[interfaceId];

    std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])=0;
    std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])=0;

    listOfCriteriaByInterfaceId[interfaceId]=rList;

    return rList;
}

map<simtime_t,double>* CollectStats::getSublistByDLTOfCriterion(map<simtime_t,double>* pCriterion,double pDlt){

    map<simtime_t,double>* rMap = new map<simtime_t,double>();

    vector <simtime_t>* timeStamps = Utilities::retrieveKeys(pCriterion);
    int statIndex;
    int recentStatIndex = timeStamps->size() - 1;
    simtime_t historyBound = NOW - SimTime(pDlt);

    for (statIndex = recentStatIndex; statIndex >= 0; statIndex--) {
        simtime_t statRecordDate = (*timeStamps)[statIndex] ;
        if (statRecordDate >= historyBound)
           rMap->insert({statRecordDate,(*pCriterion)[statRecordDate]});
        else
            break;
    }

    if (statIndex == recentStatIndex){// no available stat in interval [Now-pHistoryBound, Now[
        simtime_t recentOfOldStatsDate = (*timeStamps)[recentStatIndex] ;
        rMap->insert({recentOfOldStatsDate,(*pCriterion)[recentOfOldStatsDate]});
    }

    return rMap ;
}


CollectStats::listAlternativeAttributes* CollectStats::applyAverageMethod(map<int,listOfCriteria*> dataSet)
{
    listAlternativeAttributes * myList = new listAlternativeAttributes();


    if(averageMethod==string("simple"))
    {
        for (auto& x : dataSet)
        {
            alternativeAttributes* listAttr=new alternativeAttributes();
            listAttr->delayIndicator=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->delayIndicator));
            listAttr->throughputIndicator=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->throughputIndicator));
            listAttr->reliabilityIndicator=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->reliabilityIndicator));
            myList->data.insert({x.first,listAttr});
        }
    }else if(averageMethod==string("ema"))
    {
        for (auto& x : dataSet)
        {
            std::vector<double> vEMADelay;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->delayIndicator),vEMADelay);
            std::vector<double> vEMATransmissionRate;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->throughputIndicator),vEMATransmissionRate);
            std::vector<double> vEMAreliability;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->reliabilityIndicator),vEMAreliability);
            alternativeAttributes* listAttr=new alternativeAttributes();
            listAttr->delayIndicator= vEMADelay.back() ;
            listAttr->throughputIndicator=vEMATransmissionRate.back();
            listAttr->reliabilityIndicator=vEMAreliability.back();
            myList->data.insert({x.first,listAttr});
        }
    }
    return myList;
}

CollectStats::listAlternativeAttributes* CollectStats::prepareDummyNetAttributes()
{
    listAlternativeAttributes * myList = new listAlternativeAttributes();
    for (auto& x : listOfCriteriaByInterfaceId)
    {
        alternativeAttributes* listAttr=new alternativeAttributes();
        vector<double>* d=Utilities::retrieveValues(x.second->delayIndicator);
        vector<double>* avb=Utilities::retrieveValues(x.second->throughputIndicator);
        vector<double>* qc=Utilities::retrieveValues(x.second->reliabilityIndicator);
        listAttr->delayIndicator=d->back();
        listAttr->throughputIndicator=avb->back();
        listAttr->reliabilityIndicator=qc->back();
        myList->data.insert({x.first,listAttr});
    }
    return myList;
}

CollectStats::listAlternativeAttributes* CollectStats::prepareNetAttributes()
{
    // 1- get Data until NOW -DLT
    map<int,listOfCriteria*> dataSet= getSublistByDLT();
    // 2- Apply average method
    listAlternativeAttributes* a=applyAverageMethod(dataSet);
    return a;

}


void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, long value,cObject *details)
{
    if(signal==DecisionMaker::decisionSignal){
        if(interfaceToProtocolMap[value].find("WiFi")!= std::string::npos){
            recordStatsForWlan(signal,source->getName(),dynamic_cast<cMessage*>(details),value);
        }
    }

}
void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{
    std::string interfaceName = Utilities::getInterfaceNameFromFullPath(source->getFullPath());
    int interfaceId=0;
    auto packet=dynamic_cast<cMessage*>(msg);
    std::string msgName=packet->getName();

    if(interfaceName.find("wlan")!= std::string::npos)
    {
        if((msgName.find("hetNets") != std::string::npos))
        {
          interfaceId = Utilities::extractNumber(interfaceName);
          recordStatsForWlan(signal,source->getName(),packet,interfaceId);
        }

        return;
    }
    ASSERT (false);
//    else if(interfaceName.find("lteNic")==0)
//    {
//
//        std::vector<int> result;
//        bool searchResult=Utilities::findKeyByValue(result, interfaceToProtocolMap, string("mode4"));
//        if(searchResult)
//        {
//            interfaceId=result.at(0);
//            recordStatsForLte(signal, packet, interfaceId);
//
//        }
//       }


}


void CollectStats::printMsg(std::string type, cMessage*  msg)
{
    std::cout<< simTime()<< ", "<< type  <<" id=" << msg->getTreeId()  << " ,tree id= " << msg->getTreeId()<<", Msg name=" << msg->getName()
                  << ", Class Name=" << msg->getClassName()
                  << ", Owner=" << msg->getOwner()->getName() << endl;
}


void CollectStats::recordStatTuple(int interfaceId, double delay, double transmissionRate, double reliability){

    insertStatTuple(listOfCriteriaByInterfaceId[interfaceId], NOW ,delay, transmissionRate, reliability) ;
    if(interfaceId==0){
        emit(tr0,transmissionRate);
        emit(delay0,delay);
    }
    else{
        emit(tr1,transmissionRate);
        emit(delay1,delay);
    }

}




void CollectStats::insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delay, double availableBandwitdth, double reliability){

    // check and initialize list if necessary
    if(!list->delayIndicator)
        list->delayIndicator= new map<simtime_t,double>();
    if(!list->throughputIndicator)
        list->throughputIndicator = new  map<simtime_t,double>();
    if(!list->reliabilityIndicator)
        list->reliabilityIndicator= new  map<simtime_t,double>();


    list->delayIndicator->insert({timestamp,delay});
    list->throughputIndicator->insert({timestamp,availableBandwitdth});
    list->reliabilityIndicator->insert({timestamp,reliability});
}



