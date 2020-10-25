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

#include <inet/common/ModuleAccess.h>
#include "stack/phy/packet/cbr_m.h"
#include <stdlib.h>     /* srand, rand */

#include "../stats/CollectStats.h"
Define_Module(DecisionMaker);




void DecisionMaker::initialize()
{

    mode4=par("mode4IsActive").boolValue();
    pathToConfigFiles=par("pathToConfigFiles").stringValue();
    simpleWeights=par("simpleWeights").stringValue();
    criteriaType=par("criteriaType").stringValue();
    decisionSignal=registerSignal("decision");

    if(mode4)
    {
        registerNodeToBinder();
    }
    isDeciderActive=par("isDeciderActive").boolValue();
    if(!isDeciderActive)
    {
       dummyNetworkChoice=par("dummyNetworkChoice").intValue();
    }
}

void DecisionMaker::registerNodeToBinder()
{
    binder_ = getBinder();
    // Get our UE
    cModule *ue = getContainingNode(this);
    //Register with the binder
    nodeId_ = binder_->registerNode(ue, UE, 0);
    // Register the nodeId_ with the binder.
    binder_->setMacNodeId(nodeId_, nodeId_);
}

/**
 * To create control info, mandatory to pass message lower layer protocol
 */
void DecisionMaker::setCtrlInfoWithRespectToNetType(cMessage* msg, int networkIndex) {


    string networkTypeName=getNetworkProtocolName(networkIndex);

    if (networkTypeName.find("802") == 0)  { // IEEE 802 protocol family have common ctrlInfo

        cModule* host = inet::getContainingNode(this);
        msg->setControlInfo(Utilities::Ieee802CtrlInfo(host->getFullName()));

    } else if (networkTypeName == "mode4") {
        msg->setControlInfo(Utilities::LteCtrlInfo(nodeId_));

    } else
        throw cRuntimeError(string("Unknown protocol name '" + networkTypeName + "'").c_str());

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
    }
    else
    {
        delete msg;
        EV_INFO<<"interface index doesn't exist" <<endl;
    }
}



void DecisionMaker::sendToUpper(cMessage*  msg)
{
    if (string(msg->getName()).find("hetNets") == 0) {
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
                std::string decisionData= stats->prepareNetAttributes();

                if(decisionData!="")
                {
                    // MCDM here
                    std::cout<< "decision Data "<< decisionData <<"\n" << endl;
                networkIndex = McdaAlg::decisionProcess(decisionData,
                        pathToConfigFiles, "simple", simpleWeights,
                        criteriaType, trafficType, "TOPSIS");

                    std::cout<< "The best network is "<< networkIndex <<"\n" << endl;
                    emit(decisionSignal,networkIndex);
                }
            }
            else
            {
                networkIndex=dummyNetworkChoice;
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

   // handleLteLowerMsg(msg);

}


DecisionMaker::~DecisionMaker()
{
    if(mode4)
    {
        binder_->unregisterNode(nodeId_);
    }

}

void DecisionMaker::handleLteLowerMsg(cMessage* msg)
{
    if(mode4)
    {
        if (msg->isName("CBR")) {
            Cbr* cbrPkt = check_and_cast<Cbr*>(msg);
            //double channel_load = cbrPkt->getCbr();
            //emit(cbr_, channel_load);
            delete cbrPkt;
        }
    }

}




