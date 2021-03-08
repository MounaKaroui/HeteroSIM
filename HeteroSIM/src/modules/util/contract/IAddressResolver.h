/*
 * IAddressResolver.h
 *
 *  Created on: Mar 5, 2021
 *      Author: ali
 */

#ifndef MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_
#define MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_


#include "inet/linklayer/common/MACAddress.h"

using namespace inet;

class IAddressResolver {
public:
    virtual ~IAddressResolver() {}


    virtual MACAddress resolveIEEE802Address(const char * hostName, int interfaceId) = 0;
};

#endif /* MODULES_UTIL_CONTRACT_IADDRESSRESOLVER_H_ */
