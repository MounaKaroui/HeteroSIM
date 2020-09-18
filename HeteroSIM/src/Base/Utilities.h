/*
 * Builder.h
 *
 *  Created on: Aug 31, 2020
 *      Author: mouna1
 */

#ifndef BASE_UTILITIES_H_
#define BASE_UTILITIES_H_

#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "common/LteControlInfo.h"
#include <inet/common/ModuleAccess.h>
using namespace inet;



namespace Utilities {

Ieee802Ctrl* Ieee802CtrlInfo(std::string moduleName);
FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_);
int extractNumber(std::string input);
double calculateEWA_BiasCorrection(std::vector<double> crit, double beta);

} /* namespace Builder */

#endif /* BASE_UTILITIES_H_ */
