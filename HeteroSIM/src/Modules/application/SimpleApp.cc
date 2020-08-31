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

#include "../../Modules/application/SimpleApp.h"
#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>

Define_Module(SimpleApp);


// TODO : develop and general module interface for apps;

void SimpleApp::initialize(int stage)
{

    if(stage==0){
        toDecisionMaker =findGate("toDecisionMaker");
        fromDecisionMaker=findGate("toDecisionMaker");
        updateInterval=par("updateInterval").doubleValue();

        cModule* host = inet::getContainingNode(this);
        std::string name=host->getFullName();
        nodeId=extractNumber(name.c_str());

        msgLength=par("msgLength").intValue();
        selfSender=new cMessage("Send");


        const auto jitter = uniform(SimTime(0, SIMTIME_MS), updateInterval);
        scheduleAt(simTime() + jitter + updateInterval, selfSender);
    }

}

int SimpleApp::extractNumber(std::string input)
{
    size_t i = 0;
    for ( ; i < input.length(); i++ ){ if ( isdigit(input[i]) ) break; }
    // remove the first chars, which aren't digits
    input = input.substr(i, input.length() - i );
    // convert the remaining text to an integer
    int id = atoi(input.c_str());
    return id;
}


HeterogeneousMessage* SimpleApp::BuildMsg(int networkType, std::string name)
{

    HeterogeneousMessage*  heteroMsg=new HeterogeneousMessage();
    heteroMsg->setName(name.c_str());
    heteroMsg->setByteLength(msgLength);
    heteroMsg->setTimestamp(simTime());
    heteroMsg->setNetworkType(networkType); // to be overrrided in decider module
    heteroMsg->setNodeId(nodeId);
    return  heteroMsg;
}

void SimpleApp::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        // vanet Msg
        HeterogeneousMessage* vanetMsg=BuildMsg(VANET, "hetNets");
        send(vanetMsg, toDecisionMaker);

        // mode4 Msg
        //HeterogeneousMessage* mode4Msg=BuildMsg(MODE4, "hetNets msg");
        //send(mode4Msg, toDecisionMaker);

        // wsn Msg
        //HeterogeneousMessage* wsnMsg=BuildMsg(WSN, "WSN msg");
        //simtime_t  delay=uniform(SimTime(0, SIMTIME_MS), updateInterval);
        //sendDelayed(wsnMsg, delay/2, toDecisionMaker);

        scheduleAt(simTime()+updateInterval, selfSender);
    }
    else
    {
       // HeterogeneousMessage* received=dynamic_cast<HeterogeneousMessage*>(msg);
      // EV_INFO<< msg->getFullName()<< "from "<< received->getSourceAddress()<<endl;
    }
}

void SimpleApp::finish()
{
    cancelAndDelete(selfSender);
}

