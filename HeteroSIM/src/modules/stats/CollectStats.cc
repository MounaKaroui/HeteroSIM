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
#include "../../modules/messages/Messages_m.h"

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

#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/mac/packet/LteRac_m.h"

#include <random>


Define_Module(CollectStats);


void CollectStats::initialize(int stage)
{

    if (stage == inet::INITSTAGE_LOCAL) {
        interfaceToProtocolMapping = par("interfaceToProtocolMapping").stringValue();
        averageMethod = par("averageMethod").stringValue();
        controlTrafficSendInterval = par("controlTrafficSendInterval").doubleValue();

        throughputIndicator0Signal = registerSignal("throughputIndicator0");
        throughputIndicator1Signal = registerSignal("throughputIndicator1");

        delayIndicator0Signal = registerSignal("delayIndicator0");
        delayIndicator1Signal = registerSignal("delayIndicator1");

        reliabilityIndicator0Signal = registerSignal("reliabilityIndicator0");
        reliabilityIndicator1Signal = registerSignal("reliabilityIndicator1");

        setInterfaceToProtocolMap();
        DecisionMaker *decisionModule =dynamic_cast<DecisionMaker*>(getParentModule()->getSubmodule("decisionMaker"));
        deciderIsNaiveSingleCriterionBasedDecision=decisionModule->isNaiveSingleCriterionBasedDecision();

        if (decisionModule->isDeciderActived()) {
            registerSignals();
            setCommonDltMax(par("dltMaxByAppByInterfaceId").stringValue());
            setMinNumOfControlTrafficPktsInDLT(par("minNumOfControlTrafficPktInDLTByApp").stringValue());
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
    cStringTokenizer tokenizer(strValues.c_str(),string(";").c_str());
      while (tokenizer.hasMoreTokens()){

          std::string mappingEntry= tokenizer.nextToken();
          vector<string> result;
          boost::split(result, mappingEntry, boost::is_any_of(":"));
          dltMaxByAppByInterfaceId[stoi(result[0])][stoi(result[1])]=stod(result[2]);
//          DLTMaxByAppByInterfaceId.insert({stoi(result[0]),stod(result[1])});
      }
}

void CollectStats::setMinNumOfControlTrafficPktsInDLT(const char * strValues)
{
    cStringTokenizer tokenizer(strValues,string(",").c_str());
      while (tokenizer.hasMoreTokens()){

          std::string mappingEntry= tokenizer.nextToken();
          vector<string> result;
          boost::split(result, mappingEntry, boost::is_any_of(":"));
          minNumOfControlTrafficPktInDLTByApp.insert({stoi(result[0]),stod(result[1])});
      }
}

void CollectStats::initializeDLT()
{
    for(auto const & x: interfaceToProtocolMap){
        vector<int>* appIDs = Utilities::retrieveKeys(&minNumOfControlTrafficPktInDLTByApp) ;

        for(auto it = appIDs->begin(); it != appIDs->end(); ++it ){
            dltByInterfaceIdByCriterionByApp[x.first]["delayIndicator"][*it]=getDLT(0,x.first,*it);
            dltByInterfaceIdByCriterionByApp[x.first]["throughputIndicator"][*it]=getDLT(0,x.first,*it);
            dltByInterfaceIdByCriterionByApp[x.first]["reliabilityIndicator"][*it]=getDLT(0,x.first,*it);
        }
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
                lteMacBufferOverflowUl_ = macModule->registerSignal("macBufferOverFlowUl");
                macModule->subscribe(lteMacBufferOverflowUl_, this);
                lteMacBufferOverflowDl_ = macModule->registerSignal("macBufferOverFlowDl");
                macModule->subscribe(lteMacBufferOverflowDl_, this);
                lteMacBufferOverflowD2D_ = macModule->registerSignal("macBufferOverFlowD2D");
                macModule->subscribe(lteMacBufferOverflowD2D_, this);

                std::string phyModuleName = "^.lteNic.phy";
                LtePhyBase * phyModule=check_and_cast<LtePhyBase *>(getModuleByPath(phyModuleName.c_str()));
                ltePhyRecievedAirFrameSingal=phyModule->registerSignal("receivedAirFrame");
                phyModule->subscribe(ltePhyRecievedAirFrameSingal, this);
            }
        }
    }
}




