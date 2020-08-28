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
// @author  from Artery git repo

#include "../../Modules/radioDriverBase/RadioDriverBase.h"





void RadioDriverBase::initialize()
{
    mUpperLayerIn = gate("upperLayer$i");
    mUpperLayerOut = gate("upperLayer$o");
    mPropertiesOut = gate("properties");
}

void RadioDriverBase::handleMessage(cMessage* msg)
{
    if (isDataRequest(msg)) {
        handleDataRequest(msg);
    } else {
        throw cRuntimeError("unexpected message");
    }
}

bool RadioDriverBase::isDataRequest(cMessage* msg)
{
    return (msg->getArrivalGate() == mUpperLayerIn);
}

void RadioDriverBase::indicateData(cMessage* msg)
{
    send(msg, mUpperLayerOut);
}

