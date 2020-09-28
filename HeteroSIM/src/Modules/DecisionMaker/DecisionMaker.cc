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

    critNumb=par("criteriaNum").intValue();
    mode4=par("mode4IsActive").boolValue();
    pathToConfigFiles=par("pathToConfigFiles").stringValue();
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


void DecisionMaker::ctrlInfoWithRespectToNetType(cMessage* msg, int networkType)
{


    if(networkType==MODE4)
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
    HeterogeneousMessage* hetNetsMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    int id=hetNetsMsg->getApplId();
    int gateId=gate("toApplication",id)->getId(); // To get Id
    send(msg, gateId);
}



void DecisionMaker::networkInit(int& networkIndex)
{

//        if(simTime()<10)
//        {
//            networkIndex=0;
//
//        }else if(simTime()>10 and simTime()<20)
//        {
//            networkIndex=1;
//        }else if(simTime()>20 and simTime()<50)
//        {
//            networkIndex=2;
//        }
//        else
//        {
           networkIndex=rand()%3;
      //  }

}

int DecisionMaker::takeDecision(cMessage* msg)
{

    int networkIndex; // generate values between 0 and 2, Just for test
    // MCDM_procedure
    HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    std::string trafficType=hetMsg->getTrafficType();

    cModule* mStats=getParentModule()->getSubmodule("collectStatistics");
    CollectStats* stats=dynamic_cast<CollectStats*>(mStats);

    networkInit(networkIndex);
    if(isDeciderActive)
    {
        if(stats->allPathsCriteriaValues!="")
        {
            // TODO call MCDM here
            // allPathsCriteriaValues is the final list of criteria

            //networkIndex=McdaAlg::decisionProcess(stats->allPathsCriteriaValues, pathToConfigFiles,critNumb, trafficType, "VIKOR");
            //std::cout<< "The best network is "<< networkIndex <<"\n"<< endl;
        }
    }
    else
    {

        networkIndex=dummyNetworkChoice;

    }


    return networkIndex;
}


void DecisionMaker::handleMessage(cMessage *msg)
{

    HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    int id;


    if(hetMsg!=nullptr)
    {
    id=hetMsg->getApplId();


    int arrivalGate = msg->getArrivalGateId();
    int gateId=gate("fromApplication",id)->getId();

    if (arrivalGate == gateId)
    {
        int networkIndex=takeDecision(msg);
        sendToLower(msg, networkIndex);

    }else
    {
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