double CollectStats::getThroughputIndicator(uint64_t dataBitLength, double transmitTime)
{
    if(isnan(transmitTime) || isinf(transmitTime) || transmitTime ==0 || dataBitLength<0)
        throw cRuntimeError("Incorrect input parameter(s) to calculate throughput: transmit time is %d and data length is %d",transmitTime,dataBitLength);
    return ((double)dataBitLength/transmitTime)*0.000001; //times *0.000001 to convert from bps to mbps
}


void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{
    BasicMsg* hetMsg;
    int currentAppID ;

    if ( comingSignal == DecisionMaker::decisionSignal){ //when packet leave decision maker toward transmission interface

        hetMsg=dynamic_cast<BasicMsg*>(msg);
        currentAppID = hetMsg->getApplId();

        //For delay metric
        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;

        // For reliability metric
        if(!deciderIsNaiveSingleCriterionBasedDecision)
            std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) += PK(msg)->getBitLength();
        else
            std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) = PK(msg)->getBitLength();

        //utility
        lastTransmittedFramesByInterfaceId[interfaceId].insert({msg->getName(),msg->dup()});

        return ;
    }


    double delayIndicator, throughputIndicator, reliabilityIndicator;
    double throughputMesureInterval;

    //retrieve info of last transmitted packet from its name
    std::map<string,cMessage*>::iterator messageIt = lastTransmittedFramesByInterfaceId[interfaceId].find(msg->getName());
    //This happens when the instant at which the packet has been sent is no logger in current DLT.
    if(messageIt==lastTransmittedFramesByInterfaceId[interfaceId].end())
        return ;//So, statistics related to it should be ignored.

    hetMsg=dynamic_cast<BasicMsg*>(messageIt->second);
    currentAppID = hetMsg->getApplId();

    if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //signal of packet coming out of the radio layer -> frame transmitter

        throughputMesureInterval = dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][currentAppID];

        Ieee802Ctrl *controlInfo = dynamic_cast<Ieee802Ctrl*>(msg->getControlInfo());

        ASSERT(!controlInfo->getDestinationAddress().isMulticast());

    } else if (comingSignal== NF_PACKET_DROP || /*comingSignal== NF_LINK_BREAK ||*/ comingSignal == NF_PACKET_ACK_RECEIVED) { //otherwise it is MAC error signal
            //This happens when the instant at which the packet has been sent is no logger in current DLT. So, statistics related to it should be ignored.
            if(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) == packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
               return;
            }

            simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];

            if(macDelay == 0){ //case of packet drop due to queue overflow considering penalties
                //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
                //so check the following assertion
                ASSERT(comingSignal == NF_PACKET_DROP);

                //Delay metric
                delayIndicator = recentValidWlanDelay;
                //Throughput metric
                throughputIndicator =0; //TODO is this safe ?

            }else { //case of packet drop due failing CSMA/CA process or previous ACK received considering penalties

                //Delay metric
                delayIndicator = SIMTIME_DBL(macDelay);
                recentValidWlanDelay = delayIndicator;

                if (comingSignal == NF_PACKET_ACK_RECEIVED){
                    if(!deciderIsNaiveSingleCriterionBasedDecision){
                        std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) += PK(messageIt->second)->getBitLength();
                        lastTransmittedFramesAckByInterfaceId[interfaceId].insert({msg->getName(),true});
                    }else{
                        std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) = PK(messageIt->second)->getBitLength();
                    }
                }
                 //Throughput metric
                throughputMesureInterval = dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][currentAppID];
                // In case of decider using NaiveSingleCriterionBasedDecision, this interval is used instead of the MAC delay to avoid correlation of delay metric and throughput
                throughputIndicator = getThroughputIndicator(std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]), throughputMesureInterval);
            }

            double numerator = (double) std::get<1>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) ;
            double denominator =(double) std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]);
            // Reliability metric
            reliabilityIndicator = denominator==0 ? 0 : numerator/denominator ;

            // All traffic stats (for data or control) are to be shared with each data application  //TODO avoid "hard coding" of 3 applications
            recordStatTuple(interfaceId,0,delayIndicator, throughputIndicator, reliabilityIndicator) ;
            recordStatTuple(interfaceId,1,delayIndicator, throughputIndicator, reliabilityIndicator) ;
            recordStatTuple(interfaceId,2,delayIndicator, throughputIndicator, reliabilityIndicator) ;

            if(deciderIsNaiveSingleCriterionBasedDecision){
                lastTransmittedFramesByInterfaceId[interfaceId].erase(msg->getName());
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
            }
        }
}

