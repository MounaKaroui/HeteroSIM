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
#include "stack/phy/packet/cbr_m.h"
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
        dltByInterfaceIdByCriterion[x.first]["delay"]=0;
        dltByInterfaceIdByCriterion[x.first]["availableBandwidth"]=0;
        dltByInterfaceIdByCriterion[x.first]["queueVacancy"]=0;
    }
}

void CollectStats::registerSignals()
{

    for(auto const & x: interfaceToProtocolMap){

        if (x.second.find("80211")== 0 || x.second.find("80215") == 0) {

            std::string macModuleName= "^.wlan["+ to_string(x.first) +"].mac";
            std::string radioModuleName= "^.wlan["+ to_string(x.first) +"].radio";

            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetReceivedFromUpperSignal);
            subscribeToSignal<inet::physicallayer::Radio>(radioModuleName, LayeredProtocolBase::packetSentToLowerSignal);

            //packet drop signals
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_PACKET_DROP);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, NF_LINK_BREAK);
            subscribeToSignal<inet::LayeredProtocolBase>(macModuleName, LayeredProtocolBase::packetFromUpperDroppedSignal); //TODO seek into replacing this signal by NF_PACKET_DROP --> modification of CSMA.cc

        }
        else if(x.second=="mode4")
        {
            std::string pdcpRrcModuleName = "^.lteNic.pdcpRrc";
            std::string phyModuleName = "^.lteNic.phy";
            std::string macModuleName = "^.lteNic.mac";

            bool mode4 = getAncestorPar("withMode4");
            if (mode4)
            {
                    subscribeToSignal<LtePdcpRrcUeD2D>(pdcpRrcModuleName,receivedPacketFromUpperLayerLteSignal);
                    subscribeToSignal<LtePhyVUeMode4>(phyModuleName,LtePhyVUeMode4::sentToLowerLayerSignal);
            }
        }

    }
}

double CollectStats::extractLteBufferVacancy()
{
    double queueVacancy=0;
    cModule* host=getContainingNode(this);
    std::string moduleName=string(host->getFullName())+".lteNic.mac";
    cModule* macModule=getModuleByPath(moduleName.c_str());
    LteMacVUeMode4* lteMac=dynamic_cast<LteMacVUeMode4*>(macModule);
    double count=0;
    for(auto& buf: lteMac->macBuffers_)
    {

        if(buf.second->getQueueLength()!=0)
            queueVacancy+=(buf.second->getQueueLength()-buf.second->getQueueOccupancy())/buf.second->getQueueLength();
        count++;
    }

    return (queueVacancy/count)*100;
}

double CollectStats::getWlanCBR(int interfaceId){

    cModule* host=getContainingNode(this);
    std::string moduleName=string(host->getFullName())+".wlan["+to_string(interfaceId)+"].mac.rx";
    cModule* rxModule=getModuleByPath(moduleName.c_str());
    ChannelLoadAccess* channelLoadRx= dynamic_cast<ChannelLoadAccess*>(rxModule);
    return channelLoadRx->getCBR();
}

double CollectStats::extractQueueVacancy(int interfaceId)
{
    double queueVacancy=0;
    cModule* host=getContainingNode(this);
    std::string moduleName=string(host->getFullName())+".wlan["+to_string(interfaceId)+"].mac.dcf";
    cModule* dcfModule=getModuleByPath(moduleName.c_str());
    using namespace inet::ieee80211;
    Dcf* dcf= dynamic_cast<Dcf*>(dcfModule);
    queueVacancy = ((dcf->pendingQueue->getMaxQueueSize()
            - dcf->pendingQueue->getLength())
            / dcf->pendingQueue->getMaxQueueSize()) * 100;
    return queueVacancy;
}

double CollectStats::getLteCBR()
{
    cModule* host=getContainingNode(this);
    std::string moduleName=string(host->getFullName())+".lteNic.phy";
    cModule* phyModule=getModuleByPath(moduleName.c_str());
    LtePhyVUeMode4* lteMac=dynamic_cast<LtePhyVUeMode4*>(phyModule);
    return lteMac->mCBR;
}

