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


void BaseAppl::initialize()
{
    toDecisionMaker =findGate("toDecisionMaker");
    fromDecisionMaker=findGate("toDecisionMaker");

    updateInterval=par("updateInterval").doubleValue();
    msgLength=par("msgLength").intValue();

    appIndex=par("appIndex").intValue();
    appName=par("appName").stringValue();
    setNodeId();
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

            heteroMsg->setTrafficType(appName.c_str());
            heteroMsg->setApplId(appIndex);

            heteroMsg->setNodeId(nodeId);

            return  heteroMsg;
        }