void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{
    if (comingSignal == DecisionMaker::decisionSignal) {
        return;// from here we do not know the IDs of MAC PDUs sent to initialize utility variables.
    }

    double delayInidicator, throughputIndicator, reliabilityIndicator;

    UserControlInfo *uInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());

    int currentAppID ;
    LteMacPdu *pduSent;

    if((comingSignal==lteMacSentPacketToLowerLayerSingal)){

        if(uInfo->getFrameType() == RACPKT){
            lastRacSendTimestamp = NOW;
            return ;
        }


        if (uInfo->getFrameType() == DATAPKT){

            pduSent = dynamic_cast<LteMacPdu*>(msg)->dup();

            bool isMulticastMessage = uInfo->getDestId() == lteInterfaceMacId_;  // by convention the node self mac id is set as the destId for multicast message. See LtePdcpRrcUeD2D::fromDataPort line 63.

            if (!isMulticastMessage) { //unicast traffic: initialize utility variables and wait for HARQ feedback to record stats.
                  //this instruction block is equivalent to "when packet leave decision maker toward transmission interface" recordStatsForWlan

                if(uInfo->getLcid() == D2D_SHORT_BSR){// RSR report case. (See in LteMacUeD2D::macPduMake)
                   string pktName = "BSR-pdu-"+std::to_string(pduSent->getMacPduId());

                    //Check if this PDU is note already timestamped. This may happen in  case of retransmission of it
                    if (packetFromUpperTimeStampsByInterfaceId[interfaceId].find(pktName) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                        return ; //If yes this is a retransmission of it, ignore it.
                    }

                    //For delay metric
                    packetFromUpperTimeStampsByInterfaceId[interfaceId][pktName]=NOW;
                    lastTransmittedFramesByInterfaceId[interfaceId].insert({pktName,(msg)->dup()});
                    return;

                } else if (uInfo->getLcid() == SHORT_BSR) {

                    string pktNamePrefix = "data-pdu-"+std::to_string(pduSent->getMacPduId());

                    map<uint16_t, int64_t> appIdToSDUbytes;
                    map<uint16_t, int64_t>::iterator it;
                    ASSERT(pduSent->hasSdu());

                    bool isRetransmittedPDU = false;

                    while (pduSent->hasSdu()) {

                        cPacket *sdu = pduSent->popSdu();
                        FlowControlInfo *fInfo = check_and_cast<FlowControlInfo*>(sdu->getControlInfo());
                        currentAppID = fInfo->getSrcPort();

                        string currentSduName = pktNamePrefix + "-app-" + std::to_string(currentAppID) + "-sdu";

                        //Check if the PDU containing this SDU is note already timestamped. This may happen in  case of retransmission of it
                        if (packetFromUpperTimeStampsByInterfaceId[interfaceId].find(currentSduName) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end()){
                            isRetransmittedPDU = true;
                            break; // we break because, since it is the same PDU that carries the SDUs; if one SDU of them is not found then the others should not be found also
                        }

                        //For delay metric
                        packetFromUpperTimeStampsByInterfaceId[interfaceId][currentSduName]=NOW;

                        cPacket *sduCopy = sdu->dup();
                        sduCopy->setControlInfo(fInfo->dup());
                        lastTransmittedFramesByInterfaceId[interfaceId].insert( {currentSduName, sduCopy });

                        map<uint16_t, int64_t>::iterator it = appIdToSDUbytes.find(currentAppID);

                        if (it == appIdToSDUbytes.end())
                            appIdToSDUbytes.insert( { currentAppID, 0 });

                        appIdToSDUbytes[currentAppID] += sdu->getBitLength();

                    }

                    if(isRetransmittedPDU)
                        return ; //If yes this is a retransmission of it, ignore it.

                    // For reliability metric of each application
                    for (it = appIdToSDUbytes.begin();it != appIdToSDUbytes.end(); it++)
                        std::get<0>(
                                attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[it->first][interfaceId]) +=
                                it->second;

                } else
                    throw cRuntimeError("Unknown LCID type");

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

            LteHarqFeedback *harqFeedback = check_and_cast<LteHarqFeedback*>(
                    frame->getEncapsulatedPacket());

            string pktSuffixName = "pdu-"
                    + std::to_string(harqFeedback->getFbMacPduId());

            //This KLUDGE is because harqFeedback's uInfo->getLcid() contains always the default value. Otherwise we do not know if this feedback is for a Data or BSR PDU. TODO fix this in eNodeB's feedback packet creation.
            bool bsrEntryfound =
                    packetFromUpperTimeStampsByInterfaceId[interfaceId].find(
                            "BSR-" + pktSuffixName)
                            != packetFromUpperTimeStampsByInterfaceId[interfaceId].end();

            if (bsrEntryfound) { //check if the feedback is for a BSR
                lastMacGrantObtentionDelay =
                        NOW
                                - packetFromUpperTimeStampsByInterfaceId[interfaceId]["BSR-"
                                        + pktSuffixName];

                //purge related stat utilities
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(
                        "BSR-" + pktSuffixName);
                map<string, cMessage*>::iterator it =
                        lastTransmittedFramesByInterfaceId[interfaceId].find(
                                "BSR-" + pktSuffixName);
                delete it->second;
                lastTransmittedFramesByInterfaceId[interfaceId].erase(
                        it->first);

                return; // wait for the next DATA PDU to account this delay
            }

            bool dataEntryNotFound = false;

            //Retrieve the PDU of this feedback in HARQ buffer
            cModule *module = getModuleByPath(string("^.lteNic.mac").c_str());
            LteMacBase *lteMacModule = check_and_cast<LteMacBase*>(module);
            HarqTxBuffers *harqTxBuffers = lteMacModule->getHarqTxBuffers();

            HarqTxBuffers::iterator hit = harqTxBuffers->find(
                    uInfo->getSourceId());
            ASSERT( hit != harqTxBuffers->end());

            LteHarqBufferTx *txBuf = hit->second;
            LteMacPdu *pduSent =
                    txBuf->getProcess(harqFeedback->getAcid())->getPdu(
                            harqFeedback->getCw())->dup();
            string pktNamePrefix = "data-pdu-"
                    + std::to_string(pduSent->getMacPduId());

            map<uint16_t, int64_t> appIdToSDUbytes;
            map<uint16_t, int64_t>::iterator it;
            map<string, simtime_t>::iterator timeStampIt;

            while (pduSent->hasSdu()) {

                cPacket *sdu = pduSent->popSdu();
                FlowControlInfo *fInfo = check_and_cast<FlowControlInfo*>(
                        sdu->getControlInfo());
                currentAppID = fInfo->getSrcPort();

                string currentSduName = pktNamePrefix + "-app-"
                        + std::to_string(currentAppID) + "-sdu";

                if ((timeStampIt =
                        packetFromUpperTimeStampsByInterfaceId[interfaceId].find(
                                currentSduName))
                        == packetFromUpperTimeStampsByInterfaceId[interfaceId].end()) {
                    dataEntryNotFound = true;
                    break; // we break because, since it is the same PDU that carries the SDUs;
                            //if one SDU of them is not found then the others should not be found also
                }

                it = appIdToSDUbytes.find(currentAppID);
                if (it == appIdToSDUbytes.end())
                    appIdToSDUbytes.insert( { currentAppID, 0 });
                appIdToSDUbytes[currentAppID] += sdu->getBitLength();

                if (harqFeedback->getResult()) { // H-ARQ feedback is ACK
                    lastTransmittedFramesAckByInterfaceId[interfaceId].insert( {
                            currentSduName, true });
                }
            }
            delete pduSent;

            //This happens when the instant at which the packet has been sent is no logger in current DLT. So, statistics related to it should be ignored.
            if (dataEntryNotFound)
                return; // So the feedback is considered as obsolete.  Ignore it.

            simtime_t lastMacRACDelay =
                    lastRacReceptionTimestamp > lastRacSendTimestamp ?
                            lastRacReceptionTimestamp - lastRacSendTimestamp :
                            SimTime::ZERO; //TODO
            simtime_t macDataPduHarqProcessTime = NOW - timeStampIt->second;

            //Delay metric
            delayInidicator =
                    SIMTIME_DBL(
                            TTI+lastMacRACDelay+TTI+lastMacGrantObtentionDelay+TTI+macDataPduHarqProcessTime);

            for (it = appIdToSDUbytes.begin(); it != appIdToSDUbytes.end();
                    it++) {
                currentAppID = it->first;

                if (harqFeedback->getResult()) { // H-ARQ feedback is ACK
                    std::get<1>(
                            attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) +=
                            it->second; //To note that this packet length data has been successfully sent
                }

                //Throughput metric
                double throughputMesureInterval = dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][currentAppID];
                // In case of decider using NaiveSingleCriterionBasedDecision, this interval is used instead of the MAC delay to avoid correlation of delay metric and throughput

                throughputIndicator =
                        getThroughputIndicator(
                                std::get<1>(
                                        attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]),
                                throughputMesureInterval);

                // Reliability metric
                double numerator =
                        (double) std::get<1>(
                                attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]);
                double denominator =
                        (double) std::get<0>(
                                attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]);
                reliabilityIndicator =
                        denominator == 0 ? 0 : numerator / denominator;

                // All traffic stats (for data or control) are to be shared with each data application  //TODO avoid "hard coding" of 3 applications
                recordStatTuple(interfaceId, 0, delayInidicator,
                            throughputIndicator, reliabilityIndicator);
                recordStatTuple(interfaceId, 1, delayInidicator,
                            throughputIndicator, reliabilityIndicator);
                recordStatTuple(interfaceId, 2, delayInidicator,
                            throughputIndicator, reliabilityIndicator);

                if(deciderIsNaiveSingleCriterionBasedDecision){
                        std::get<0>(
                                attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) -=
                                it->second;
                        if (harqFeedback->getResult()) { // H-ARQ feedback is ACKstd::get<1>(
                            std::get<0>(attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[currentAppID][interfaceId]) -=
                                it->second;
                        }
                    }

            }

        }

    }else if (comingSignal == lteMacBufferOverflowDl_ || comingSignal == lteMacBufferOverflowUl_  || comingSignal == lteMacBufferOverflowD2D_ ){
        cRuntimeError("LTE interface buffer overflow not supported yet");
    }
}