void CollectStats::recordStatsForWlan(simsignal_t comingSignal, string sourceName, cMessage* msg,  int interfaceId)
{

    if ( comingSignal == LayeredProtocolBase::packetReceivedFromUpperSignal &&  sourceName==string("mac")){ //when packet enter to MAC layer
        packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()]=NOW;
//        printMsg("Inserting",msg);
        return ;
    }

    if(listOfCriteriaByInterfaceId.find(interfaceId) == listOfCriteriaByInterfaceId.end()) // initialize stats data structure in case of the first record
           listOfCriteriaByInterfaceId.insert({ interfaceId, new listOfCriteria()});

    double delay, availableBandwidth, cbr, queueVacancy;

    if (comingSignal == LayeredProtocolBase::packetSentToLowerSignal && sourceName==string("radio")) { //when the packet comes out of the radio layer --> transmission duration already elapsed

        //Delay
        ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
//        printMsg("Reading",msg);
        simtime_t macAndRadioDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];
        packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
        delay = SIMTIME_DBL(macAndRadioDelay);

        //Transmission rate
        RadioFrame *radioFrame = check_and_cast<RadioFrame *>(msg);
        ASSERT(radioFrame && radioFrame->getDuration() != 0) ;
        //CBR
        cbr = getWlanCBR(interfaceId);
        emit(cbr0,cbr);

        availableBandwidth = getAvailableBandwidth((PK(msg)->getBitLength()),(radioFrame->getDuration()).dbl(),cbr);
        // Queue vacancy
        queueVacancy=extractQueueVacancy(interfaceId);


        recordStatTuple(interfaceId, delay, availableBandwidth, queueVacancy) ;


    } else if (comingSignal== NF_PACKET_DROP || comingSignal== NF_LINK_BREAK || comingSignal==LayeredProtocolBase::packetFromUpperDroppedSignal) // packet drop related calculations
        {
            ASSERT(packetFromUpperTimeStampsByInterfaceId[interfaceId].find(msg->getName()) != packetFromUpperTimeStampsByInterfaceId[interfaceId].end());
//            printMsg("Reading",msg);
            simtime_t macDelay = NOW - packetFromUpperTimeStampsByInterfaceId[interfaceId][msg->getName()];


            if(macDelay == 0){ //case of packet drop due to queue overflow
                //In case of full queue 802.15.4 interface sends "packetFromUpperDroppedSignal" signal
                //In case of full queue 802.11 interface sends "NF_PACKET_DROP" signal
                //so check the following assertion
                ASSERT(comingSignal == NF_PACKET_DROP  || comingSignal == LayeredProtocolBase::packetFromUpperDroppedSignal);
                ASSERT(false);
                //TODO add delay penalties to consider in case of packet drop
//                throw cRuntimeError("Packet drop due to queue overflow not supported yet");

            }else { //case of packet drop due failing CSMA process

                //Delay
                delay = SIMTIME_DBL(macDelay);
                packetFromUpperTimeStampsByInterfaceId[interfaceId].erase(msg->getName());
                //CBR
                 cbr = getWlanCBR(interfaceId);
                 emit(cbr0,cbr);

                 //Transmission rate
                availableBandwidth = getAvailableBandwidth(0,delay,cbr); //consider 0 bits are transmitted
                // Queue vacancy
                queueVacancy=extractQueueVacancy(interfaceId);

               recordStatTuple(interfaceId, delay, availableBandwidth, queueVacancy) ;
            }
        }
}



int CollectStats::getNumberOfTbFramesForTTI()
{
    std::string phyModuleName = "^.lteNic.phy";
    cModule* module = getModuleByPath(phyModuleName.c_str());
    LtePhyVUeMode4* mPhyLayer=dynamic_cast<LtePhyVUeMode4*>(module);
    return mPhyLayer->numberofTBFrames;
}

void CollectStats::recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId)
{
    double delay, availableBandwidth,cbr, queueVacancy;

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
            // cbr
            cbr=getLteCBR();
            emit(cbr1,cbr);
            //Transmission rate
            availableBandwidth = getAvailableBandwidth(getNumberOfTbFramesForTTI()*(PK(msg)->getBitLength()),  (lteAirFrame->getDuration()).dbl(),cbr);
            // buffer vacancy
            queueVacancy=extractLteBufferVacancy();

            recordStatTuple(interfaceId,delay,availableBandwidth,queueVacancy) ;
        }
    }
}



