/*
 * ChannelLoadAccess.cpp
 *
 *  Created on: Oct 8, 2020
 *      Author: Ali MAMADOU MAMADOU
 */

#include "../../cbrMeasurement/util/ChannelLoadAccess.h"



ChannelLoadAccess::ChannelLoadAccess()
{
}

ChannelLoadAccess::~ChannelLoadAccess()
{
}

double ChannelLoadAccess :: getCBR(){
    return mChannelLoadSampler.cbr();
}
