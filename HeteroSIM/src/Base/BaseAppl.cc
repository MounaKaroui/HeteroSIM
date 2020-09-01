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
    setNodeId();
}

void BaseAppl::setNodeId()
{
    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    nodeId=extractNumber(name.c_str());
}


HeterogeneousMessage* BaseAppl::BuildMsg(int networkType, std::string name)
        {

            HeterogeneousMessage*  heteroMsg=new HeterogeneousMessage();
            heteroMsg->setName(name.c_str());
            heteroMsg->setByteLength(msgLength);
            heteroMsg->setTimestamp(simTime());
            heteroMsg->setNetworkType(networkType); // to be overrrided in decider module
            heteroMsg->setNodeId(nodeId);
            return  heteroMsg;
        }

int BaseAppl::extractNumber(std::string input)
{
    size_t i = 0;
    for ( ; i < input.length(); i++ ){ if ( isdigit(input[i]) ) break; }
    // remove the first chars, which aren't digits
    input = input.substr(i, input.length() - i );
    // convert the remaining text to an integer
    int id = atoi(input.c_str());
    return id;
}