double CollectStats::getsendIntervalParam()
{
    cModule* host=getContainingNode(this);
    std::string hostName=host->getFullName();
    if(hostName=="car[20]")
    {
        std::string moduleName=hostName+".applLayer[0]"; // TODO: generalize this in case of many data applications
        cModule* module=getModuleByPath(moduleName.c_str());
        return module->par("sendInterval").doubleValue();
    }
}

void CollectStats::updateDLT(listOfCriteria* list, int interfaceId)
{
    // update dtl according to coefficient of variation
    dltByInterfaceIdByCriterion[interfaceId]["delay"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->delay)));
    dltByInterfaceIdByCriterion[interfaceId]["availableBandwidth"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->availableBandwidth)));
    dltByInterfaceIdByCriterion[interfaceId]["queueVacancy"]= getDLT(Utilities::calculateCofficientOfVariation(Utilities::retrieveValues(list->queueVacancy)));
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

    rList->delay= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->delay,dltByInterfaceIdByCriterion[interfaceId]["delay"] );
    rList->availableBandwidth= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->availableBandwidth,dltByInterfaceIdByCriterion[interfaceId]["availableBandwidth"] );
    rList->queueVacancy= getSublistByDLTOfCriterion(listOfCriteriaByInterfaceId[interfaceId]->queueVacancy,dltByInterfaceIdByCriterion[interfaceId]["queueVacancy"] );

    //update DLT
    updateDLT(rList, interfaceId);

    //purge stats history
    delete listOfCriteriaByInterfaceId[interfaceId]->delay;
    delete listOfCriteriaByInterfaceId[interfaceId]->availableBandwidth;
    delete listOfCriteriaByInterfaceId[interfaceId]->queueVacancy;
    delete listOfCriteriaByInterfaceId[interfaceId];
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
            listAttr->delay=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->delay));
            listAttr->availableBandwidth=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->availableBandwidth));
            listAttr->queueVacancy=Utilities::calculateMeanVec(Utilities::retrieveValues(x.second->queueVacancy));
            myList->data.insert({x.first,listAttr});
        }
    }else if(averageMethod==string("ema"))
    {
        for (auto& x : dataSet)
        {
            std::vector<double> vEMADelay;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->delay),vEMADelay);
            std::vector<double> vEMATransmissionRate;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->availableBandwidth),vEMATransmissionRate);
            std::vector<double> vEMAQueueVacancy;
            Utilities::calculateEMA(Utilities::retrieveValues(x.second->queueVacancy),vEMAQueueVacancy);
            alternativeAttributes* listAttr=new alternativeAttributes();
            listAttr->delay= vEMADelay.back() ;
            listAttr->availableBandwidth=vEMATransmissionRate.back();
            listAttr->queueVacancy=vEMAQueueVacancy.back();
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
        vector<double>* d=Utilities::retrieveValues(x.second->delay);
        vector<double>* avb=Utilities::retrieveValues(x.second->availableBandwidth);
        vector<double>* qc=Utilities::retrieveValues(x.second->queueVacancy);
        listAttr->delay=d->back();
        listAttr->availableBandwidth=avb->back();
        listAttr->queueVacancy=qc->back();
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

double CollectStats::getAvailableBandwidth(int64_t dataLength, double sendInterval, double cbr)
{
    return (1-cbr)*(dataLength/sendInterval); // bps
}


void CollectStats::recordStatTuple(int interfaceId, double delay, double transmissionRate, double queueVacancy){

    insertStatTuple(listOfCriteriaByInterfaceId[interfaceId], NOW ,delay, transmissionRate, queueVacancy) ;
    if(interfaceId==0){
        emit(tr0,transmissionRate);
        emit(delay0,delay);
    }
    else{
        emit(tr1,transmissionRate);
        emit(delay1,delay);
    }

}




void CollectStats::insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delay, double availableBandwitdth, double queueVacancy){

    // check and initialize list if necessary
    if(!list->delay)
        list->delay= new map<simtime_t,double>();
    if(!list->availableBandwidth)
        list->availableBandwidth = new  map<simtime_t,double>();
    if(!list->queueVacancy)
        list->queueVacancy= new  map<simtime_t,double>();


    list->delay->insert({timestamp,delay});
    list->availableBandwidth->insert({timestamp,availableBandwitdth});
    list->queueVacancy->insert({timestamp,queueVacancy});
}