void CollectStats::updateDLT(listOfCriteria* list, int interfaceId,int appID)
{
    // update dtl according to coefficient of variation
    dltByInterfaceIdByCriterionByApp[interfaceId]["delayIndicator"][appID]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->delayIndicator)),interfaceId,appID);
    dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][appID]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->throughputIndicator)),interfaceId,appID);
    dltByInterfaceIdByCriterionByApp[interfaceId]["reliabilityIndicator"][appID]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->reliabilityIndicator)),interfaceId,appID);
}

double CollectStats::getDLT(double CofficientOfVariation, int interfaceId, int appID) {
    double dlt = exp(
            -1 * CofficientOfVariation
                    + log(dltMaxByAppByInterfaceId[appID][interfaceId]));
    if (dlt < 0)
        throw cRuntimeError("Data life time interval can not be negative.");

    double minDLT= minNumOfControlTrafficPktInDLTByApp[appID]*controlTrafficSendInterval;
    return dlt+minDLT;
}

map<int,CollectStats::listOfCriteria*> CollectStats::getSublistByDLT(int appID)
{
    map<int,CollectStats::listOfCriteria*> rMap;

    for (const auto &pair : listOfCriteriaByInterfaceIdByAppId){
       int interfaceId = pair.first;
       rMap.insert({interfaceId, getSublistByDLT(interfaceId,appID)}) ;
    }

    return rMap;
}

