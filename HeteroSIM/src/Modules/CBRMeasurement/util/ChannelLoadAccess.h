/*
 * ChannelLoadAccess.h
 *
 *  Created on: Oct 8, 2020
 *      Author: Ali MAMADOU MAMADOU
 */

#ifndef MODULES_CBRMEASUREMENT_UTIL_CHANNELLOADACCESS_H_
#define MODULES_CBRMEASUREMENT_UTIL_CHANNELLOADACCESS_H_

#include "ChannelLoadSampler.h"

class  ChannelLoadAccess {

public:
    ChannelLoadAccess();
    ~ChannelLoadAccess();


    double getCBR();

protected:
    artery::ChannelLoadSampler mChannelLoadSampler;


};

#endif /* MODULES_CBRMEASUREMENT_UTIL_CHANNELLOADACCESS_H_ */
