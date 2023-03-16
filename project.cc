#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include <stdlib.h>
#include <cmath>
#include <chrono>
#include <thread>
using namespace ns3;
using namespace std;

class member
{
  string nethost = "192.168.1.0";
  string netmask = "255.255.255.0";

public:
  NodeContainer node;
  NetDeviceContainer devices;
  InternetStackHelper stack;
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interface;
  ApplicationContainer app;
  CsmaHelper csma;
  MobilityHelper mobility;
  AnimationInterface *anim = 0;
  int random_inf;

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  int maskdefrate[3] = {560, 670, 830};
  struct memberAttribute
  {
	  int infected_rate;
	  int get_covid_rate;
  } memberData[50];

  member (int amount, int datarate, int delay)
  {
    srand(time(NULL));
    random_inf = rand() % 50;
    node.Create (amount);
    stack.Install (node);
    devices = csma.Install (node);
    setIP (nethost, netmask);
    setCSMA (datarate, delay);
    setPosition (node);
    setSentPacket (random_inf);
    setRecvPacket (amount);
    SetNodeData(amount);
    Simulator::Stop (Seconds (60.0));
    anim = new AnimationInterface("project_6.xml");
    anim->SetMaxPktsPerTraceFile (1000000);
    for (uint32_t i = 0; i < 50; i++)
      {
        anim->UpdateNodeColor(node.Get(i), 0,0,255);
        anim->UpdateNodeSize (i, 2, 2);
      }
    anim->UpdateNodeColor(node.Get(random_inf), 255,0,0);

    Simulator::Run ();
    Simulator::Destroy ();
  };
  void
  RecvPacket (Ptr<Socket> socket)
  {
    if (socket->Recv ())
      {srand(time(NULL));
        int random = rand() % 1000 + 1;
        if(random > memberData[socket->GetNode()->GetId()].get_covid_rate)
        {
          int random2 = rand() % 1000 +1;
          if(random2 > memberData[socket->GetNode()->GetId()].infected_rate)
          {
            Ptr<MobilityModel> position = socket->GetNode()->GetObject<MobilityModel>();
            Ptr<MobilityModel> position_infected = node.Get(random_inf)->GetObject<MobilityModel>();
            double position_inf_x = position_infected->GetPosition().x;
            double position_inf_y = position_infected->GetPosition().y;
            double position_x = position->GetPosition().x;
            double position_y = position->GetPosition().y;
            double distance = sqrt(abs((pow((position_inf_x - position_x),2.0)+pow((position_inf_y - position_y),2.0))));
            if (distance < 20.0)
            {
              anim->UpdateNodeColor(socket->GetNode(), 255,255,0);
            }
          }
        }
      }

  }
  void
  setCSMA (int datarate, int delay)
  {
    // set csma
    csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (datarate)));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));
  }

  void
  setIP (string nethost, string netmask)
  {
    // set IPV4
    address.SetBase (Ipv4Address (nethost.c_str ()), Ipv4Mask (netmask.c_str ()));
    interface = address.Assign (devices);
  }

  void
  setSentPacket (int node_id)
  {
    Ptr<Socket> source = Socket::CreateSocket (node.Get (node_id), tid);
    InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
    source->SetAllowBroadcast (true);
    source->Connect (remote);
    OnOffHelper onoff ("ns3::UdpSocketFactory",
                       Address (InetSocketAddress (Ipv4Address ("255.255.255.255"), 80)));
    onoff.SetConstantRate (DataRate ("5kb/s"));
    app.Add (onoff.Install (node.Get (node_id)));
    Time (1, 20);
  }

  void
  Time (float start, float stop)
  {
    app.Start (Seconds (start));
    app.Stop (Seconds (stop));
  }

  void
  setRecvPacket (int amount)
  {
    for (int n = 0; n < amount; n++)
      {
        Ptr<Socket> recvSink = Socket::CreateSocket (node.Get (n), tid);
        InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
        recvSink->Bind (local);
        recvSink->SetRecvCallback (MakeCallback (&member::RecvPacket, this));

      }
  }
  static void
  CourseChange (std::string context, Ptr<const MobilityModel> mobility)
  {
    Vector pos = mobility->GetPosition ();
    Vector vel = mobility->GetVelocity ();
    std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x
              << ", y=" << pos.y << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
              << ", z=" << vel.z << std::endl;
  }
  void
  setPosition (NodeContainer c)
  {
    float x = 40;
    float y = 13.5;
    for (int col = 0; col < 6; col++)
      {
        for (int row = 0; row < 9; row++)
          {
            uint32_t node_id = (col * 9) + row;

            if (49 <= ((col * 9) + row))
              {
                AddNode (c, 7, 50, node_id);
                break;
              }
            else
              {
                AddNode (c, x, y, node_id);
                y+=11.5; // set space
//                if ((row + 1) % 3 == 0)
//                  {
//                    y += 11.5;
//                  }
//                else
//                  {
//                    y += 8;
//                  }
              }
          }
        x += 15;
        y = 13.5;
      }
  }
  void
  AddNode (NodeContainer c, float x, float y, uint32_t node_id)
  {
    mobility.SetMobilityModel ("ns3::WaypointMobilityModel");
    mobility.Install (c.Get (node_id));
    MovetoRack (c, node_id, x, y);
    MoveBack (c, node_id, x, y);
  }
  float
  halfcircle (float num, float y)
  {
    return (137 - (sqrt (pow (5.0, 2.0) - pow (num - y, 2.0))));
  }
  void
  MovetoRack (NodeContainer c, uint32_t node_id, float x, float y)
  {
    Ptr<WaypointMobilityModel> wayMobility;
    wayMobility = c.Get (node_id)->GetObject<WaypointMobilityModel> ();
    //float before = x - 4;
    //float after = x + 4;
    //float num = rand () % 10;

    //Instructor's Move
    if (node_id == 49)
      {
        Move (wayMobility, 0.0, 5.9, x, y, x + 20, y + 18);
        Move (wayMobility, 6, 9.9, x + 20, y + 18, 129, y + 18);
        Move (wayMobility, 10, 12.5, 129, y + 18, 129, y - 30);
        Move (wayMobility, 12.6, 15, 129, y - 30, 129, y + 30);
      }
    //row1
    else
      {
        Move (wayMobility, 0.0, 1.9, x, y, x+1, y+1);
      }

  }
  void
  MoveBack (NodeContainer c, uint32_t node_id, float x, float y)
  {
    Ptr<WaypointMobilityModel> wayMobility;
    wayMobility = c.Get (node_id)->GetObject<WaypointMobilityModel> ();


    //Instructor's Move
    if (node_id == 49)
      {
        Move (wayMobility, 20, 22.5, 129, y - 30, 129, y + 18);
        Move (wayMobility, 22.6, 26.9, 129, y + 18, x + 20, y + 18);
        Move (wayMobility, 27, 33, x + 20, y + 18, x, y);
      }
      else if (node_id >= 45)
      {
        Move (wayMobility, 30, 40, x, y, rand() % (91-36) + 36, rand() % (98-12) + 12);
        Move (wayMobility, 40.1, 50, rand() % (91-36) + 36, rand() % (98-12) + 12, rand() % (91-36) + 36, rand() % (98-12) + 12);
        Move (wayMobility, 50.1, 60, rand() % (91-36) + 36, rand() % (98-12) + 12, rand() % (91-36) + 36, rand() % (98-12) + 12);
      }

  }
  void
  Move (Ptr<WaypointMobilityModel> wayMobility, double time_str, double time_end, float x_str,
        float y_str, float x_end, float y_end)
  {
    Waypoint waypointStart (Seconds (time_str), Vector3D (x_str, y_str, 0));
    Waypoint waypointEnd (Seconds (time_end), Vector3D (x_end, y_end, 0));

    wayMobility->AddWaypoint (waypointStart);
    wayMobility->AddWaypoint (waypointEnd);
  }

  void
  SetNodeData (int amount)
  {
    srand(time(NULL));
    for (int node_id = 0; node_id < amount; node_id++)
      {
        int dose = rand () % 4;
        memberData[node_id].get_covid_rate = maskdefrate[rand()%3];
        int vaccine_type = rand () % 4;
		if(dose == 1){
			if (vaccine_type == 2 || vaccine_type == 3){
				memberData[node_id].infected_rate = 439;
			}else if(vaccine_type == 0){
				memberData[node_id].infected_rate = 567;
			}else if(vaccine_type == 1){
				memberData[node_id].infected_rate = 720;
			}
		}else if(dose == 2){
			if(vaccine_type == 0){
				memberData[node_id].infected_rate = 820;
			}else if(vaccine_type == 1){
				memberData[node_id].infected_rate = 775;
			}else if (vaccine_type == 2){
				memberData[node_id].infected_rate = 590;
			}else if(vaccine_type == 3){
				memberData[node_id].infected_rate = 801;
			}
		}else if(dose == 3){
			if(vaccine_type == 3){
				vaccine_type = rand()%3;
			}
			if(vaccine_type == 0){
				memberData[node_id].infected_rate = 972;
			}else if(vaccine_type == 1){
				memberData[node_id].infected_rate = 970;
			}else if (vaccine_type == 2){
				memberData[node_id].infected_rate = 638;
			}
		}


      }
  }

  uint16_t port = 9;
};

int
main (int argc, char *argv[])
{
  member (50, 5000000, 2);
}
