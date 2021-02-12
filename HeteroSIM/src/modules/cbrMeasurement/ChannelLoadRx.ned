package heterosim.src.modules.cbrMeasurement;

import inet.linklayer.ieee80211.mac.Rx;

simple ChannelLoadRx extends Rx
{
    parameters:
        @class(ChannelLoadRx);
        @signal[ChannelLoad](type=double);
        @statistic[ChannelLoad](record=timeavg,vector?);
        
        
        bool asyncChannelReport = default(false);
        double asyncChannelReportInterval @unit(s) = default(100ms);
        bool cbrWithTx = default(false);
}
