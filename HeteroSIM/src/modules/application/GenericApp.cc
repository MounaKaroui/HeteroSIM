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

#include "../../modules/application/GenericApp.h"

#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>

Define_Module(GenericApp);


// Use BaseApp  to create an App



void GenericApp::initialize()
{
    BaseAppl::initialize();
    trafficType=par("trafficType").stringValue();
}

void GenericApp::handleAppMessage(cMessage *msg)
{
    if(string(msg->getName()).find("hetNets-data")==0)
        emit(BaseAppl::rcvdPacket, msg);
   delete msg;
}

BasicMsg* GenericApp::BuildMsg(std::string namePrefix)
{

    HeterogeneousMessage*  heteroMsg=new HeterogeneousMessage();
    heteroMsg->setName((namePrefix+string("-data-")+std::to_string(heteroMsg->getTreeId())).c_str());
    heteroMsg->setByteLength(msgLength);
    heteroMsg->setTimestamp(simTime());
    heteroMsg->setTrafficType(trafficType.c_str());
    heteroMsg->setApplId(appID);
    heteroMsg->setNodeId(nodeId);
    heteroMsg->setSourceAddress(inet::getContainingNode(this)->getFullName());
    heteroMsg->setDestinationAddress(destAddress.c_str());

    return  heteroMsg;
}


