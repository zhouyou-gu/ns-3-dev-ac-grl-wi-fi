//
// Created by Zhouyou Gu on 6/07/22.
//uthor: Piotr Gawlowicz <gawlowicz@tkn.tu-berlin.de>

#include "mygym.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/node-list.h"
#include "ns3/log.h"
#include <sstream>
#include <iostream>
#include <ns3/udp-server.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyGymEnv");

NS_OBJECT_ENSURE_REGISTERED (MyGymEnv);

MyGymEnv::MyGymEnv ()
{
  NS_LOG_FUNCTION (this);
}

MyGymEnv::MyGymEnv (int n_ap, int n_sta, Ptr<PropagationLossModel> lm)
{
  NS_LOG_FUNCTION (this);
  m_n_ap = n_ap;
  m_n_sta = n_sta;
  m_loss_model = lm;
  is_simulation_end = false;
}

MyGymEnv::~MyGymEnv ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MyGymEnv::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyGymEnv")
    .SetParent<OpenGymEnv> ()
    .SetGroupName ("OpenGym")
    .AddConstructor<MyGymEnv> ()
  ;
  return tid;
}

void
MyGymEnv::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

/*
Define observation space
*/
Ptr<OpenGymSpace>
MyGymEnv::GetObservationSpace()
{

  std::vector<uint32_t> shape_aoi = {(uint32_t)m_n_sta,};
  std::string dtype = TypeNameGet<float> ();
  Ptr<OpenGymBoxSpace> aoi = CreateObject<OpenGymBoxSpace> (0., 1000000000., shape_aoi, dtype);
  Ptr<OpenGymDictSpace> space = CreateObject<OpenGymDictSpace> ();
  space->Add("aoi", aoi);

//  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

/*
Define action space
*/
Ptr<OpenGymSpace>
MyGymEnv::GetActionSpace()
{
  float low = -1000.0;
  float high = 0.0;
  std::vector<uint32_t> shape_loss_ap_ap= {(uint32_t)m_n_ap * (uint32_t)m_n_ap,};
  std::vector<uint32_t> shape_loss_sta_ap= {(uint32_t)m_n_ap * (uint32_t)m_n_sta,};
  std::vector<uint32_t> shape_loss_sta_sta= {(uint32_t)m_n_sta * (uint32_t)m_n_sta,};
  std::string dtype = TypeNameGet<float> ();
  Ptr<OpenGymBoxSpace> loss_ap_ap = CreateObject<OpenGymBoxSpace> (low, high, shape_loss_ap_ap, dtype);
  Ptr<OpenGymBoxSpace> loss_sta_ap = CreateObject<OpenGymBoxSpace> (low, high, shape_loss_sta_ap, dtype);
  Ptr<OpenGymBoxSpace> loss_sta_sta = CreateObject<OpenGymBoxSpace> (low, high, shape_loss_sta_sta, dtype);
  Ptr<OpenGymDictSpace> space = CreateObject<OpenGymDictSpace> ();

  space->Add("loss_ap_ap", loss_ap_ap);
  space->Add("loss_sta_ap", loss_sta_ap);
  space->Add("loss_sta_sta", loss_sta_sta);

//  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool
MyGymEnv::GetGameOver()
{
//  NS_LOG_UNCOND ("MyGetGameOver: " << Simulator::IsFinished());
  return Simulator::IsFinished();
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer>
MyGymEnv::GetObservation()
{

  std::vector<uint32_t> shape_pm = {(uint32_t)m_n_sta,};
  Ptr<OpenGymBoxContainer<float> > pm = CreateObject<OpenGymBoxContainer<float> >(shape_pm);
  if (is_simulation_end){
    for (uint32_t j = 0; j < m_n_sta; j++){
      float aoi = DynamicCast<UdpServer>(m_serverApps.Get(j))->GetLastAoI_us();
      pm->AddValue(aoi);
    }
  } else {
    for (uint32_t j = 0; j < m_n_sta; j++){
      pm->AddValue(0.);
    }
  }

  Ptr<OpenGymDictContainer> data = CreateObject<OpenGymDictContainer> ();
  data->Add("aoi",pm);

  // Print data from tuple
//  Ptr<OpenGymBoxContainer<float> > aoi = DynamicCast<OpenGymBoxContainer<float> >(data->Get("aoi"));
//  NS_LOG_UNCOND ("MyGetObservation: " << data);
//  NS_LOG_UNCOND ("---" << aoi);

  return data;
}

/*
Define reward function
*/
float
MyGymEnv::GetReward()
{
  return 0;
}

/*
Define extra info. Optional
*/
std::string
MyGymEnv::GetExtraInfo()
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
//  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}

/*
Execute received actions
*/
bool
MyGymEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<MatrixPropagationLossModel> lm = DynamicCast<MatrixPropagationLossModel>(m_loss_model);
  Ptr<OpenGymDictContainer> dict = DynamicCast<OpenGymDictContainer>(action);
  Ptr<OpenGymBoxContainer<float> > loss_ap_ap = DynamicCast<OpenGymBoxContainer<float> >(dict->Get("loss_ap_ap"));
  Ptr<OpenGymBoxContainer<float> > loss_sta_ap = DynamicCast<OpenGymBoxContainer<float> >(dict->Get("loss_sta_ap"));
  Ptr<OpenGymBoxContainer<float> > loss_sta_sta = DynamicCast<OpenGymBoxContainer<float> >(dict->Get("loss_sta_sta"));

  int counter = 0;


  for (uint32_t i = 0; i < m_n_ap; i++){
    for (uint32_t j = 0; j < m_n_ap; j++){
      lm->SetLoss (m_apNodes.Get (i)->GetObject<MobilityModel> (), m_apNodes.Get (j)->GetObject<MobilityModel> (), -loss_ap_ap->GetValue(counter),
                   false);
      counter++;
    }
  }
  counter = 0;

  for (uint32_t i = 0; i < m_n_ap; i++){
    for (uint32_t j = 0; j < m_n_sta; j++){
      lm->SetLoss (m_apNodes.Get (i)->GetObject<MobilityModel> (), m_staNodes.Get (j)->GetObject<MobilityModel> (), -loss_sta_ap->GetValue(counter),
                     true);
      counter++;
    }
  }
  counter = 0;

  for (uint32_t i = 0; i < m_n_sta; i++){
    for (uint32_t j = 0; j < m_n_sta; j++){
      lm->SetLoss (m_staNodes.Get (i)->GetObject<MobilityModel> (), m_staNodes.Get (j)->GetObject<MobilityModel> (), -loss_sta_sta->GetValue(counter),
                   false);
      counter++;
    }
  }
  counter = 0;
  return true;
}

} // ns3 namespace