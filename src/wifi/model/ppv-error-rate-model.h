#ifndef PPV_ERROR_RATE_MODEL_H
#define PPV_ERROR_RATE_MODEL_H

#include "error-rate-model.h"

namespace ns3 {

class PpvErrorRateModel : public ErrorRateModel
{
public:
  static TypeId GetTypeId (void);

    PpvErrorRateModel ();


private:
  double DoGetChunkSuccessRate (WifiMode mode, const WifiTxVector& txVector, double snr, uint64_t nbits,
                                uint8_t numRxAntennas, WifiPpduField field, uint16_t staId) const override;
};

} //namespace ns3

#endif /* PPV_ERROR_RATE_MODEL_H */