CollectStats::listOfCriteria* CollectStats::getSublistByDLT(int interfaceId,int appID) {

    listOfCriteria* rList = new listOfCriteria();

    //Check if stats of "interfaceId" are recorded
    if(listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->delayIndicator && listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->throughputIndicator && listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->reliabilityIndicator ){

        rList->delayIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->delayIndicator,dltByInterfaceIdByCriterionByApp[interfaceId]["delayIndicator"][appID]);
        rList->throughputIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->throughputIndicator,dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][appID] );
        rList->reliabilityIndicator= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->reliabilityIndicator,dltByInterfaceIdByCriterionByApp[interfaceId]["reliabilityIndicator"][appID] );

        //update DLT
        updateDLT(rList, interfaceId,appID);

        //purge stats history
        delete listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->delayIndicator;
        delete listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->throughputIndicator;
        delete listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]->reliabilityIndicator;
        delete listOfCriteriaByInterfaceIdByAppId[interfaceId][appID];

    }else{ //This is the case where the feedback for recording stats of "interfaceId" are not known yet
        insertStatTuple(rList, NOW, 10, 0, 0); //consider these penalties
    }

    //purge packets in utility variables (for recording stats) that are no longer in the current DLT interval.
    purgeUtilVars(interfaceId,appID); //TODO purge stats related to control traffic apps

    listOfCriteriaByInterfaceIdByAppId[interfaceId][appID]=rList;

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

