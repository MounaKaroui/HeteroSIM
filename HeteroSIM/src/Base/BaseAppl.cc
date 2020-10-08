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

#include "BaseAppl.h"
#include <inet/common/ModuleAccess.h>

Define_Module(BaseAppl);

void BaseAppl::initialize()
{
    toDecisionMaker =findGate("toDecisionMaker");
    fromDecisionMaker=findGate("toDecisionMaker");

    sendInterval=SimTime(par("sendInterval").doubleValue());
    startTime=SimTime(par("startTime").doubleValue());
    stopTime=SimTime(par("stopTime").doubleValue());

    msgLength=par("msgLength").intValue();

    appID=par("appID").intValue();
    trafficType=par("trafficType").stringValue();
    setNodeId();


    msgSentTrigger=new cMessage("Send trigger");

    if(startTime>=0)
        scheduleAt(startTime, msgSentTrigger);
    else
        throw cRuntimeError("Start time '%d' can not be negative.",SIMTIME_DBL(startTime));

}

void BaseAppl::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        HeterogeneousMessage* vanetMsg = BaseAppl::BuildMsg("hetNets");
        send(vanetMsg, toDecisionMaker);

        if (stopTime >= simTime() || stopTime < 0)
            scheduleAt(simTime() + sendInterval, msgSentTrigger);
    }

}

void BaseAppl::setNodeId()
{
    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    nodeId=Utilities::extractNumber(name.c_str());
}



HeterogeneousMessage* BaseAppl::BuildMsg(std::string namePrefix)
{

    HeterogeneousMessage*  heteroMsg=new HeterogeneousMessage();
    heteroMsg->setName((namePrefix+"-"+std::to_string(heteroMsg->getTreeId())).c_str());

    heteroMsg->setByteLength(msgLength);
    heteroMsg->setTimestamp(simTime());

    heteroMsg->setTrafficType(trafficType.c_str());
    heteroMsg->setApplId(appID);

    heteroMsg->setNodeId(nodeId);

    return  heteroMsg;
}


void BaseAppl::finish()
{
    cancelAndDelete(msgSentTrigger);
}




