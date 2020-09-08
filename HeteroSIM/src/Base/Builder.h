/*
 * Builder.h
 *
 *  Created on: Aug 31, 2020
 *      Author: mouna1
 */

#ifndef BASE_BUILDER_H_
#define BASE_BUILDER_H_

#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "common/LteControlInfo.h"
#include <inet/common/ModuleAccess.h>
using namespace inet;



namespace Builder {

Ieee802Ctrl* Ieee802CtrlInfo(std::string moduleName);
FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_);
int extractNumber(std::string input);


} /* namespace Builder */

#endif /* BASE_BUILDER_H_ */
