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
        mac.qosStation = default(false);
        radio.bandName = "5 GHz";
        radio.bandwidth = 10 MHz;
        
        radio.channelNumber = default(180);
        radio.carrierFrequency = default(5.9 GHz);
        mac.rx.typename = default("heterosim.src.modules.cbrMeasurement.ChannelLoadRx");		
		radio.transmitter.power = 200 mW;
		mac.address = default("auto");

}