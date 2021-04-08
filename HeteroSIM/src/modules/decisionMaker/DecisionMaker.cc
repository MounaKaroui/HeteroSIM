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

#include <inet/common/ModuleAccess.h>
//#include "stack/phy/packet/cbr_m.h"
#include <stdlib.h>     /* srand, rand */


Define_Module(DecisionMaker);


simsignal_t DecisionMaker::decisionSignal = cComponent::registerSignal("decision");

void DecisionMaker::initialize()
{

    lteInterfaceIsActive = par("lteInterfaceIsActive").boolValue();

    pathToConfigFiles=par("pathToConfigFiles").stringValue();
    simpleWeights=par("simpleWeights").stringValue();
    criteriaType=par("criteriaType").stringValue();
    hysteresisTh=par("hysteresisTh").doubleValue();
    withMovingDLT=par("withMovingDLT").boolValue();


    isDeciderActive=par("isDeciderActive").boolValue();
    isRandomDecision = par("isRandomDecision").boolValue() ;
    dummyNetworkChoice = par("dummyNetworkChoice").intValue();

    isPingPongReductionActive=par("isPingPongReductionActive").boolValue();
    lastDecision=-1;

    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    int nodeId=Utilities::extractNumber(name.c_str());
    generator.seed(nodeId);
    distribution.param(std::bernoulli_distribution::param_type(0.5));

    addressResolver=getModuleFromPar<IAddressResolver>(par("addressResolver"), this);



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

    auto destAddress = basicMsg->getDestinationAddress();
    MACAddress destMacAddress;

    if(!strcmp(destAddress, "all")){//broadcast
        destMacAddress.setBroadcast();
    }else{
        destMacAddress = addressResolver->resolveIEEE802Address(destAddress, networkIndex);
    }

    //This means we assume that the receiver is using the same technology settings as the transmitter at its "networkIndex"^th interface
    MACAddress scrMacAddress = addressResolver->resolveIEEE802Address(basicMsg->getSourceAddress(), networkIndex);

    auto controlInfo = new Ieee802Ctrl();
    controlInfo->setSourceAddress(scrMacAddress);
    controlInfo->setDestinationAddress(destMacAddress);
    msg->setControlInfo(controlInfo) ;
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
    std::string rStr="";

    for (auto& x : listOfAlternativeAttributes->data)
    {
        rStr+=to_string(x.second->availableBandwidth);
        rStr+=","+to_string(x.second->delay);
        rStr+=","+to_string(x.second->reliability)+",";
    }
    return rStr.substr(0, rStr.size()-1);
}


double DecisionMaker::normalizeTh(double x1, double x2)
{
    return abs(x1-x2)/(x1+x2);
}

double DecisionMaker::calculateWeightedThresholdAverage(CollectStats::listAlternativeAttributes* newDecisionData)
{
    // Weight matrix [AB,Delay,Qv]
    McdaAlg::Matrix weight=McdaAlg::simple_weighting(simpleWeights);
    double sumTh=0;

    for (auto& newData : newDecisionData->data)
    {
        sumTh+=normalizeTh(newData.second->availableBandwidth,lastDecisionData->data[newData.first]->availableBandwidth)*weight.at(0,0);
        sumTh+=normalizeTh(newData.second->delay,lastDecisionData->data[newData.first]->delay)*weight.at(1,0);
        sumTh+=normalizeTh(newData.second->reliability,lastDecisionData->data[newData.first]->reliability)*weight.at(2,0);
    }

    return sumTh/weight.size(1);
}




int DecisionMaker:: reducePingPongEffects(int newDecision, CollectStats::listAlternativeAttributes* newDecisionData ){


    if(lastDecision==-1 ||newDecision==lastDecision)
    {
        return newDecision;
    }
    else
    {
        double meanTh =calculateWeightedThresholdAverage(newDecisionData);

        if(meanTh > hysteresisTh)
            return newDecision;
       else
            return lastDecision;
    }

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

        if(isDeciderActive)
        {
            cModule* mStats=getParentModule()->getSubmodule("collectStatistics");
            CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
            CollectStats::listAlternativeAttributes* decisionData;
            if (!withMovingDLT) {
                decisionData = stats->prepareDummyNetAttributes();

            } else {
                decisionData = stats->prepareNetAttributes();
            }
            std::string decisionDataStr=convertListOfCriteriaToString(decisionData);
            if(decisionDataStr!="")
            {
                // MCDM decision
                //std::cout<< "decision Data "<< decisionDataStr <<"\n" << endl;
                networkIndex = McdaAlg::decisionProcess(decisionDataStr,
                        pathToConfigFiles, "simple", simpleWeights,
                        criteriaType, trafficType, "TOPSIS");
                if(isPingPongReductionActive)
                {
                // Ping pong effects
                networkIndex=reducePingPongEffects(networkIndex,decisionData);
                // Store last decision data
                lastDecisionData=decisionData;
                // Store last decision
                lastDecision=networkIndex;
                }
                cModule* host = inet::getContainingNode(this);
                std::string name=host->getFullName();
                int nodeId=Utilities::extractNumber(name.c_str());
                std::cout<< "Node Id= " << nodeId << "\n"<< endl;

                std::cout<< "The best network is "<< networkIndex <<"\n" << endl;
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

                cModule* host = inet::getContainingNode(this);
                std::string name=host->getFullName();
                int nodeId=Utilities::extractNumber(name.c_str());
                std::cout<< "Node Id= " << nodeId << "\n"<< endl;
                std::cout<< "The random network is "<< networkIndex <<"\n" << endl;
            }else{
                networkIndex=dummyNetworkChoice;
                std::cout<< "The dummy choice is "<< networkIndex <<"\n" << endl;
            }
        }
    }

    return networkIndex;
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


DecisionMaker::~DecisionMaker()
{
    if(lteInterfaceIsActive)
        binder_->unregisterNode(nodeId_);

}



