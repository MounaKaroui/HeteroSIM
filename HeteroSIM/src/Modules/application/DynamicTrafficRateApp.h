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

#ifndef MODULES_APPLICATION_DYNAMICTRAFFICRATEAPP_H_
#define MODULES_APPLICATION_DYNAMICTRAFFICRATEAPP_H_

#include <omnetpp.h>
#include "Base/BaseAppl.h"

class DynamicTrafficRateApp : public BaseAppl {

public:
    virtual ~DynamicTrafficRateApp();


protected:

    int interfaceId;

    int pcktToSendForOfferedLoadRate=0;

    simtime_t offeredLoadRateSentInterval;
    simtime_t offeredLoadRefreshInterval;

    cMessage* msgOfferedLoadRateSentTrigger;

    simsignal_t pcktToSendSignal;

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    virtual void handleAppMessage(cMessage *msg) override;
    virtual BasicMsg* BuildMsg(std::string namePrefix) override;

    void sendData ();
    void setOfferedLoadRefreshInterval();

};

#endif /* MODULES_APPLICATION_DYNAMICTRAFFICRATEAPP_H_ */
