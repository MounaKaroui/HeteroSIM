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

package heterosim.Modules.Mode4Driver;

//
// TODO auto-generated module
//

import lte.apps.mode4App.Mode4BaseApp;
simple Mode4Driver  like Mode4BaseApp
{
    
    parameters:
        int macNodeId = default(0);
         @signal[cbr];
		@statistic[cbr](title="Channel Busy Ratio"; unit=""; source="cbr"; record=vector); 
    gates:
        
        output toDecider;
        input  fromDecider;
        
        input lowerGateIn; // FROM NIC
        output lowerGateOut;  // TO NIC
}
