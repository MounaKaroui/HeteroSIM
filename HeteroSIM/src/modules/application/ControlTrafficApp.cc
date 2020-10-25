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

#include "../../modules/application/ControlTrafficApp.h"

Define_Module(ControlTrafficApp);

void ControlTrafficApp::initialize()
{

    BaseAppl::initialize();
    interfaceId=par("interfaceId").intValue();
}

void ControlTrafficApp::handleAppMessage(cMessage *msg)
{
     delete msg;
}


BasicMsg* ControlTrafficApp::BuildMsg(std::string namePrefix)
{
    ControlMsg*  ctrlMsg=new ControlMsg();
    ctrlMsg->setName((namePrefix+string("-controlTraffic-")+std::to_string(ctrlMsg->getTreeId())).c_str());
    ctrlMsg->setNetworkId(interfaceId);
    ctrlMsg->setApplId(appID);
    ctrlMsg->setByteLength(msgLength);
    ctrlMsg->setApplId(appID);
    return  ctrlMsg;
}

