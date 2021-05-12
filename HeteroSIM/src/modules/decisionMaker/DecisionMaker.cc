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
// @author Mouna KAROUI

#include "../decisionMaker/DecisionMaker.h"
#include "../../modules/messages/Messages_m.h"
#include "common/LteCommon.h"

#include "common/LteControlInfo_m.h"

#include <inet/common/ModuleAccess.h>
//#include "stack/phy/packet/cbr_m.h"
#include <stdlib.h>     /* srand, rand */


Define_Module(DecisionMaker);


simsignal_t DecisionMaker::decisionSignal = cComponent::registerSignal("decision");

void DecisionMaker::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        lteInterfaceIsActive = getAncestorPar("lteInterfaceIsActive").boolValue();

        pathToConfigFiles=par("pathToConfigFiles").stringValue();
        simpleWeights=par("simpleWeights").stringValue();
        criteriaType=par("criteriaType").stringValue();


        isDeciderActive=par("isDeciderActive").boolValue();
        isRandomDecision = par("isRandomDecision").boolValue() ;
        naiveSingleCriterionBasedDecision = par("naiveSingleCriterionBasedDecision").boolValue();
        naiveSingleCriterionBasedDecisionChoice = par("naiveSingleCriterionBasedDecisionChoice").intValue();
        dummyNetworkChoice = par("dummyNetworkChoice").intValue();

        cModule* host = inet::getContainingNode(this);
        std::string name=host->getFullName();
        int nodeId=Utilities::extractNumber(name.c_str());
        generator.seed(nodeId);
        distribution.param(std::bernoulli_distribution::param_type(0.5));

        addressResolver=getModuleFromPar<IAddressResolver>(par("addressResolver"), this);
        if (lteInterfaceIsActive) lteBinder_ =getBinder();
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER) {// this is to wait that lteInterfaceUpperLayerAddress_ be available
        if(lteInterfaceIsActive){
            const char* moduleName = getParentModule()->getFullName();
            lteInterfaceUpperLayerAddress_ = lteBinder_->getMacNodeIdByModuleName(moduleName);
        }
    }

}

/**
 * To create control info, mandatory to pass message lower layer protocol
 */
void DecisionMaker::setCtrlInfoWithRespectToNetType(cMessage* msg, int networkIndex) {


    string networkTypeName=getNetworkProtocolName(networkIndex);

    if (networkTypeName.find("WiFi") != std::string::npos)  { // IEEE 802 protocol family have common ctrlInfo
        setIeee802CtrlInfo(msg,networkIndex);
    }
    else if (networkTypeName == "LTE") {
        setLteCtrlInfo(msg);
    }
    else
        throw cRuntimeError(string("Unknown protocol name '" + networkTypeName + "'").c_str());

}

void DecisionMaker::setIeee802CtrlInfo(cMessage* msg, int networkIndex){

    auto basicMsg =dynamic_cast<BasicMsg*>(msg);

    auto destHost = basicMsg->getDestinationAddress();
    MACAddress destMacAddress;

    if(!strcmp(destHost, "all")){//broadcast
        destMacAddress.setBroadcast();
    }else{
        destMacAddress = addressResolver->resolveIEEE802Address(destHost, networkIndex);
    }

    //There is an assumption here that the receiver is using the same technology settings as the transmitter at its "networkIndex"^th interface
    MACAddress scrMacAddress = addressResolver->resolveIEEE802Address(basicMsg->getSourceAddress(), networkIndex);

    auto controlInfo = new Ieee802Ctrl();
    controlInfo->setSourceAddress(scrMacAddress);
    controlInfo->setDestinationAddress(destMacAddress);
    msg->setControlInfo(controlInfo);
}

void DecisionMaker::setLteCtrlInfo(cMessage* msg){

    auto basicMsg =dynamic_cast<BasicMsg*>(msg);
    auto destHost = basicMsg->getDestinationAddress();

    FlowControlInfo* ctrlInfo =new FlowControlInfo() ;

    ctrlInfo->setSrcAddr(lteInterfaceUpperLayerAddress_);

    if(!strcmp(destHost, "all")){//broadcast
        ctrlInfo->setDstAddr(0xE0000000);// 0xE0000000=224.0.0.0 is as broadcast/multicast address
    }else{
        ctrlInfo->setDstAddr(addressResolver->resolveLTEMacAddress(destHost));//
    }
    ctrlInfo->setSrcPort(basicMsg->getApplId());
    ctrlInfo->setDstPort(basicMsg->getApplId());
    //ctrlInfo->setSequenceNumber(sequenceNumber) TODO ?
    msg->setControlInfo(ctrlInfo);
}

string DecisionMaker::getNetworkProtocolName(int networkIndex){

    cModule* mStats=getParentModule()->getSubmodule("collectStatistics");
    CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
    return stats->interfaceToProtocolMap[networkIndex];
}

void DecisionMaker:: sendToLower(cMessage*  msg, int networkIndex)
{

    setCtrlInfoWithRespectToNetType(msg, networkIndex);
    if(networkIndex<gateSize("toRadio"))
    {
        int gateId=gate("toRadio",networkIndex)->getId();
        send(msg, gateId);
        emit(decisionSignal, networkIndex, msg);
    }
    else
    {
        delete msg;
        EV_INFO<<"interface index doesn't exist" <<endl;
    }
}



