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
#include "stack/mac/packet/LteRac_m.h"


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
        setSendIntervals(par("initialSendIntervalByInterfaceId").stringValue());

        setInterfaceToProtocolMap();
        DecisionMaker *decisionModule =dynamic_cast<DecisionMaker*>(getParentModule()->getSubmodule("decisionMaker"));
        if (decisionModule->isDeciderActived()) {
            registerSignals();
            initializeDLT();
        }

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

void CollectStats::setSendIntervals(std::string strValues)
{
    cStringTokenizer tokenizer(strValues.c_str(),string(",").c_str());
      while (tokenizer.hasMoreTokens()){

          std::string mappingEntry= tokenizer.nextToken();
          vector<string> result;
          boost::split(result, mappingEntry, boost::is_any_of(":"));
          sendIntervalByInterfaceId.insert({stoi(result[0]),stod(result[0])});
          sendIntervalLastUpdateTimestampByInterfaceId.insert({stoi(result[0]),NOW});
      }
}

void CollectStats::updateSendInterval(int interfaceId)
{
   std::map<int, simtime_t>::iterator it1 = sendIntervalLastUpdateTimestampByInterfaceId.find(interfaceId);
   std::map<int, double>::iterator it2 = sendIntervalByInterfaceId.find(interfaceId);
   it2->second = (NOW - it1->second).dbl();
   it1->second = NOW;
}

