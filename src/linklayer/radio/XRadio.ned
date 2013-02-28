// * **************************************************************************
// * file:        XRadio.ned
// ***************************************************************************
// ***************************************************************************

package inet.linklayer.radio;

//
// This is an abstract module that defines all parameters for radio modules
//
simple XRadio like IRadio
{
    parameters:
        int channelNumber = default(0); // channel identifier this radio listens. Works only with simlified management module. Otherwise it scans all the channels as specified in ieee 80211
        double carrierFrequency @unit("Hz") = default(2.4GHz);
        double bitrate @unit("bps");
        double transmitRange @unit("m") = default(20m);   // amilyen tavolsagban meeg lehet venni a csomagot
        double transmitAsNoiseRange @unit("m") = default(5*transmitRange);   // amilyen tavolsagban meeg zajkent megerkezik a csomag

        // battery module parameters (if any of them is negative, the battery module is disabled)
        double usage_radio_idle @unit(mA) = default(-1mA); // disable battery registration
        double usage_radio_recv @unit(mA) = default(-1mA); // disable battery registration
        double usage_radio_sleep @unit(mA) = default(-1mA); // disable battery registration
        double usage_radio_send @unit(mA) = default(-1mA); // disable battery registration

        // misc
        bool drawCoverage = default(true);  // draw the coverage area in Tkenv
        int refreshCoverageInterval @unit("s") = default(0s);  // update period in simulation time for coverage redraw

        @signal[bitrate](type="double";unit=bps); // current transmit bitrate
        @signal[channelNo](type=long);
        @signal[radioState](type=long);  // enum
        @signal[lossRate](type="double"); // 0..1; percentage of frames received incorrectly in the last period
        @statistic[bitrate](title="tx bitrate"; record=min,max,timeavg,vector; interpolationmode=sample-hold;unit=bps);
        @statistic[channelNo](title="channel no"; record=histogram,vector; interpolationmode=sample-hold);
        @statistic[radioState](title="radio state (enum)"; record=count,vector; interpolationmode=sample-hold);
        @statistic[lossRate](title="packet loss rate"; record=min,max,timeavg,vector; interpolationmode=backward-sample-hold);
    gates:
        input upperLayerIn @labels(PhyControlInfo/down); // from higher layer protocol (MAC)
        output upperLayerOut @labels(PhyControlInfo/up); // to higher layer protocol (MAC)
        input radioIn @labels(AirFrame); // to receive frames (AirFrame) on the radio channel
}