void DecisionMaker::sendToUpper(cMessage*  msg)
{
    if (string(msg->getName()).find("hetNets") != std::string::npos) {
        BasicMsg* hetNetsMsg = dynamic_cast<BasicMsg*>(msg);
        int id = hetNetsMsg->getApplId();
        if(id<gateSize("toApplication"))
        {
        int gateId = gate("toApplication", id)->getId(); // To get Id
        send(msg, gateId);

        }
        else
        {
            delete msg;
        }
    }
}


std::string DecisionMaker::convertListOfCriteriaToString(CollectStats::listAlternativeAttributes* listOfAlternativeAttributes)
{
    std::stringstream ss;

    for (auto& x : listOfAlternativeAttributes->data)
    {
        ss<<x.second->delayIndicator;
        ss<<","<<x.second->throughputIndicator;
        ss<<","<<x.second->reliabilityIndicator<<",";
    }

    std::string rStr=ss.str();
    return rStr.substr(0, rStr.size()-1);
}


int DecisionMaker::takeDecision(cMessage* msg)
{
    int networkIndex;
    ControlMsg * controlMsg =dynamic_cast<ControlMsg*>(msg);

    if(controlMsg){
        networkIndex=controlMsg->getNetworkId();
    }else{

        // MCDM_procedure
        HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
        std::string trafficType=hetMsg->getTrafficType();

        cModule* host = inet::getContainingNode(this);
         std::string name=host->getFullName();
         int nodeId=Utilities::extractNumber(name.c_str());
         std::cout<< "Node Id= " << nodeId << "\n"<< endl;

        if(isDeciderActive)
        {
            if(! naiveSingleCriterionBasedDecision){

                cModule* mStats=getParentModule()->getSubmodule("collectStatistics");
                CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
                CollectStats::listAlternativeAttributes* decisionData = stats->prepareNetAttributes();

                std::string decisionDataStr=convertListOfCriteriaToString(decisionData);
                if(decisionDataStr!="")
                {
                    // MCDM decision
                    //std::cout<< "decision Data "<< decisionDataStr <<"\n" << endl;
                    networkIndex = McdaAlg::decisionProcess(decisionDataStr,
                            pathToConfigFiles, "simple", simpleWeights,
                            criteriaType, trafficType, "TOPSIS");

                    std::cout<< "The best network is "<< networkIndex <<"\n" << endl;
                } else
                    throw cRuntimeError("No stats for decision making ");
            }else{
                networkIndex = takeNaiveSingleCriterionBasedDecision();
                std::cout<< "The naive single criteria based decision is "<< networkIndex <<"\n" << endl;
            }
        }
        else
        {
            if(isRandomDecision)
            {
                if (distribution(generator))
                    networkIndex=1;
                else
                    networkIndex=0;

                std::cout<< "The random network is "<< networkIndex <<"\n" << endl;
            }else{
                networkIndex=dummyNetworkChoice;
                std::cout<< "The dummy choice is "<< networkIndex <<"\n" << endl;
            }
        }
    }

    return networkIndex;
}

int DecisionMaker::takeNaiveSingleCriterionBasedDecision(){

    CollectStats* stats=dynamic_cast<CollectStats*>(getParentModule()->getSubmodule("collectStatistics"));

    double min = DBL_MAX;
    double max = DBL_MIN;
    int rChoice=0;

    for (const auto &pair : stats->recentCriteriaStatsByInterfaceId.data){

        int interfaceId = pair.first;
        double recentData ;
        switch (naiveSingleCriterionBasedDecisionChoice){
        case 0 : //delay criterion
            recentData =pair.second->delayIndicator;
            if( recentData < min ){ // look at minimum since this criterion is considered as smaller the better
                min = recentData ;
                rChoice = interfaceId ;
            }
            break ;

        case 1 : //throughput criterion
            recentData =pair.second->throughputIndicator;
            if( recentData > max ){ // look at maximum since this criterion is considered as smaller the better
                max = recentData ;
                rChoice = interfaceId ;
            }
          break ;

        case 2 : //reliability criterion
             recentData =pair.second->reliabilityIndicator;
            if( recentData > max ){ // look at maximum since this criterion is considered as smaller the better
                max = recentData ;
                rChoice = interfaceId ;
            }
            break;
        default:  throw cRuntimeError("Can not find criterion that corresponds to the configured 'naiveSingleCriterionBasedDecisionChoice' parameter.");

        }
    }

    return rChoice ;

}

void DecisionMaker::handleMessage(cMessage *msg)
{

    if (msg != nullptr) {
        if (msg->getArrivalGate()->getName()==string("fromApplication")) {
            int networkIndex = takeDecision(msg);
            sendToLower(msg, networkIndex);

        } else {
            // from  radio
            sendToUpper(msg);

        }
    }
}


DecisionMaker::~DecisionMaker(){} ;

void DecisionMaker::finish(){
    if(lteInterfaceIsActive)
            lteBinder_->unregisterNode(lteInterfaceUpperLayerAddress_);
};
