// This car module provides two interfaces for sending and receiving messages via LTE and/or DSRC.

package heterosim.Modules.nodes;

import heterosim.Modules.DecisionMaker.DecisionMaker;
import heterosim.Modules.application.SimpleApp;
import inet.linklayer.contract.IWirelessNic;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.ipv4.IPv4;
import inet.networklayer.ipv4.IPv4RoutingTable;
import inet.transportlayer.udp.UDP;
import lte.stack.phy.ILteNic;
import lte.corenetwork.nodes.*;
import lte.stack.phy.LteNicUe;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

module HeterogeneousCarSimpleModel
{
    parameters:
        @node();
        @display("bgb=769.974,704.74603");
        //string applType;           //type of the application layer
        string wlanType = default("Ieee80211Nic");
        int numRadios = default(1);
        string mobilityType = default("LinearMobility"); // TODO 
        int numApps = default(1);


    gates:
        input radioIn[numRadios] @directIn; //LTE input gate


    submodules:

        decisionMaker: DecisionMaker {
            parameters:
                @display("p=378.07202,271.872;i=block/fork;is=s");
        }


        wlan[numRadios]: <wlanType> like IWirelessNic {
            @display("p=378.07202,454.536;is=s");
        }

        mobility: <mobilityType> like IMobility {
            @display("p=53.884,202.774");
        }

        simpleApp[numApps]: SimpleApp {
            @display("p=379.13998,61.059998;is=vs");
        }

    connections allowunconnected:

        // Simple App to decider 
        for i=0..sizeof(numApps)-1 {
            simpleApp[i].toDecisionMaker --> decisionMaker.fromApplication;
            decisionMaker.toApplication --> simpleApp[i].fromDecisionMaker;
        }


        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            
            decisionMaker.ifOutRadio[i] --> wlan[i].upperLayerIn;
            wlan[i].upperLayerOut --> decisionMaker.ifInRadio[i];
        }

}