CollectStats::listAlternativeAttributes* CollectStats::prepareNetAttributes(int appID)
{
    // 1- get Data until NOW -DLT
    map<int,listOfCriteria*> dataSet= getSublistByDLT(appID);
    // 2- Apply average method
    listAlternativeAttributes* a=applyAverageMethod(dataSet);
    return a;

}

void CollectStats::purgeUtilVars(int interfaceId,int appID) {

    double minOfDlt = //TODO min really ? why not consider max of criterio's DLT instead
            std::max(dltByInterfaceIdByCriterionByApp[interfaceId]["delayIndicator"][appID],
                    std::max(
                            dltByInterfaceIdByCriterionByApp[interfaceId]["throughputIndicator"][appID],
                            dltByInterfaceIdByCriterionByApp[interfaceId]["reliabilityIndicator"][appID]));
    simtime_t dltIntervalLowerBound = NOW - SimTime(minOfDlt);

    for (auto it = lastTransmittedFramesByInterfaceId[interfaceId].begin();
            it != lastTransmittedFramesByInterfaceId[interfaceId].end();){ //This may be computationally very heavy. TODO iterate by timestamp and do break after dltIntervalLowerBound


        if((it->first.rfind("BSR-",0)==0) || (getAppId(it->second,interfaceId)!=appID)){
            it++;
            continue;
        }

        //retrieve the corresponding timeStamps
        std::map<string, simtime_t>::iterator msgTimeStampIt =
                packetFromUpperTimeStampsByInterfaceId[interfaceId].find(
                        it->first);

        if (msgTimeStampIt->second < dltIntervalLowerBound) {

            std::get<0>(
                    attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[appID][interfaceId]) -= PK(it->second)->getBitLength();

            std::map<string, bool>::iterator msgAckIt =
                    lastTransmittedFramesAckByInterfaceId[interfaceId].find(msgTimeStampIt->first);

            if (msgAckIt != lastTransmittedFramesAckByInterfaceId[interfaceId].end()){

                std::get<1>(
                        attemptedToBeAndSuccessfullyTransmittedDataByAppIdByInterfaceId[appID][interfaceId]) -= PK(it->second)->getBitLength();
            }

            delete it->second;
            lastTransmittedFramesByInterfaceId[interfaceId].erase(it++);
            packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msgTimeStampIt);

        }else{
            it++;
        }
    }

}

