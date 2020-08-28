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

package heterosim.Modules.nodes;
import heterosim.Modules.DecisionMaker.DecisionMaker;
import heterosim.Modules.application.SimpleApp;
import heterosim.Modules.radioDriver.RadioDriver;
import inet.node.inet.WirelessHost;


import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

import inet.applications.contract.IUDPApp;
import inet.transportlayer.contract.IUDP;

import inet.linklayer.contract.IWirelessNic;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.ipv4.IPv4;
import inet.networklayer.ipv4.IPv4RoutingTable;
import inet.transportlayer.udp.UDP;

module car
{
    parameters:
        @display("i=block/wtx;bgb=805.156,545.184;is=vs");
        int numApps = default(1);
        string wlanType = default("Ieee80211Nic");
        int numRadios = default(1);
        string mobilityType = default("LinearMobility");
        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
        input radioIn[numRadios] @directIn;


    submodules:
        wlan[numRadios]: <wlanType> like IWirelessNic {
            @display("p=607.022,350.836");
            *.opMode = "p";
        }
        mobility: <mobilityType> like IMobility {
            @display("p=76.982,426.556");
        }
        radioDriver[numRadios]: RadioDriver {
            @display("p=332.958,274.72202;i=block/layer");
        }
        interfaceTable: InterfaceTable {
            @display("p=76.982,330.644;is=s");
        }

        decisionMaker: DecisionMaker {
            @display("p=332.958,159.516");
        }
        simpleApp[numApps]: SimpleApp {
            @display("p=332.958,34.182;i=block/app2");
        }
    connections allowunconnected:

        // Apps to decider 
        for i=0..sizeof(numApps)-1 {
            simpleApp[i].toDecisionMaker --> decisionMaker.fromApplication;
            decisionMaker.toApplication --> simpleApp[i].fromDecisionMaker;
        }

        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            // radio deriver to WLAN
            wlan[i].upperLayerOut --> radioDriver[i].lowerLayerIn;
            wlan[i].upperLayerIn <-- radioDriver[i].lowerLayerOut;
            // decider to radio driver 
            decisionMaker.ifOutRadio++ --> radioDriver[0].lowerLayerIn;
            radioDriver[0].properties --> decisionMaker.ifInRadio++;
        }


}
