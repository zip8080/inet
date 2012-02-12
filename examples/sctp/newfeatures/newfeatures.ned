// * --------------------------------------------------------------------------
// *
// *     //====//  //===== <===//===>  //====//
// *    //        //          //      //    //    SCTP Optimization Project
// *   //=====   //          //      //====//   ==============================
// *        //  //          //      //           University of Duisburg-Essen
// *  =====//  //=====     //      //
// *
// * --------------------------------------------------------------------------
// *
// *   Copyright (C) 2009-2012 by Thomas Dreibholz
// *
// *   This program is free software: you can redistribute it and/or modify
// *   it under the terms of the GNU General Public License as published by
// *   the Free Software Foundation, either version 3 of the License, or
// *   (at your option) any later version.
// *
// *   This program is distributed in the hope that it will be useful,
// *   but WITHOUT ANY WARRANTY; without even the implied warranty of
// *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *   GNU General Public License for more details.
// *
// *   You should have received a copy of the GNU General Public License
// *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
// *
// *   Contact: dreibh@iem.uni-due.de

package inet.examples.sctp.newfeatures;

import inet.nodes.inet.StandardHost;
import inet.nodes.inet.Router;
import ned.DatarateChannel;
import ned.DelayChannel;
import ned.IdealChannel;
import inet.networklayer.autorouting.MultihomedFlatNetworkConfigurator;


channel coreChannel extends DatarateChannel
{
   int netID = default(0);        // NOTE: Shared by all networks!
   datarate  = default(1 Mbps);
   delay     = default(0ms);
   ber       = default(0);
   per       = default(0);
   @display("ls=black,3");
}

channel accessChannel extends DatarateChannel
{
   int netID = default(1);
   datarate  = default(1 Gbps);
   delay     = default(0.1ms);
   ber       = default(0);
   per       = default(0);
   @display("ls=blue,5");
}


network newFeaturesTest
{
    // ====== System Parameters =============================================
    parameters:
        int networks = default(1);
        int systems  = default(3);
    @display("bgi=maps/world,s");

    // ====== Network Components ============================================
   submodules:
      // ------ Routing Auto-Configuration --------------------------------
      configurator: MultihomedFlatNetworkConfigurator {
         parameters:
            @display("p=850,75;i=abstract/penguin_l,gold");
      }

      // ------ Routers -----------------------------------------------------
      westernBorderRouter[networks] : StandardHost {
         parameters:
            IPForward = true;
            @display("p=500,150,c,150;i=abstract/router_l,gold");
         gates:
            pppg[1 + systems];
      }
      westernCoreRouter[networks] : StandardHost {
         parameters:
            IPForward = true;
            @display("p=700,150,c,150;i=abstract/router_l,gold");
         gates:
            pppg[2];
      }
      easternCoreRouter[networks] : StandardHost {
         parameters:
            IPForward = true;
            @display("p=1000,150,c,150;i=abstract/router_l,gold");
         gates:
            pppg[2];
      }
      easternBorderRouter[networks] : StandardHost {
         parameters:
            IPForward = true;
            @display("p=1200,150,c,150;i=abstract/router_l,gold");
         gates:
            pppg[1 + systems];
      }

      // ------ Systems -----------------------------------------------------
      clientMain[systems] : StandardHost {
         parameters:
            IPForward = false;
            @display("p=50,50,c,200;i=device/laptop_l,green");
         gates:
            pppg[networks];
      }
      serverMain[systems] : StandardHost {
         parameters:
            IPForward = false;
            @display("p=1650,50,c,200;i=device/server_l,green");
         gates:
            pppg[networks];
      }

   // ====== Connections ====================================================
   connections:
      for n=0..networks-1 {
         westernCoreRouter[n].pppg[0]
            <--> coreChannel { netID=1000+n; @display("ls=blue,3"); }
            <--> easternCoreRouter[n].pppg[0];

         westernBorderRouter[n].pppg[0]
            <--> accessChannel { netID=1000+n; @display("ls=blue,5"); }
            <--> westernCoreRouter[n].pppg[1];
         easternBorderRouter[n].pppg[0]
            <--> accessChannel { netID=1000+n; @display("ls=blue,5"); }
            <--> easternCoreRouter[n].pppg[1];
      }

      for s=0..systems-1, for n=0..networks-1 {
            clientMain[s].pppg[n]
               <--> accessChannel { netID=1000+n; @display("ls=green,5"); }
               <--> westernBorderRouter[n].pppg[1 + s];
            serverMain[s].pppg[n]
               <--> accessChannel { netID=1000+n; @display("ls=green,5"); }
               <--> easternBorderRouter[n].pppg[1 + s];
      }
}
