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

#include "stack/mac/packet/LteMacPdu_m.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/phy/layer/LtePhyBase.h"


Define_Module(CollectStats);


void CollectStats::initialize(int stage)
{

    if (stage == inet::INITSTAGE_LOCAL) {
        interfaceToProtocolMapping = par("interfaceToProtocolMapping").stringValue();
        averageMethod = par("averageMethod").stringValue();

        tr0 = registerSignal("tr0");
        tr1 = registerSignal("tr1");

        delay0 = registerSignal("delay0");
        delay1 = registerSignal("delay1");

        cbr0 = registerSignal("cbr0");
        cbr1 = registerSignal("cbr1");

        gamma = par("gamma").intValue();
        sendInterval = par("sendPeriod").doubleValue();

        setInterfaceToProtocolMap();
        registerSignals();
        initializeDLT();
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER) {// this is to wait that lteInterfaceMacId_ be available
            if(getAncestorPar("lteInterfaceIsActive").boolValue()){
                const char* moduleName = getParentModule()->getFullName();
                lteInterfaceMacId_ = getBinder()->getMacNodeIdByModuleName(moduleName);
            }
        }
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
        dltByInterfaceIdByCriterion[x.first]["delayIndicator"]=getDLT(0);
        dltByInterfaceIdByCriterion[x.first]["throughputIndicator"]=getDLT(0);
        dltByInterfaceIdByCriterion[x.first]["reliabilityIndicator"]=getDLT(0);
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
        else if(x.second.find("LTE")!= std::string::npos)
        {
            if (getAncestorPar("lteInterfaceIsActive"))
            {
                std::string macModuleName = "^.lteNic.mac";

                cModule* module = getModuleByPath(macModuleName.c_str());
                LteMacBase * macModule=check_and_cast<LteMacBase *>(module);
                lteMacSentPacketToLowerLayerSingal =  macModule->registerSignal("sentPacketToLowerLayer");
                macModule->subscribe(lteMacSentPacketToLowerLayerSingal, this);

                std::string phyModuleName = "^.lteNic.phy";
                LtePhyBase * phyModule=check_and_cast<LtePhyBase *>(getModuleByPath(phyModuleName.c_str()));
                ltePhyRecievedAirFrameSingal=phyModule->registerSignal("receivedAirFrame");
                phyModule->subscribe(ltePhyRecievedAirFrameSingal, this);
            }
        }
    }
}




double CollectStats::getThroughputIndicator(int64_t dataLength, double transmitTime)
{
    return (double)dataLength/transmitTime;
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
        //TODO why negation with !
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
                reliabilityIndicator =
                        (double) std::get<1>(
                                attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])
                                / (double) std::get<0>(
                                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);

                recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator) ;

                //purge
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
                lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
                lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
            }
        }
}

void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{
    if (comingSignal == DecisionMaker::decisionSignal) {
        return;// from here we do not know the IDs of MAC PDUs sent to initialize utility variables.
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delayInidicator, throughputIndicator, reliabilityIndicator;
    double throughputMesureInterval = dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"];

    UserControlInfo *uInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
    bool isDataPacket = (uInfo->getFrameType() == DATAPKT) && (uInfo->getLcid() == SHORT_BSR); // the second condition is to ignore sent PDU sent just to ask grant. See LteMacUeD2D::macPduMake line 93.
    bool isAckOrNackPacket = uInfo->getFrameType() == HARQPKT ;

    if((comingSignal==lteMacSentPacketToLowerLayerSingal) && (isDataPacket)){

        LteMacPdu_Base *pduSent = dynamic_cast<LteMacPdu_Base*>(msg);

        bool isMulticastMessage = uInfo->getDestId() == lteInterfaceMacId_;  // by convention the node self mac id is set as the destId for multicast message. See LtePdcpRrcUeD2D::fromDataPort line 63.

        if (isMulticastMessage) { //broadcast case record stats

            //Delay metric
            delayInidicator = TTI;
            // Reliability metric
            reliabilityIndicator=1 ; // consider the maximum

            //Throughput metric
             std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += pduSent->getByteLength();
             throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

            recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator);

        }else{ //unicast traffic: initialize utility variables and wait for HARQ feedback to record stats.
              //this instruction block is equivalent to "when packet leave decision maker toward transmission interface" recordStatsForWlan

            string pduName = "pdu"+std::to_string(pduSent->getMacPduId());

            //For delay metric
            packetFromUpperTimeStampsByInterfaceId[interfaceId][pduName]=NOW; //TODO consider the eventual static time between the instant this data packet starts to be handled by the MAC layer and the instant of the creation the corresponding PDU.

            // For reliability metric
            std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(msg)->getByteLength();

            //utility
            lastTransmittedFramesByInterfaceId[interfaceId].insert({pduName,pduSent->dup()});
        }

    }else if ((comingSignal == ltePhyRecievedAirFrameSingal) && (isAckOrNackPacket)){

        LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);
        LteHarqFeedback *harqFeedback = check_and_cast<LteHarqFeedback*>(frame->getEncapsulatedPacket());

        if (harqFeedback->getResult()){ // H-ARQ feedback is ACK

            string pduName = "pdu"+std::to_string(harqFeedback->getFbMacPduId());

            if (packetFromUpperTimeStampsByInterfaceId[interfaceId].find(pduName) == packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                //This feedback is for a D2D_SHORT_BSR packet sent just to ask grant. See LteMacUeD2D::macPduMake line 93.
                return ;
            }
           simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][pduName];

           //retrieve last transmitted packet from its name
           std::map<string,cMessage*>::iterator messageIt = lastTransmittedFramesByInterfaceId[interfaceId].find(pduName);
           std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(messageIt->second)->getByteLength(); //To note that this packet length data has been successfully sent

           //Delay metric
           delayInidicator = SIMTIME_DBL(macDelay);

           //Throughput metric
            throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

            // Reliability metric
            reliabilityIndicator =
                    (double) std::get<1>(
                            attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])
                            / (double) std::get<0>(
                                    attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);

            recordStatTuple(interfaceId, delayInidicator, throughputIndicator,reliabilityIndicator);

            //purge
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(pduName);
            lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());

        } //else TODO : Can a H-ARQ process fails to send a MAC PDU ? If yes record stats at failure.
    }
}

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
    if (signal == DecisionMaker::decisionSignal) {

        if (interfaceToProtocolMap[value].find("WiFi") != std::string::npos) {
            recordStatsForWlan(signal, source->getName(),dynamic_cast<cMessage*>(details), value);

        }else if (interfaceToProtocolMap[value].find("LTE")!= std::string::npos) {
            recordStatsForLte(signal, dynamic_cast<cMessage*>(details), value);

        } else ASSERT2(false,"Can not find interface to protocol entry");
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

    } else if(interfaceName.find("lteNic")!= std::string::npos){

        std::vector<int> result;
        bool searchResult = Utilities::findKeyByValue(result, interfaceToProtocolMap, string("LTE"));
        if (searchResult) {
            recordStatsForLte(signal, packet, result.at(0));

        }else ASSERT2(false,"Can not find interface to protocol entry");
    }
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



