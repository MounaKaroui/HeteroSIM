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

#ifndef __HETEROSIM_ADRESSRESOLVER_H_
#define __HETEROSIM_ADRESSRESOLVER_H_

#include <omnetpp.h>

#include "modules/util/contract/IAddressResolver.h"

#include "corenetwork/binder/LteBinder.h"

#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/ModuleAccess.h"

using namespace omnetpp;
using namespace inet ;

/**
 * TODO - Generated class
 */
class AddressResolver : public cSimpleModule, public IAddressResolver
{

public :
    typedef std::map<const InterfaceEntry*,const char *> IEEE80211ResolveCache;


    virtual MACAddress resolveIEEE802Address(const char * hostName, int interfaceId);

    virtual MacNodeId resolveLTEMacAddress(const char * hostName);


  protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    IInterfaceTable *ift = nullptr;
    cModule *host  = nullptr ;

    static IEEE80211ResolveCache globalIeee80211ResolveCache;

    LteBinder* binder_; //

  public :
     AddressResolver();
     ~AddressResolver();
     virtual int numInitStages() const override { return NUM_INIT_STAGES; }


};

#endif
