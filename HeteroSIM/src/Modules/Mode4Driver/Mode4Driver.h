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

#ifndef __HETEROSIM_MODE4DRIVER_H_
#define __HETEROSIM_MODE4DRIVER_H_

#include <omnetpp.h>

#include "apps/mode4App/Mode4BaseApp.h"
#include "corenetwork/binder/LteBinder.h"
#include "Modules/messages/HeterogeneousMessage_m.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Mode4Driver : public Mode4BaseApp {

    public:
        ~Mode4Driver() override;

    protected:
        simsignal_t cbr_;
        LteBinder* binder_;
        MacNodeId nodeId_;

        int fromDecider;
        int toDecider;
        int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        void handleLowerMessage(cMessage* msg);
        void handleMessage(cMessage* msg);
        void sendLowerPackets(cPacket* pkt);
        void finish();
        void handleSelfMessage(cMessage* msg);
};

#endif
