/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// cross_layer_sim.cc

#include "ns3/core-module.h"
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <iomanip>

using namespace ns3;

//––– Global ZMQ sockets so PollZmq can see them –––
static zmq::socket_t* g_hapticSub = nullptr;
static zmq::socket_t* g_videoSub  = nullptr;

// Statistics
static int    g_hapticCount       = 0;
static int    g_videoCount        = 0;
static double g_totalHapticLat    = 0.0;
static double g_totalVideoLat     = 0.0;
static bool   g_seenFirstTs       = false;
static double g_baseTs            = 0.0;

// This is called once per millisecond of *real* wall-clock
// (because we use the realtime scheduler).  It polls ZMQ,
// prints any incoming packets, and re-schedules itself 1 ms later.
void
PollZmq ()
{
  zmq::pollitem_t items[2] = {
    { static_cast<void*>(*g_hapticSub), 0, ZMQ_POLLIN, 0 },
    { static_cast<void*>(*g_videoSub),  0, ZMQ_POLLIN,  0 }
  };
  zmq::poll (items, 2, std::chrono::milliseconds (1));

  double simNow = Simulator::Now ().GetSeconds ();

  //––– Haptic? –––
  if (items[0].revents & ZMQ_POLLIN)
    {
      zmq::message_t m;
      (void) g_hapticSub->recv (m, zmq::recv_flags::none);
      double ts = std::stod (m.to_string ());
      if (!g_seenFirstTs) { g_baseTs = ts; g_seenFirstTs = true; }
      ts -= g_baseTs;
      double lat = (simNow - ts) * 1000.0;
      g_hapticCount++;  g_totalHapticLat += lat;
      std::cout << std::fixed<<std::setprecision(3)
                << std::setw(8)<< simNow
                << std::setw(10)<<"Haptic"
                << std::setw(12)<< ts
                << std::setw(12)<< lat
                << "\n";
    }

  //––– Video? –––
  if (items[1].revents & ZMQ_POLLIN)
    {
      zmq::message_t m;
      (void) g_videoSub->recv (m, zmq::recv_flags::none);
      std::string s = m.to_string ();
      double ts = std::stod (s.substr (0, s.find (',')));
      if (!g_seenFirstTs) { g_baseTs = ts; g_seenFirstTs = true; }
      ts -= g_baseTs;
      double lat = (simNow - ts) * 1000.0;
      g_videoCount++;  g_totalVideoLat += lat;
      std::cout << std::fixed<<std::setprecision(3)
                << std::setw(8)<< simNow
                << std::setw(10)<<"Video"
                << std::setw(12)<< ts
                << std::setw(12)<< lat
                << "\n";
    }

  // Schedule yourself again in 1 ms sim-time (which maps to ~1 ms wall-clock)
  Simulator::Schedule (MilliSeconds (1), &PollZmq);
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // 1) real-time scheduler
  Time::SetResolution (Time::NS);
  ObjectFactory f;
  f.SetTypeId ("ns3::RealtimeSimulatorImpl");
  Simulator::SetScheduler (f);

  // 2) bind our ZMQ SUB sockets once
  zmq::context_t ctx (1);
  static zmq::socket_t hSub (ctx, zmq::socket_type::sub);
  static zmq::socket_t vSub (ctx, zmq::socket_type::sub);
  g_hapticSub = &hSub;
  g_videoSub  = &vSub;

  hSub.connect ("tcp://127.0.0.1:5556");
  hSub.set (zmq::sockopt::subscribe, "");
  std::cout << "[ZMQ] Connected to haptic → tcp://127.0.0.1:5556\n";

  vSub.connect ("tcp://127.0.0.1:5566");
  vSub.set (zmq::sockopt::subscribe, "");
  std::cout << "[ZMQ] Connected to video  → tcp://127.0.0.1:5566\n";

  // 3) start polling at t=0
  Simulator::Schedule (MilliSeconds (0), &PollZmq);

  // 4) stop after 30 s of sim-time
  Simulator::Stop (Seconds (30.0));

  // 5) run & clean up
  Simulator::Run ();
  Simulator::Destroy ();

  // 6) final summary
  std::cout << "\n=== Final Summary ===\n"
            << "Haptic: " << g_hapticCount
            << ", avg = " << (g_hapticCount ? g_totalHapticLat/g_hapticCount : 0.0)
            << " ms\n"
            << "Video : " << g_videoCount
            << ", avg = " << (g_videoCount ? g_totalVideoLat/g_videoCount : 0.0)
            << " ms\n";
  return 0;
}