int CollectStats::getAppId(cMessage * msg, int pInterfaceId){

    if(pInterfaceId==0){

        return dynamic_cast<BasicMsg*>(msg)->getApplId();

    }else if(pInterfaceId==1 ){

        cPacket* sdu = PK(msg);
        FlowControlInfo* fInfo = check_and_cast<FlowControlInfo*>(sdu->getControlInfo());
        return  fInfo->getSrcPort();

    } else throw cRuntimeError("Unknown Interface Id type");

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

/**
 * To initialize stats data structure in case of the first record
 */
void CollectStats::checkAndInitializeListOfCriteria(int isPositiveRealNumberinterfaceId, int appID){

    if(listOfCriteriaByInterfaceIdByAppId.find(interfaceId) == listOfCriteriaByInterfaceIdByAppId.end()){

        map<int,listOfCriteria*> newMap = {{appID, new listOfCriteria()}};
        listOfCriteriaByInterfaceIdByAppId.insert( {interfaceId, newMap });

    }else if(listOfCriteriaByInterfaceIdByAppId[interfaceId].find(appID)==listOfCriteriaByInterfaceIdByAppId[interfaceId].end()){
        listOfCriteriaByInterfaceIdByAppId[interfaceId].insert({appID, new listOfCriteria()});
    }
}

void CollectStats::recordStatTuple(int interfaceId, int appID, double delayIndicator, double throughputIndicator, double reliabilityIndicator){

    if (!Utilities::isPositiveRealNumber(delayIndicator)
            || !Utilities::isPositiveRealNumber(throughputIndicator)
            || !Utilities::isPositiveRealNumber(reliabilityIndicator < 0))
        throw cRuntimeError("Invalid decision statistic.");

    if(!deciderIsNaiveSingleCriterionBasedDecision){
        checkAndInitializeListOfCriteria(interfaceId, appID);
        insertStatTuple(listOfCriteriaByInterfaceIdByAppId[interfaceId][appID], NOW ,delayIndicator, throughputIndicator, reliabilityIndicator) ;
    } else
        insertStatTupleAsRecentStat(interfaceId,appID, NOW ,delayIndicator, throughputIndicator, reliabilityIndicator) ;

    if(interfaceId==0){
        if (appID == 0)
            emit(delayIndicator0Signal, delayIndicator);
        else if (appID == 1)
            emit(throughputIndicator0Signal, throughputIndicator);
        else if (appID == 2)
            emit(reliabilityIndicator0Signal, reliabilityIndicator);
    }
    else{
        if (appID == 0)
            emit(delayIndicator1Signal, delayIndicator);
        else if (appID == 1)
            emit(throughputIndicator1Signal, throughputIndicator);
        else if (appID == 2)
            emit(reliabilityIndicator1Signal, reliabilityIndicator);
    }


}

void CollectStats::insertStatTupleAsRecentStat(int interfaceId, int appID, simtime_t timestamp, double delayIndicator, double throughputIndicator, double reliabilityIndicator){

    // check and initialize map if necessary
    if(recentCriteriaStatsByAppIdByInterfaceId.find(appID)== recentCriteriaStatsByAppIdByInterfaceId.end()){

        listAlternativeAttributes* listAltAtt = new listAlternativeAttributes();
        listAltAtt->data.insert({interfaceId, new alternativeAttributes()});
        recentCriteriaStatsByAppIdByInterfaceId.insert({appID, listAltAtt});

    }else if(recentCriteriaStatsByAppIdByInterfaceId[appID]->data.find(interfaceId)==recentCriteriaStatsByAppIdByInterfaceId[appID]->data.end()){
        recentCriteriaStatsByAppIdByInterfaceId[appID]->data.insert({interfaceId, new alternativeAttributes()});
    }

    recentCriteriaStatsByAppIdByInterfaceId[appID]->data[interfaceId]->delayIndicator=delayIndicator;
    recentCriteriaStatsByAppIdByInterfaceId[appID]->data[interfaceId]->throughputIndicator=throughputIndicator;
    recentCriteriaStatsByAppIdByInterfaceId[appID]->data[interfaceId]->reliabilityIndicator=reliabilityIndicator;
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

    for (auto pair1  : lastTransmittedFramesByInterfaceId){
        for (auto pair2  : pair1.second)
               delete pair2.second;
        pair1.second.clear();
    }
}
