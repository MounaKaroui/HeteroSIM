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
        controlTrafficSendInterval = par("controlTrafficSendInterval").doubleValue();
        minNumOfControlTrafficPktInDLT= par("minNumOfControlTrafficPktInDLT").intValue();

        throughputIndicator0Signal = registerSignal("throughputIndicator0");
        throughputIndicator1Signal = registerSignal("throughputIndicator1");

        delayIndicator0Signal = registerSignal("delayIndicator0");
        delayIndicator1Signal = registerSignal("delayIndicator1");

        reliabilityIndicator0Signal = registerSignal("reliabilityIndicator0");
        reliabilityIndicator1Signal = registerSignal("reliabilityIndicator1");

        setInterfaceToProtocolMap();
        DecisionMaker *decisionModule =dynamic_cast<DecisionMaker*>(getParentModule()->getSubmodule("decisionMaker"));
        if (decisionModule->isDeciderActived()) {
            registerSignals();
            setCommonDltMax(par("commonDLTMaxByInterfaceId").stringValue());
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

void CollectStats::setCommonDltMax(std::string strValues)
{
    cStringTokenizer tokenizer(strValues.c_str(),string(",").c_str());
      while (tokenizer.hasMoreTokens()){

          std::string mappingEntry= tokenizer.nextToken();
          vector<string> result;
          boost::split(result, mappingEntry, boost::is_any_of(":"));
          commonDLTMaxByInterfaceId.insert({stoi(result[0]),stod(result[1])});
      }
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




double CollectStats::getThroughputIndicator(int64_t dataBitLength, double transmitTime)
{
    if(isnan(transmitTime) || isinf(transmitTime) || transmitTime ==0 || dataBitLength<0)
        throw cRuntimeError("Incorrect input parameter(s) to calculate throughput: transmit it is %d and data length is %d",transmitTime,dataBitLength);
    return ((double)dataBitLength/transmitTime)*0.000001; //times *0.000001 to convert from bps to mbps
}


void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{

    if ( comingSignal == DecisionMaker::decisionSignal){ //when packet leave decision maker toward transmission interface

        //For delay metric
        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;

        // For reliability metric
        std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(msg)->getBitLength();

        //utility
        lastTransmittedFramesLengthByInterfaceId[interfaceId].insert({msg->getName(),PK(msg)->getBitLength()});

        return ;
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delayInidicator, throughputIndicator, reliabilityIndicator;
    double throughputMesureInterval = dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"];

    //retrieve info of last transmitted packet from its name
    std::map<string,int64_t>::iterator messageIt = lastTransmittedFramesLengthByInterfaceId[interfaceId].find(msg->getName());

    if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //signal of packet coming out of the radio layer -> frame transmitter

        Ieee802Ctrl *controlInfo = dynamic_cast<Ieee802Ctrl*>(msg->getControlInfo());
        //TODO why negation with !
       if (!controlInfo->getDestinationAddress().isMulticast()) { // record immediately statistics if transmitted frame do not require ACK

            //Delay metric
            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
            simtime_t macAndRadioDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]; //--> transmission duration already elapsed
            delayInidicator = SIMTIME_DBL(macAndRadioDelay);

            //Throughput metric
            std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += messageIt->second;
            throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

            // Reliability metric
            reliabilityIndicator=1 ; // consider the maximum

            recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator);

            //purge
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
            lastTransmittedFramesLengthByInterfaceId[interfaceId].erase(msg->getName());
        }
        //else wait for ack or packet drop to record statistics


    } else if (comingSignal== NF_PACKET_DROP || comingSignal== NF_LINK_BREAK || comingSignal == NF_PACKET_ACK_RECEIVED) { //otherwise it is MAC error signal

            //This happens when the instant at which the packet has been sent is no logger in current DLT. So, statistics related to it should be ignored.
            if(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) == packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
               return;
            }

            simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];


            if(macDelay == 0){ //case of packet drop due to queue overflow
                //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
                //so check the following assertion
                ASSERT(comingSignal == NF_PACKET_DROP);
                ASSERT2(false,"Packet drop due to queue overflow not supported yet"); //TODO add delay penalties to consider in case of packet drop


            }else { //case of packet drop due failing CSMA/CA process or previous ACK received

               //If this packet drop is due to failing CSMA/CA process, the MAC module emit successfully NF_PACKET_DROP and NF_LINK_BREAK signals
                if(comingSignal == NF_PACKET_DROP)
                    return ; // so this is to ignore one of them

                //Delay metric
                delayInidicator = SIMTIME_DBL(macDelay);


                if (comingSignal == NF_PACKET_ACK_RECEIVED){
                    std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += messageIt->second;
                    lastTransmittedFramesAckByInterfaceId[interfaceId].insert({msg->getName(),true});
                }

                 //Throughput metric
                throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]), throughputMesureInterval);

                // Reliability metric
                double numerator = (double) std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) ;
                double denominator =(double) std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);
                reliabilityIndicator = denominator==0 ? 0 : numerator/denominator ;

                recordStatTuple(interfaceId, delayInidicator, throughputIndicator, reliabilityIndicator) ;
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

    if((comingSignal==lteMacSentPacketToLowerLayerSingal)){

        if(uInfo->getFrameType() == RACPKT){
            lastRacSendTimestamp = NOW;
            return ;
        }

        if (uInfo->getFrameType() == DATAPKT){
            LteMacPdu_Base *pduSent = dynamic_cast<LteMacPdu_Base*>(msg);

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
                    std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) += PK(msg)->getBitLength();

                    //utility
                    lastTransmittedFramesLengthByInterfaceId[interfaceId].insert({pktName,PK(msg)->getBitLength()});
                }

            } //TODO move here code for broadcast/multicast traffic metric assumptions
        }

    }else if ((comingSignal == ltePhyRecievedAirFrameSingal)){

        LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);

        if (uInfo->getFrameType() == RACPKT) { //RAC packet case

            LteRac *racPkt = check_and_cast<LteRac*>(frame->getEncapsulatedPacket());
            if (racPkt->getSuccess()){
                lastRacReceptionTimestamp = NOW;
            }

            return;
        }

        if ( uInfo->getFrameType() == HARQPKT){ //it is ACK Or NACK Packet of a BSR or DATA packet

            LteHarqFeedback *harqFeedback = check_and_cast<LteHarqFeedback*>(frame->getEncapsulatedPacket());

            if (harqFeedback->getResult()){ // H-ARQ feedback is ACK

                string pktSuffixName = "pdu-"+std::to_string(harqFeedback->getFbMacPduId());
                map<string,simtime_t>::iterator timeStampIt ;

                //This KLUDGE is because harqFeedback's uInfo->getLcid() contains always the default value. Otherwise we do not know if this feedback is for a Data or BSR PDU. TODO fix this in eNodeB's feedback packet creation.
                bool dataEntryfound = (timeStampIt=packetFromUpperTimeStampsByInterfaceId[interfaceId].find("data-"+pktSuffixName)) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end();
                bool bsrEntryfound  = packetFromUpperTimeStampsByInterfaceId[interfaceId].find("BSR-"+pktSuffixName) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end();

                //This happens when the instant at which the packet has been sent is no logger in current DLT. So, statistics related to it should be ignored.
                if (!dataEntryfound && ! bsrEntryfound){
                    return; // So the feedback is considered as obsolete.  Ignore it.

                }else if (bsrEntryfound){ //check if the feedback is for a BSR
                    lastMacGrantObtentionDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId]["BSR-"+pktSuffixName];

                    packetFromUpperTimeStampsByInterfaceId[interfaceId].erase("BSR-"+pktSuffixName);
                    return ; // wait for the next DATA PDU to account this delay
                }

                simtime_t lastMacRACDelay = lastRacReceptionTimestamp > lastRacSendTimestamp ? lastRacReceptionTimestamp - lastRacSendTimestamp: SimTime::ZERO; //TODO
                simtime_t macDataPduHarqProcessTime = NOW - timeStampIt->second;

                //Delay metric
                delayInidicator = SIMTIME_DBL(TTI+lastMacRACDelay+TTI+lastMacGrantObtentionDelay+TTI+macDataPduHarqProcessTime);

                //retrieve last transmitted packet from its name
                std::map<string, int64_t>::iterator messageIt = lastTransmittedFramesLengthByInterfaceId[interfaceId].find(timeStampIt->first);
                std::get<1>(
                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) +=
                        messageIt->second; //To note that this packet length data has been successfully sent

                //Throughput metric
                throughputIndicator =
                        getThroughputIndicator(
                                std::get<1>(
                                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]),
                                throughputMesureInterval);

                // Reliability metric
                double numerator = (double) std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) ;
                double denominator =(double) std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]);
                reliabilityIndicator = denominator==0 ? 0 : numerator/denominator ;

                recordStatTuple(interfaceId, delayInidicator,
                        throughputIndicator, reliabilityIndicator);

                lastTransmittedFramesAckByInterfaceId[interfaceId].insert({timeStampIt->first,true});

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

