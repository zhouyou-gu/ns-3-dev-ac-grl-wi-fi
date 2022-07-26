//
// Created by Zhouyou Gu on 6/07/22.
//

#ifndef MY_GYM_ENTITY_H
#define MY_GYM_ENTITY_H

#include <ns3/node-container.h>
#include <ns3/application-container.h>
#include <ns3/propagation-loss-model.h>
#include "ns3/opengym-module.h"
#include "ns3/nstime.h"

namespace ns3 {

class MyGymEnv : public OpenGymEnv
{
public:
  MyGymEnv ();
  MyGymEnv (int n_ap, int n_sta, Ptr<PropagationLossModel> lm);
  virtual ~MyGymEnv ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  Ptr<OpenGymSpace> GetActionSpace();
  Ptr<OpenGymSpace> GetObservationSpace();
  bool GetGameOver();
  Ptr<OpenGymDataContainer> GetObservation();
  float GetReward();
  std::string GetExtraInfo();
  bool ExecuteActions(Ptr<OpenGymDataContainer> action);

  NodeContainer m_apNodes;
  NodeContainer m_staNodes;
  ApplicationContainer m_serverApps;

private:
  int m_n_ap = 0;
  int m_n_sta = 0;
  Ptr<PropagationLossModel> m_loss_model;
};

}


#endif // MY_GYM_ENTITY_H
