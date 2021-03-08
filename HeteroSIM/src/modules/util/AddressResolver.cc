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

#include "AddressResolver.h"

#include "../../base/Utilities.h"

AddressResolver::IEEE80211ResolveCache AddressResolver::globalIeee80211ResolveCache;

Define_Module(AddressResolver);

AddressResolver::AddressResolver(){
    ift = nullptr;
}

void AddressResolver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_NETWORK_LAYER_3 ) { // interfaces should be available

        WATCH_PTRMAP(globalIeee80211ResolveCache);

        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        ASSERT((host = findContainingNode(this)) != nullptr);

        // register our IEEE 802 addresses in the global cache
        for (int i = 0; i < ift->getNumInterfaces(); i++) {
            const InterfaceEntry *ie = ift->getInterface(i);

            if (ie->isLoopback())
                continue;

            globalIeee80211ResolveCache.insert({ie,host->getFullName()});
        }
    }
}


MACAddress AddressResolver::resolveIEEE802Address(const char *hostName, int interfaceId) {

    for (auto it = globalIeee80211ResolveCache.begin(); it != globalIeee80211ResolveCache.end();) {

        bool hostNameMatches = !strcmp(it->second, hostName);
        bool interfaceIdMatches = Utilities::extractNumber( it->first->getInterfaceModule()->getFullName()) == interfaceId;

        if (hostNameMatches && interfaceIdMatches) {
            return it->first->getMacAddress();
        }
        else
            ++it;
    }

    ASSERT2(false,"Cannot resolve address"); // TODO add exception details
}


void AddressResolver::handleMessage(cMessage *msg)
{
   ASSERT(false);
}

AddressResolver::~AddressResolver() {

    // delete my 802.11 interface entries from the globalCache
    for (auto it = globalIeee80211ResolveCache.begin(); it != globalIeee80211ResolveCache.end();) {
        if (it->second == host->getName()) {
            auto cur = it++;
            delete cur->second;
            globalIeee80211ResolveCache.erase(cur);
        } else
            ++it;
    }
}