double CollectStats::getDLT(double CofficientOfVariation, int interfaceId) {
    double dlt = exp(
            -1 * CofficientOfVariation
                    + log(commonDLTMaxByInterfaceId[interfaceId]));
    if (dlt < 0)
        throw cRuntimeError("Data life time interval can not be negative.");

    double minDLT= minNumOfControlTrafficPktInDLT*controlTrafficSendInterval;
    return dlt+minDLT;
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

    }else{ //This is the case where the feedback for recording stats of "interfaceId" are not known yet
        insertStatTuple(rList, NOW, DBL_MAX, 0, 0); //consider these penalties
    }

    //purge packets in utility variables (for recording stats) that are no longer in the current DLT interval.
    purgeUtilVars(interfaceId);

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

void CollectStats::purgeUtilVars(int interfaceId) {

    double minOfDlt = //TODO min really ? why not consider max of criterio's DLT instead
            std::max(dltByInterfaceIdByCriterion[interfaceId]["delayIndicator"],
                    std::max(
                            dltByInterfaceIdByCriterion[interfaceId]["throughputIndicator"],
                            dltByInterfaceIdByCriterion[interfaceId]["reliabilityIndicator"]));
    simtime_t dltIntervalLowerBound = NOW - SimTime(minOfDlt);

    for (auto it = lastTransmittedFramesLengthByInterfaceId[interfaceId].begin();
            it != lastTransmittedFramesLengthByInterfaceId[interfaceId].end();){ //This may be computationally very heavy. TODO iterate by timestamp and do break after dltIntervalLowerBound

        //retrieve the corresponding timeStamps
        std::map<string, simtime_t>::iterator msgTimeStampIt =
                packetFromUpperTimeStampsByInterfaceId[interfaceId].find(
                        it->first);

        if (msgTimeStampIt->second < dltIntervalLowerBound) {

            std::get<0>(
                    attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) -= it->second;

            std::map<string, bool>::iterator msgAckIt =
                    lastTransmittedFramesAckByInterfaceId[interfaceId].find(msgTimeStampIt->first);

            if (msgAckIt != lastTransmittedFramesAckByInterfaceId[interfaceId].end()){

                std::get<1>(
                        attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId[interfaceId]) -= it->second;
            }

            lastTransmittedFramesLengthByInterfaceId[interfaceId].erase(it++);
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgTimeStampIt);
        }else{
            it++;
        }
    }

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


void CollectStats::recordStatTuple(int interfaceId, double delayIndicator, double throughputIndicator, double reliabilityIndicator){

    if (delayIndicator < 0 ||  throughputIndicator < 0 || reliabilityIndicator < 0 )
        throw cRuntimeError("A decision metric can not be negative.");

    DecisionMaker* decisionModule = dynamic_cast<DecisionMaker*>(getParentModule()->getSubmodule("decisionMaker"));
    if(!decisionModule->isNaiveSingleCriterionBasedDecision())
        insertStatTuple(listOfCriteriaByInterfaceId[interfaceId], NOW ,delayIndicator, throughputIndicator, reliabilityIndicator) ;
    else
        insertStatTuple(interfaceId, NOW ,delayIndicator, throughputIndicator, reliabilityIndicator) ;

    if(interfaceId==0){
        emit(throughputIndicator0Signal,throughputIndicator);
        emit(delayIndicator0Signal,delayIndicator);
    }
    else{
        emit(throughputIndicator1Signal,throughputIndicator);
        emit(delayIndicator1Signal,delayIndicator);
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

    for (auto pair : lastTransmittedFramesLengthByInterfaceId){
        pair.second.clear();
    }
}
