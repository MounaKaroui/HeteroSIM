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

package heterosim.src.vanetNic;
import inet.linklayer.ieee80211.Ieee80211Nic;

module VanetNic extends Ieee80211Nic
{
    parameters:
        opMode = "p";
        bitrate = default(6 Mbps);
        mgmtType=default("Ieee80211MgmtAdhoc");
        mac.modeSet = opMode;
        mac.qosStation = default(true);
        radio.bandName = "5 GHz";
        radio.bandwidth = 10 MHz;
        
        radio.channelNumber = default(180);
        radio.carrierFrequency = default(5.9 GHz);
        mac.rx.typename = default("heterosim.src.modules.cbrMeasurement.ChannelLoadRx");		
		radio.transmitter.power = 200 mW;
		mac.address = default("auto");
		
        mac.hcf.rateSelection.dataFrameBitrate = bitrate;
        mac.hcf.rateSelection.mgmtFrameBitrate = bitrate;
        mac.hcf.rateSelection.multicastFrameBitrate = bitrate;

        // EDCA settings for activated OCB mode (see table 8-106 in IEEE 802.11-2012)
        mac.hcf.edcaTxopProcedures[*].txopLimit = 0;
////
//        // AC_BK
        mac.hcf.edca.edcaf[0].aifsn = 9;
        mac.hcf.edca.edcaf[0].cwMin = 15;
        mac.hcf.edca.edcaf[0].cwMax = 1023;

        // AC_BE
        mac.hcf.edca.edcaf[1].aifsn = 6;
        mac.hcf.edca.edcaf[1].cwMin = 15;
        mac.hcf.edca.edcaf[1].cwMax = 1023;

        // AC_VI
        mac.hcf.edca.edcaf[2].aifsn = 3;
        mac.hcf.edca.edcaf[2].cwMin = 7;
        mac.hcf.edca.edcaf[2].cwMax = 15;

        // AC_VO
        mac.hcf.edca.edcaf[3].aifsn = 2;
        mac.hcf.edca.edcaf[3].cwMin = 3;
        mac.hcf.edca.edcaf[3].cwMax = 7;
}