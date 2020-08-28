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

#ifndef __HETEROSIM_RADIODRIVERBASE_H_
#define __HETEROSIM_RADIODRIVERBASE_H_

#include <omnetpp/ccomponent.h>
#include <omnetpp/cmessage.h>
#include <omnetpp/csimplemodule.h>

using namespace omnetpp;

/**
 * TODO - Generated class
 */

class RadioDriverBase : public cSimpleModule
{
public:
       static const omnetpp::simsignal_t ChannelLoadSignal;

       virtual void initialize() override;
       virtual void handleMessage(omnetpp::cMessage*) override;

   protected:
       void indicateData(omnetpp::cMessage*);
       bool isDataRequest(omnetpp::cMessage*);
       virtual void handleDataRequest(omnetpp::cMessage*) = 0;

   private:
       omnetpp::cGate* mUpperLayerIn;
       omnetpp::cGate* mUpperLayerOut;
       omnetpp::cGate* mPropertiesOut;
};


#endif
