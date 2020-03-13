 /*
  * This is a basic example with symmetric flow topology:
  *
  * mobile ||||||||||||||||||||||||||||   
  *        10 Mb/s, 1 ms (csma)       |
  *                                   Access Point
  *        100 Mb/s, 0.1 ms (p2p)     |
  * STA    ----------------------------
  * 
  * 
  * 
  * The source generates traffic across the network using BulkSendApplication.
  * Packets transmitted during a simulation run are captured into a .pcap file, and
  * congestion window values are also traced.
  */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("HomeWork1.2");
Ptr<PacketSink> sink;           /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;       /* The value of the last total received bytes */


void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                             /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;      /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}


int
main (int argc, char *argv[])
{

  Time::SetResolution (Time::NS);
  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "10Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 10;                        /* Simulation time in seconds. */
  bool pcapTracing = true;                          /* PCAP Tracing is enabled or not. */
  bool verbose = true;
  std::string bottleneckBandwidth = "10Mbps";
  std::string bottleneckDelay = "1ms";

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ", tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  if (tcpVariant.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));
    }


  
  if (verbose){
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }

  NS_LOG_INFO ("Configure TCP Options.");
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);


  NS_LOG_INFO ("Set up Legacy Channels.");
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));


  NS_LOG_INFO ("Set up Physical Channels.");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ( "ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (phyRate),
                                      "ControlMode", StringValue ("HtMcs0"));


  NS_LOG_INFO ("Create nodes.");
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> apWifiNode = wifiNodes.Get (0);
  Ptr<Node> staWifiNode = wifiNodes.Get (1);

  NodeContainer csmaNode_src;
  csmaNode_src.Add (apWifiNode);
  csmaNode_src.Create (1);
  

  NS_LOG_INFO ("Configure AP and STA.");
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ( "ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice;
  apDevice = wifiHelper.Install (wifiPhy, wifiMac, apWifiNode);

  wifiMac.SetType ( "ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, wifiMac, staWifiNode);



  NS_LOG_INFO ("Configure Mobility Model.");
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 1.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apWifiNode);
  mobility.Install (staWifiNode);


  NS_LOG_INFO ("Create channels and devices.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue(bottleneckBandwidth));
  csma.SetChannelAttribute ("Delay", StringValue(bottleneckDelay));
  NetDeviceContainer csmaDevice_src;
  csmaDevice_src = csma.Install (csmaNode_src);


  // We now have our nodes, devices and channels created, but we have no protocol stacks present.
  InternetStackHelper stack;
  stack.Install (wifiNodes);
  stack.Install (csmaNode_src.Get (1));


  NS_LOG_INFO ("Assign IP.");
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterface_src;
  csmaInterface_src = address.Assign (csmaDevice_src);


  NS_LOG_INFO ("Populate Routing Protocols.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
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


  NS_LOG_INFO ("Install TCP Receiver on the sta point.");
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (staWifiNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));


  NS_LOG_INFO ("Install TCP/UDP Transmitter on the node.");
  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterface.GetAddress (0), 9)));
  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp = server.Install (csmaNode_src);


  NS_LOG_INFO ("Application Test.");
  sinkApp.Start (Seconds (0.0));
  serverApp.Start (Seconds (1.0));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);


  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("AccessPoint", apDevice);
      wifiPhy.EnablePcap ("Station", staDevices);
      csma.EnablePcap ("LAN_SRC", csmaDevice_src.Get (0), true);
    }
  
  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));
  Simulator::Destroy ();

  if (averageThroughput < 10){
      NS_LOG_ERROR ("Obtained throughput is not in the expected boundaries!");
      exit (1);}
  std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;

  return 0;
}
