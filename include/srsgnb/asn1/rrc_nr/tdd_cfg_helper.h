
#ifndef SRSGNB_TDD_CFG_HELPER_H
#define SRSGNB_TDD_CFG_HELPER_H

#include "rrc_nr.h"
#include "srsgnb/adt/static_vector.h"
#include "srsgnb/ran/slot_point.h"

namespace srsgnb {

/// Get number of slots per frame given a SCS
inline unsigned get_nof_slots_per_subframe(asn1::rrc_nr::subcarrier_spacing_e scs)
{
  return 1U << scs.value;
}

namespace tdd_cfg_helper {

/// Get TDD period in msec from a TDD configuration
/// \remark See 38.321, TDD-UL-DL-Pattern.
float get_period_ms(asn1::rrc_nr::tdd_ul_dl_pattern_s::dl_ul_tx_periodicity_e_ cfg);

/// Get TDD period in slots from a TDD configuration
/// \remark See 38.321, TDD-UL-DL-Pattern.
unsigned nof_slots_per_period(const asn1::rrc_nr::tdd_ul_dl_cfg_common_s& cfg);

/// Check if current slot has active DL given a TDD configuration
/// \return both fully DL and partially DL slots (i.e. only some symbols are DL) return true.
bool slot_is_dl(const asn1::rrc_nr::tdd_ul_dl_cfg_common_s& cfg, slot_point sl);

/// Check if current slot has active UL given a TDD configuration
bool slot_is_ul(const asn1::rrc_nr::tdd_ul_dl_cfg_common_s& cfg, slot_point sl);

} // namespace tdd_cfg_helper

} // namespace srsgnb

#endif // SRSGNB_TDD_CFG_HELPER_H
