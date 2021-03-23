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

import heterosim.src.modules.nodes.car;
import heterosim.src.modules.nodes.GenericNode;

import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.physicallayer.common.packetlevel.RadioMedium;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.node.ethernet.Eth10G;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

import lte.corenetwork.binder.LteBinder;
import lte.world.radio.LteChannelControl;
import lte.corenetwork.nodes.eNodeB;
import lte.epc.PgwStandardSimplified;


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
                @display("p=52,49");
                mediumLimitCache.carrierFrequency = 5.9GHz;
        }



        car[numCars]: car {
            @display("p=276,202;is=s");
        }

//        visualizer: IntegratedCanvasVisualizer if visualize {
//            @display("p=50,50");
//        }

        //channelControl: LteChannelControl if mode4isActive {
          //  @display("p=60,268;is=s");
        //}


        //binder: LteBinder if mode4isActive {
          //  @display("p=61,179;is=s");
        //}


        //deployer: LteDeployer if mode4isActive {
          //  @display("p=53,352;is=s");
        //}

}


network SimpleNetwork2
{
    parameters:
        int numNodes = default(1);
        @display("bgb=587,415;i=background/budapest");


    submodules:

		//# WiFi interfaces related submodules 	
        radioMedium: <default("Ieee80211ScalarRadioMedium")> like IRadioMedium {
            parameters:
               // mediumLimitCache.carrierFrequency = 5.9GHz;
        }

        //# LTE interface related submodules 	
        channelControl: LteChannelControl;
        routingRecorder: RoutingTableRecorder { //Needed only to initialize eNB layers. TODO seek into removing this dependency
            enabled = default(false);
        }
        pgw: PgwStandardSimplified { // Also needed only to initialize eNB layers. TODO seek into removing this dependency
            nodeType = "PGW";
        }
        binder: LteBinder;
        configurator: IPv4NetworkConfigurator; // Also needed only to initialize eNB layers. TODO seek into removing this dependency
        eNB: eNodeB;

		//# Common scenario submodules
        node[numNodes]: GenericNode {
            @display("p=276,202;is=s");
        }

    connections allowunconnected:
}