void CollectStats::initializeDLT()
{
    for(auto const & x: interfaceToProtocolMap){
        dltByInterfaceIdByCriterion[x.first]["delayIndicator"]=getDLT(0,x.first);
        dltByInterfaceIdByCriterion[x.first]["throughputIndicator"]=getDLT(0,x.first);
        dltByInterfaceIdByCriterion[x.first]["reliabilityIndicator"]=getDLT(0,x.first);
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

        updateSendInterval(interfaceId);
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

            //This happens when the decision maker has already taken his decision meanwhile. So this ack is no logger needed.
            if(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                return ; //ignore it
            }

            simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];


            if(macDelay == 0){ //case of packet drop due to queue overflow
                //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
                //so check the following assertion
                ASSERT(comingSignal == NF_PACKET_DROP);
                ASSERT2(false,"Packet drop due to queue overflow not supported yet"); //TODO add delay penalties to consider in case of packet drop


            }else { //case of packet drop due failing CSMA/CA process or previous ACK received

                //This happens when the decision maker has already taken his decision meanwhile. So this ack is no logger needed.
                if(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                    return ; //ignore it
                }

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
        updateSendInterval(interfaceId);
        return;// from here we do not know the IDs of MAC PDUs sent to initialize utility variables.
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delayInidicator, throughputIndicator, reliabilityIndicator;
    double throughputMesureInterval = dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"];

    UserControlInfo *uInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());

    if((comingSignal==lteMacSentPacketToLowerLayerSingal)){

        if(uInfo->getFrameType() == RACPKT){
            if (getFullPath() == "SimpleNetwork2.node[15].collectStatistics") {
                EV_DEBUG << " A breakpoint here";
            }
            lastRacSendTimestamp = NOW;
            return ;
        }

        if (uInfo->getFrameType() == DATAPKT){
            LteMacPdu_Base *pduSent = dynamic_cast<LteMacPdu_Base*>(msg);

            if (getFullPath() == "SimpleNetwork2.node[21].collectStatistics") {
                EV_DEBUG << " A breakpoint here";
            }

            bool isMulticastMessage = uInfo->getDestId() == lteInterfaceMacId_;  // by convention the node self mac id is set as the destId for multicast message. See LtePdcpRrcUeD2D::fromDataPort line 63.

            if (!isMulticastMessage) { //unicast traffic: initialize utility variables and wait for HARQ feedback to record stats.
                  //this instruction block is equivalent to "when packet leave decision maker toward transmission interface" recordStatsForWlan

                string pktName;

                if(uInfo->getLcid() == D2D_SHORT_BSR){// RSR report case. (See in LteMacUeD2D::macPduMake)
                    pktName = "BSR-pdu-"+std::to_string(pduSent->getMacPduId());

                }else if(uInfo->getLcid() == SHORT_BSR){
                    pktName = "data-pdu-"+std::to_string(pduSent->getMacPduId());

                }else throw cRuntimeError("Unknown LCID type");


                //Check if this PDU is note already timestamped. This may happen in  case of retransmission if it
                if (packetFromUpperTimeStampsByInterfaceId[interfaceId].find(pktName) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                    return ; //If yes this is a retransmission of it, ignore it.
                }

                //For delay metric
                 packetFromUpperTimeStampsByInterfaceId[interfaceId][pktName]=NOW;

                if(uInfo->getLcid() == SHORT_BSR){// Data MAC PDU report case. (See in LteMacUeD2D::macPduMake)

                    // For reliability metric
                    std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(msg)->getByteLength();

                    //utility
                    lastTransmittedFramesByInterfaceId[interfaceId].insert({pktName,pduSent->dup()});
                }

            } //TODO move here code for broadcast/multicast traffic metric assumptions
        }

    }else if ((comingSignal == ltePhyRecievedAirFrameSingal)){

        LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);

        if (uInfo->getFrameType() == RACPKT) { //RAC packet case

            LteRac *racPkt = check_and_cast<LteRac*>(frame->getEncapsulatedPacket());
            if (getFullPath() == "SimpleNetwork2.node[15].collectStatistics") {
                EV_DEBUG << " A breakpoint here";
            }
            if (racPkt->getSuccess()){

                lastRacReceptionTimestamp = NOW;
            }

            return;
        }

        if ( uInfo->getFrameType() == HARQPKT){ //it is ACK Or NACK Packet of a BSR or DATA packet

            LteHarqFeedback *harqFeedback = check_and_cast<LteHarqFeedback*>(frame->getEncapsulatedPacket());

            if (getFullPath() == "SimpleNetwork2.node[21].collectStatistics") {
                EV_DEBUG << " A breakpoint here" <<harqFeedback->getResult();
            }

            if (harqFeedback->getResult()){ // H-ARQ feedback is ACK

                string pktSuffixName = "pdu-"+std::to_string(harqFeedback->getFbMacPduId());
                map<string,simtime_t>::iterator timeStampIt ;

                //This KLUDGE is because harqFeedback's uInfo->getLcid() contains always the default value. Otherwise we do not know if this feedback is for a Data or BSR PDU. TODO fix this in eNodeB's feedback packet creation.
                bool dataEntryfound = (timeStampIt=packetFromUpperTimeStampsByInterfaceId[interfaceId].find("data-"+pktSuffixName)) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end();
                bool bsrEntryfound  = packetFromUpperTimeStampsByInterfaceId[interfaceId].find("BSR-"+pktSuffixName) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end();

                //This condition is only verified if the entry are removed because the decision maker has already taken his decision meanwhile.
                if (!dataEntryfound && ! bsrEntryfound){
                    return ; // So the feedback is considered as obsolete.  Ignore it.

                }else if (bsrEntryfound){ //check if the feedback is for a BSR
                    lastMacGrantObtentionDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][pktSuffixName];

                    packetFromUpperTimeStampsByInterfaceId[interfaceId].erase("BSR-"+pktSuffixName);
                    return ; // wait for the next DATA PDU to account this delay
                }

                simtime_t lastMacRACDelay = lastRacReceptionTimestamp - lastRacSendTimestamp;
                simtime_t macDataPduHarqProcessTime = NOW - timeStampIt->second;

                //Delay metric
                delayInidicator = SIMTIME_DBL(TTI+lastMacRACDelay+TTI+lastMacGrantObtentionDelay+TTI+macDataPduHarqProcessTime);

                //retrieve last transmitted packet from its name
                std::map<string, cMessage*>::iterator messageIt = lastTransmittedFramesByInterfaceId[interfaceId].find(timeStampIt->first);

                std::get<1>(
                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) +=
                        PK(messageIt->second) ->getByteLength(); //To note that this packet length data has been successfully sent

                //Throughput metric
                throughputIndicator =
                        getThroughputIndicator(
                                std::get<1>(
                                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]),
                                throughputMesureInterval);

                // Reliability metric
                reliabilityIndicator =
                        (double) std::get<1>(
                                attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])
                                / (double) std::get<0>(
                                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);

                recordStatTuple(interfaceId, delayInidicator,
                        throughputIndicator, reliabilityIndicator);


                //purge
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(timeStampIt);
                lastTransmittedFramesByInterfaceId[interfaceId].erase(messageIt);

            } //else TODO : Can a H-ARQ process fails to send a MAC PDU ? If yes record stats at failure.
        }

    }
}

