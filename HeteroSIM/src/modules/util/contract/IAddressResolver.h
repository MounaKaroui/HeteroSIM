/*
 * IAddressResolver.h
 *
 *  Created on: Mar 5, 2021
 *      Author: ali
 */

#ifndef MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_
#define MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_


#include "inet/linklayer/common/MACAddress.h"
#include "common/LteCommon.h"

using namespace inet;

class IAddressResolver {
public:
    virtual ~IAddressResolver() {}


    virtual MACAddress resolveIEEE802Address(const char * hostName, int interfaceId) = 0;
    virtual MacNodeId resolveLTEMacAddress(const char * hostName) = 0; //"interfaceId" parameter is not needed parameter because multiple LTE interfaces within a node is note supported
};

#endif /* MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_ */
