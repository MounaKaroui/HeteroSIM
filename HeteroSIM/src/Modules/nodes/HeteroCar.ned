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
import heterosim.Base.IBaseAppl;
import heterosim.Modules.DecisionMaker.DecisionMaker;
import inet.common.queue.DropTailQueue;
import inet.linklayer.ieee802154.Ieee802154NarrowbandNic;
import inet.linklayer.ieee802154.Ieee802154UWBIRNic;
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
import inet.node.inet.INetworkNode;
import lte.apps.mode4App.Mode4BaseApp;
import lte.stack.phy.ILteNic;
import inet.linklayer.ieee80211.mac.contract.IStatistics;


module car
{
    parameters:
        @display("i=block/wtx;bgb=1188.506,699.264;is=vs");
        @networkNode();
        @labels(node,ethernet-node,wireless-node);

        int numApps = default(1);
        string wlanType = default("Ieee80211Nic");
        int numRadios = default(1);
        string mobilityType = default("LinearMobility");

        *.interfaceTableModule = default(absPath(".interfaceTable"));

        bool withMode4 = default(false);
        /// LTE params
        int masterId = default(0); // Not yet removed from SimuLTE for Mode 4 simulations
        int macNodeId = default(0); // TODO: this is not a real parameter
        //int macCellId = default(0); // TODO: this is not a real parameter

        *.macModule80215 = absPath(".wlan[1].mac");
        *.radioModule80215 = absPath(".wlan[1].radio");
        
		*.macModule80211 = absPath(".wlan[0].mac");
        *.radioModule80211 = absPath(".wlan[0].radio");
        
        *.radioModuleLte=absPath(".lteNic.phy");

    gates:
        input radioIn[numRadios] @directIn;
       // inout pppg[] @labels(PPPFrame-conn);
       // inout ethg[] @labels(EtherFRame-conn);

        input lteRadioIn @directIn;


    submodules:
        wlan[numRadios]: <default("Ieee80211Nic")> like IWirelessNic {
            @display("p=220.51999,402.3,row,100;q=queue");

        }
        mobility: <mobilityType> like IMobility {
            @display("p=1050.45,84.92999");
        }
        interfaceTable: InterfaceTable {
            @display("p=1066.84,435.08;is=s");
        }

        decisionMaker: DecisionMaker {
            @display("p=402.966,223.992");

        }
        applLayer[numApps]: <default("VanetApp")> like IBaseAppl {
            @display("p=402.3,38.739998,row,200;q=queue,i,block/app2");
        }


        lteNic: <default("LteNicVUeMode4")> like ILteNic if withMode4 {
            @display("p=615.37,402.3");
            //d2dCapable = true;
            nodeType = "UE";  // DO NOT CHANGE
        }

        statistics: <default("CollectStats")> like IStatistics {
            parameters:
                @display("p=1050.45,223.49998;is=s");

        }
    connections allowunconnected:
        for i=0..numApps -1 {
            decisionMaker.toApplication++ --> applLayer[i].fromDecisionMaker;
            applLayer[i].toDecisionMaker --> decisionMaker.fromApplication++;
        }

		// with Mode4


        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            decisionMaker.toRadio++ --> wlan[i].upperLayerIn;
            wlan[i].upperLayerOut --> decisionMaker.fromRadio++;
        }


        if (withMode4) {
            lteNic.radioIn <-- lteRadioIn;
            decisionMaker.toRadio++ --> lteNic.upperLayerIn;
            lteNic.upperLayerOut --> decisionMaker.fromRadio++;
        }




}
