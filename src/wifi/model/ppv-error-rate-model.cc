#include <cmath>
#include "ns3/log.h"
#include "ppv-error-rate-model.h"
#include "wifi-utils.h"
#include "wifi-tx-vector.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PpvErrorRateModel");

NS_OBJECT_ENSURE_REGISTERED (PpvErrorRateModel);

TypeId
PpvErrorRateModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PpvErrorRateModel")
    .SetParent<ErrorRateModel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<PpvErrorRateModel> ()
  ;
  return tid;
}

PpvErrorRateModel::PpvErrorRateModel ()
{
}

double
PpvErrorRateModel::DoGetChunkSuccessRate (WifiMode mode, const WifiTxVector& txVector, double snr, uint64_t nbits, uint8_t numRxAntennas, WifiPpduField field, uint16_t staId) const
{
  NS_LOG_FUNCTION (this << mode << txVector << snr << nbits << +numRxAntennas << field << staId);
  uint64_t phyRate = mode.GetPhyRate (txVector, staId);
  double B = txVector.GetChannelWidth ()*1e6;
  double D = ((double) nbits)/((double) phyRate);
  if(B==0){
    B=1e6;
  }
  if(nbits == 0){
    nbits = 100*8;
  }
  double cd = 1.-1./((1+snr)*(1+snr));
  double nu = - (double)(nbits) * log(2) + D*B*log(1+snr);
  double de = sqrt(D*B*cd);
  double error = 1./2.*erfc(nu/de/sqrt(2));
//  NS_LOG_UNCOND(error <<",  " << Simulator::Now().GetMicroSeconds()<<", " << nbits);
//
//  NS_LOG_UNCOND(cd <<"," << nu <<"," <<de <<"," << error <<"," <<cd <<",");
//    NS_LOG_UNCOND(D <<"," << B<< ":"<< nu/de/sqrt(2));
//    NS_LOG_UNCOND(-(double)(nbits) <<"," << D*B*log(1+snr));
//    if(error>0.5)
//      NS_LOG_UNCOND(nu << "," << de << "," << error);
//  if (nu == 0. and de ==0.){
//        NS_LOG_UNCOND (this << mode << txVector << snr << nbits << +numRxAntennas << field << staId);
//        NS_LOG_UNCOND (snr <<":" << nbits <<":" << field <<":" << staId);
//        NS_LOG_UNCOND (B <<":" << D);
//        NS_LOG_UNCOND (nu <<"//" << de);
//        NS_LOG_UNCOND (field);
//  }


  return 1.-error;
}

} //namespace ns3
