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

package heterosim.src.Modules.CBRMeasurement;

import inet.linklayer.csma.CSMA;


simple ChannelLoadCSMA extends CSMA
{
    parameters:
        
        @class(ChannelLoadCSMA);
        @signal[ChannelLoad](type=double);

        double channelReportInterval @unit(ms) = default(100ms); //TODO fix default
        double channelSensingDuration @unit(ms) = default(ccaDetectionTime); 
        double channelSensingInterval @unit(ms) = default(10ms); //TODO fix default
        bool asyncChannelReport = default(true); //TODO important or not     
}