void CollectStats::updateDLT(listOfCriteria* list, int interfaceId)
{
    // update dtl according to coefficient of variation
    dltByInterfaceIdByCriterion[interfaceId]["delayIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->delayIndicator)),interfaceId);
    dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->throughputIndicator)),interfaceId);
    dltByInterfaceIdByCriterion[interfaceId]["reliabilityIndicator"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->reliabilityIndicator)),interfaceId);
}

double CollectStats::getDLT(double CofficientOfVariation,int interfaceId) {
    return exp(-1 * CofficientOfVariation + log(gamma * sendIntervalByInterfaceId[interfaceId]));
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

    //Check if stats of "interfaceId" are recorded
    if(listOfCriteriaByInterfaceId[interfaceId]->delayIndicator && listOfCriteriaByInterfaceId[interfaceId]->throughputIndicator && listOfCriteriaByInterfaceId[interfaceId]->reliabilityIndicator ){

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

    }else{ //This is the case where the feedback for recording stats of "interfaceId" are not know yet
        insertStatTuple(rList, NOW, DBL_MAX, 0, 0); //consider these penalties
    }

    //purge
    std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])=0;
    std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId])=0;
    //This is to ignore the statistics of all packets whose ack has not yet been received.
    packetFromUpperTimeStampsByInterfaceId[interfaceId].clear(); //
    lastTransmittedFramesByInterfaceId[interfaceId].clear();

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


void CollectStats::recordStatTuple(int interfaceId, double delay, double transmissionRate, double reliability){
    DecisionMaker* decisionModule = dynamic_cast<DecisionMaker*>(getParentModule()->getSubmodule("decisionMaker"));
    if(!decisionModule->isNaiveSingleCriterionBasedDecision())
        insertStatTuple(listOfCriteriaByInterfaceId[interfaceId], NOW ,delay, transmissionRate, reliability) ;
    else
        insertStatTuple(interfaceId, NOW ,delay, transmissionRate, reliability) ;


    if(interfaceId==0){
        emit(tr0,transmissionRate);
        emit(delay0,delay);
    }
    else{
        emit(tr1,transmissionRate);
        emit(delay1,delay);
    }


}

void CollectStats::insertStatTuple(int interfaceId, simtime_t timestamp, double delayIndicator, double throughputIndicator, double reliabilityIndicator){

    // check and initialize map if necessary
    if(recentCriteriaStatsByInterfaceId.data.find(interfaceId)== recentCriteriaStatsByInterfaceId.data.end() )
        recentCriteriaStatsByInterfaceId.data.insert({interfaceId, new alternativeAttributes()});

    recentCriteriaStatsByInterfaceId.data[interfaceId]->delayIndicator=delayIndicator;
    recentCriteriaStatsByInterfaceId.data[interfaceId]->throughputIndicator=throughputIndicator;
    recentCriteriaStatsByInterfaceId.data[interfaceId]->reliabilityIndicator=reliabilityIndicator;
}

void CollectStats::insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delayIndicator, double throughputIndicator, double reliabilityIndicator){

    // check and initialize list if necessary
    if(!list->delayIndicator)
        list->delayIndicator= new map<simtime_t,double>();
    if(!list->throughputIndicator)
        list->throughputIndicator = new  map<simtime_t,double>();
    if(!list->reliabilityIndicator)
        list->reliabilityIndicator= new  map<simtime_t,double>();


    list->delayIndicator->insert({timestamp,delayIndicator});
    list->throughputIndicator->insert({timestamp,throughputIndicator});
    list->reliabilityIndicator->insert({timestamp,reliabilityIndicator});
}

void CollectStats::finish(){

    //remove undisposed LTE PDUs;
    for(const auto &pair1 : interfaceToProtocolMap){

            if(pair1.second.find("LTE") != std::string::npos){

            int lteInterfaceId = pair1.first;
            for (const auto &pair2 : lastTransmittedFramesByInterfaceId[lteInterfaceId])
                delete pair2.second ;

            break; // leave looping since more than one LTE interface in not supported yet
        }
    }

}


