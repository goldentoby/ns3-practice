 /*
  * This is a basic example with symmetric flow topology:
  *
  * src -------------------------- router ------------
  *         100 Mb/s, 0.1 ms (CSMA)                      |
  *                                                      | 10 Mb/s, 1 ms
  *                                                      | bottleneck (point2point)
  *                                                      |
  * dst -------------------------- router ------------
  *         100 Mb/s, 0.1 ms (CSMA)
  * 
  * 
  * The source generates traffic across the network using BulkSendApplication.
  * Packets transmitted during a simulation run are captured into a .pcap file, and
  * congestion window values are also traced.
  */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("HomeWork1.1");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  Time::SetResolution (Time::NS);
  bool verbose = true;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  if (verbose){
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }

  std::string bottleneckBandwidth = "10Mbps";
  std::string bottleneckDelay = "1ms";
  std::string accessBandwidth = "100Mbps";
  std::string accessDelay = "0.1ms";


  NS_LOG_INFO ("Create nodes.");
  NodeContainer p2pNodes;
  p2pNodes.Create (2);
  // create on two side
  NodeContainer csmaNode_src, csmaNode_dst;
  csmaNode_src.Add (p2pNodes.Get (0));
  csmaNode_src.Create (1);
  csmaNode_dst.Add (p2pNodes.Get (1));
  csmaNode_dst.Create (1);


  NS_LOG_INFO ("Create channels and devices.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bottleneckBandwidth));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);
  
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue(accessBandwidth));
  csma.SetChannelAttribute ("Delay", StringValue(accessDelay));
  NetDeviceContainer csmaDevice_src, csmaDevice_dst;
  csmaDevice_src = csma.Install (csmaNode_src);
  csmaDevice_dst = csma.Install (csmaNode_dst);


  // We now have our nodes, devices and channels created, but we have no protocol stacks present.
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (p2pNodes.Get (1));
  stack.Install (csmaNode_src.Get (1));
  stack.Install (csmaNode_dst.Get (1));


  NS_LOG_INFO ("Assign IP.");
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterface_src;
  csmaInterface_src = address.Assign (csmaDevice_src);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterface_dst;
  csmaInterface_dst = address.Assign (csmaDevice_dst);
  
  
  // // Access link traffic control configuration
  // NS_LOG_INFO ("AMQ Setting.");
  // TrafficControlHelper tchPfifoFastAccess;
  // tchPfifoFastAccess.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("1000p"));
  // // Bottleneck link traffic control configuration
  // TrafficControlHelper tchPfifo;
  // tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));
  // TrafficControlHelper tchCoDel;
  // tchCoDel.SetRootQueueDisc ("ns3::CoDelQueueDisc");
  // Config::SetDefault ("ns3::CoDelQueueDisc::MaxSize", StringValue (std::to_string(queueDiscSize) + "p"));


  NS_LOG_INFO ("Application Test.");
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNode_src.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterface_src.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (csmaNode_dst.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  pointToPoint.EnablePcapAll ("point2point");
  csma.EnablePcap ("LAN_SRC", csmaDevice_src.Get (0), true);
  csma.EnablePcap ("LAN_DST", csmaDevice_dst.Get (0), true);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
