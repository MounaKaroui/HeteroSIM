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

package heterosim.src.modules.nodes;

import heterosim.src.base.IBaseAppl;
import heterosim.src.modules.decisionMaker.DecisionMaker;
import heterosim.src.modules.util.AddressResolver;

import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.contract.IWirelessNic;
import inet.linklayer.ieee80211.mac.contract.IStatistics;


import lte.stack.ILteNic;


module GenericNode
{
    parameters:
        @display("i=block/wtx;bgb=1188.506,699.264;is=vs");
        @networkNode();
        @labels(node,ethernet-node,wireless-node);

        //# Common parameters 
        int numApps = default(1);
        string mobilityType = default("LinearMobility");
        *.addressResolver = default(absPath(".addressResolver"));

		//# WiFi interfaces related parameters
        int numWiFiRadios = default(1);
        *.interfaceTableModule = default(absPath(".interfaceTable"));

		//# LTE interfaces related parameters        
        bool lteInterfaceIsActive = default(false);
        int macNodeId = default(0); // This is not a real parameter, but the LteBinder assigns a correct value from c++ code
        int masterId = default(1); // This is the MAC identifier of the relay node of this node which is typically the eNB. This identifier is assigned by the binder to the relay node incrementally as they are initialized in the simulation scene.  (see LteBinder::registerNode: Line 49-60) 
        int macCellId = default(0); // TODO: this is not a real parameter


    gates:
        input wifiRadioIn[numWiFiRadios] @directIn;

        input lteRadioIn @directIn; // multiple LTE interfaces within a single node not supported. See why in LteBinder::registerNode method.


    submodules:

        //# WiFi interfaces related submodules 		
        wlan[numWiFiRadios]: <default("Ieee80211Nic")> like IWirelessNic {
            @display("p=220.51999,402.3,row,100;q=queue");
            mgmtType = default("Ieee80211MgmtAdhoc");  // use adhoc management
            rx.typename = default("heterosim.src.modules.cbrMeasurement.ChannelLoadRx");
        }
        mobility: <mobilityType> like IMobility {
            @display("p=56.62,67.049995");
        }
        interfaceTable: InterfaceTable {
            @display("p=56.62,312.9;is=s");
        }

        //# LTE interface related submodules 	
        lteNic: <default("LteNicUeD2DExperimental")> like ILteNic {
            @display("p=482.75998,411.24");
        }

		//# Common submodules
        addressResolver: AddressResolver {
            @display("p=56.62,469.34998");
        }
        collectStatistics: <default("CollectStats")> like IStatistics {
             @display("p=56.62,186.24998;is=s");
        }
        decisionMaker: DecisionMaker {
            @display("p=402.966,223.992");
        }
        applLayer[numApps]: <default("VanetApp")> like IBaseAppl {
            @display("p=402.3,38.739998,row,200;q=queue,i,block/app2");
        }


    connections allowunconnected:
        for i=0..numApps -1 {
            decisionMaker.toApplication++ --> applLayer[i].fromDecisionMaker;
            applLayer[i].toDecisionMaker --> decisionMaker.fromApplication++;
        }

        for i=0..numWiFiRadios-1 {
            wifiRadioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            decisionMaker.toRadio++ --> wlan[i].upperLayerIn;
            wlan[i].upperLayerOut --> decisionMaker.fromRadio++;
        }

        if (lteInterfaceIsActive) {
            decisionMaker.toRadio++ --> lteNic.upperLayerIn;
            decisionMaker.fromRadio++ <-- lteNic.upperLayerOut;
            lteNic.radioIn <-- lteRadioIn;
        }
}
