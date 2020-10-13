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

#include "../../Modules/DecisionMaker/DecisionMaker.h"
#include <inet/common/ModuleAccess.h>
#include "stack/phy/packet/cbr_m.h"
#include <stdlib.h>     /* srand, rand */

#include "../Stats/CollectStats.h"
Define_Module(DecisionMaker);




void DecisionMaker::initialize()
{

    mode4=par("mode4IsActive").boolValue();
    pathToConfigFiles=par("pathToConfigFiles").stringValue();
    simpleWeights=par("simpleWeights").stringValue();
    criteriaType=par("criteriaType").stringValue();
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


bool  DecisionMaker::isMode4InterfaceAvailable()
{
    cModule* mStats=getParentModule()->getSubmodule("collectStatistics");
    CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
    std::vector<int> result;
    bool searchResult=Utilities::findKeyByValue(result, stats->interfaceToProtocolMap, string("mode4"));
    return searchResult;
}

void DecisionMaker::ctrlInfoWithRespectToNetType(cMessage* msg, int networkType)
{

    if(isMode4InterfaceAvailable())
    {
        msg->setControlInfo(Utilities::LteCtrlInfo(nodeId_));

    }else
    {
        // WLAN
        cModule* host = inet::getContainingNode(this);
        std::string name=host->getFullName();
        // Control info is mandatory to pass message
        //to Mgmt layer and then to MAC
        msg->setControlInfo(Utilities::Ieee802CtrlInfo(name));
    }

}
void DecisionMaker:: sendToLower(cMessage*  msg, int networkIndex)
{

    ctrlInfoWithRespectToNetType(msg, networkIndex);
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
    BasicMsg* hetNetsMsg=dynamic_cast<BasicMsg*>(msg);
    int id=hetNetsMsg->getApplId();
    int gateId=gate("toApplication",id)->getId(); // To get Id
    send(msg, gateId);
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
                        criteriaType, trafficType, "VIKOR");

                    std::cout<< "The best network is "<< networkIndex <<"\n" << endl;
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

    BasicMsg* hetMsg = dynamic_cast<BasicMsg*>(msg);

    if (hetMsg != nullptr) {

        cGate* gate = msg->getArrivalGate();
        std::string gateName = gate->getName();

        if (gateName.find("fromApplication")==0) {
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




