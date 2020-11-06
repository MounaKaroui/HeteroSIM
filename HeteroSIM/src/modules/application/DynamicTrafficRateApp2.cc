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

#include "DynamicTrafficRateApp2.h"

Define_Module(DynamicTrafficRateApp2);

void DynamicTrafficRateApp2::initialize()
{
    DynamicTrafficRateApp::initialize();
    nbrNodes=par("nbrNodes");
}

void DynamicTrafficRateApp2::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {

        if(msg==msgSentTrigger){

            offeredLoadRateSentInterval= SimTime(par("offeredLoadRate").doubleValue());
            string s= getEnvir()->getConfig()->getConfigValue("sim-time-limit");
            double  simTimeMax =std::stod(s.substr(0,s.length()-1));

            if(simTime()<=SimTime(simTimeMax/2))
            {
                // increasing
                offeredLoadRateSentInterval=SimTime(nbrNodes*(msgLength*8/bitrate)*(simTimeMax/(simTime().dbl()+ 0.000001))* 0.000001);
            }else
            {
                // decreasing
                offeredLoadRateSentInterval=SimTime(nbrNodes*(msgLength*8/bitrate)* ( 1 / (1- (simTime().dbl()/simTimeMax )))*0.000001);
            }
            pcktToSendForOfferedLoadRate = SimTime(1) / offeredLoadRateSentInterval;
            emit(pcktToSendSignal,pcktToSendForOfferedLoadRate);

            if(pcktToSendForOfferedLoadRate > 0)
                scheduleAt(simTime() + offeredLoadRateSentInterval, msgOfferedLoadRateSentTrigger);
            else
                scheduleAt(simTime() + offeredLoadRefreshInterval, msgSentTrigger); //schedule next offered load rate generation time

        }else if (msg==msgOfferedLoadRateSentTrigger){

            sendData();
            pcktToSendForOfferedLoadRate--;

            if(pcktToSendForOfferedLoadRate > 0 )
                scheduleAt(simTime() + offeredLoadRateSentInterval, msgOfferedLoadRateSentTrigger);
            else
                scheduleAt(simTime() + offeredLoadRefreshInterval, msgSentTrigger); //schedule next offered load rate generation time
        }

    }else
        DynamicTrafficRateApp::handleAppMessage(msg);

}
