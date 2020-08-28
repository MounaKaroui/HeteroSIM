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

package heterosim.simulations.networks;

import inet.environment.common.PhysicalEnvironment;
import heterosim.Modules.nodes.HeterogeneousCarSimpleModel;
import heterosim.Modules.nodes.car;
import inet.physicallayer.common.packetlevel.RadioMedium;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedVisualizer;

network SimpleTest
{
    parameters:
        int numCars = default(2);
        @display("bgb=503,338");

    submodules:
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=49.704147,85.20711");
        }
        
        configurator: IPv4NetworkConfigurator {
            @display("p=49.704147,35.502964");
        }
        
        car: car {
            @display("p=109,119;is=s");
        }
        
        physicalEnvironment: PhysicalEnvironment {
            @display("p=49.704147,135.50298");
        }
}
