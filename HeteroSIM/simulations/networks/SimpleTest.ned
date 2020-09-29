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
import heterosim.src.Modules.nodes.car;
import inet.physicallayer.common.packetlevel.RadioMedium;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.physicallayer.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
//import inet.visualizer.integrated.IntegratedVisualizer;
//import inet.visualizer.integrated.IntegratedCanvasVisualizer;


import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.deployer.LteDeployer;
import lte.world.radio.LteChannelControl;

network SimpleNetwork
{
    parameters:
        int numCars = default(1);
        @display("bgb=587,415;i=background/budapest");
        bool visualize = default(false);
        bool mode4isActive = default(false);
    submodules:


        radioMedium: <default("Ieee80211ScalarRadioMedium")> like IRadioMedium {
            parameters:
                @display("p=60,20");
                mediumLimitCache.carrierFrequency = 5.9GHz;
        }



        car[numCars]: car {
            @display("p=276,202;is=s");
        }

//        visualizer: IntegratedCanvasVisualizer if visualize {
//            @display("p=50,50");
//        }

        channelControl: LteChannelControl if mode4isActive {
            @display("p=60,268;is=s");
        }


        binder: LteBinder if mode4isActive {
            @display("p=61,179;is=s");
        }


        deployer: LteDeployer if mode4isActive {
            @display("p=53,352;is=s");
        }

//        narrowbandScalarRadioMedium: Ieee802154NarrowbandScalarRadioMedium {
//            @display("p=392,328");
//        }
}
