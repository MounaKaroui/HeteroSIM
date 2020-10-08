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

#include "VanetApp.h"

#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>

Define_Module(VanetApp);


// Use BaseApp  to create an App



void VanetApp::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
        BaseAppl::handleMessage(msg);
    else
    {
        int arrivalGate=msg->getArrivalGateId();
        if(arrivalGate==fromDecisionMaker)
        {
            // received from decider
            // TODO add stats
        }

    }
}

