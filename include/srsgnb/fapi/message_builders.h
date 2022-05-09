#ifndef SRSGNB_FAPI_MESSAGE_BUILDERS_H
#define SRSGNB_FAPI_MESSAGE_BUILDERS_H

#include "srsgnb/adt/optional.h"
#include "srsgnb/adt/span.h"
#include "srsgnb/fapi/messages.h"
#include "srsgnb/support/math_utils.h"
#include <algorithm>

namespace srsgnb {
namespace fapi {

/// Helper class to fill in the DL SSB PDU parameters specified in SCF-222 v4.0 section 3.4.2.4.
class dl_ssb_pdu_builder
{
public:
  explicit dl_ssb_pdu_builder(dl_ssb_pdu& pdu) : pdu(pdu), v3(pdu.ssb_maintenance_v3)
  {
    v3.ss_pbch_block_power_scaling = std::numeric_limits<int16_t>::min();
    v3.beta_pss_profile_sss        = std::numeric_limits<int16_t>::min();
  }

  /// Sets the basic parameters for the fields of the SSB/PBCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table SSB/PBCH PDU.
  dl_ssb_pdu_builder& set_basic_parameters(pci_t                 phys_cell_id,
                                           beta_pss_profile_type beta_pss_profile_nr,
                                           uint8_t               ssb_block_index,
                                           uint8_t               ssb_subcarrier_offset,
                                           uint16_t              ssb_offset_pointA)
  {
    pdu.phys_cell_id          = phys_cell_id;
    pdu.beta_pss_profile_nr   = beta_pss_profile_nr;
    pdu.ssb_block_index       = ssb_block_index;
    pdu.ssb_subcarrier_offset = ssb_subcarrier_offset;
    pdu.ssb_offset_pointA     = ssb_offset_pointA;

    return *this;
  }

  /// Sets the BCH payload configured by the MAC and returns a reference to the builder.
  /// \note Use this function when the MAC generates the full PBCH payload.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table MAC generated MIB PDU.
  /// \note This function assumes that given bch_payload value is codified as: a0,a1,a2,...,a29,a30,a31, with the most
  /// significant bit being the leftmost (in this case a0 in position 31 of the uint32_t).
  dl_ssb_pdu_builder& set_bch_payload_mac_full(uint32_t bch_payload)
  {
    // Configure the BCH payload as fully generated by the MAC.
    pdu.bch_payload_flag        = bch_payload_type::mac_full;
    pdu.bch_payload.bch_payload = bch_payload;

    return *this;
  }

  /// Sets the BCH payload and returns a reference to the builder. PHY configures the timing PBCH bits.
  /// \note Use this function when the PHY generates the timing PBCH information.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table MAC generated MIB PDU.
  /// \note This function assumes that given bch_payload value is codified as: 0,0,0,0,0,0,0,0,a0,a1,a2,...,a21,a22,a23,
  /// with the most significant bit being the leftmost (in this case a0 in position 24 of the uint32_t).
  dl_ssb_pdu_builder& set_bch_payload_phy_timing_info(uint32_t bch_payload)
  {
    pdu.bch_payload_flag = bch_payload_type::phy_timing_info;
    // Only use the 24 least significant bits.
    pdu.bch_payload.bch_payload = (bch_payload & 0xFFFFFF);

    return *this;
  }

  /// Sets the BCH payload configured by the PHY and returns a reference to the builder.
  /// \note Use this function when the PHY generates the full PBCH payload.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table PHY generated MIB PDU.
  dl_ssb_pdu_builder& set_bch_payload_phy_full(uint8_t dmrs_type_a_position,
                                               uint8_t pdcch_config_sib1,
                                               bool    cell_barred,
                                               bool    intra_freq_reselection)
  {
    pdu.bch_payload_flag                              = bch_payload_type::phy_full;
    pdu.bch_payload.phy_mib_pdu.dmrs_typeA_position   = dmrs_type_a_position;
    pdu.bch_payload.phy_mib_pdu.pdcch_config_sib1     = pdcch_config_sib1;
    pdu.bch_payload.phy_mib_pdu.cell_barred           = cell_barred ? 0 : 1;
    pdu.bch_payload.phy_mib_pdu.intrafreq_reselection = intra_freq_reselection ? 0 : 1;

    return *this;
  }

  /// Sets the maintenance v3 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table SSB/PBCH PDU maintenance FAPIv3.
  /// \note ssbPduIndex field is automatically filled when adding a new SSB PDU to the DL TTI request message.
  dl_ssb_pdu_builder&
  set_maintenance_v3_basic_parameters(ssb_pattern_case case_type, subcarrier_spacing scs, uint8_t l_max)
  {
    v3.case_type = case_type;
    v3.scs       = scs;
    v3.lmax      = l_max;

    return *this;
  }

  /// Sets the SSB power information and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.4, in table SSB/PBCH PDU maintenance FAPIv3.
  dl_ssb_pdu_builder& set_maintenance_v3_tx_power_info(optional<float> power_scaling_ss_pbch_dB,
                                                       optional<float> pss_to_sss_ratio_dB)
  {
    // Power scaling in SS-PBCH in hundredths of dBs.
    int ss_block_power = (power_scaling_ss_pbch_dB) ? static_cast<int>(power_scaling_ss_pbch_dB.value() * 100) : -32768;
    srsran_assert(ss_block_power <= std::numeric_limits<int16_t>::max(),
                  "SS PBCH block power scaling ({}) exceeds the maximum ({}).",
                  ss_block_power,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(ss_block_power >= std::numeric_limits<int16_t>::min(),
                  "SS PBCH block power scaling ({}) does not reach the minimum ({}).",
                  ss_block_power,
                  std::numeric_limits<int16_t>::min());
    v3.ss_pbch_block_power_scaling = static_cast<int16_t>(ss_block_power);

    // SSS to PSS ratio in thousandths of dBs.
    int beta_pss = (pss_to_sss_ratio_dB) ? static_cast<int>(pss_to_sss_ratio_dB.value() * 1000) : -32768;
    srsran_assert(beta_pss <= std::numeric_limits<int16_t>::max(),
                  "PSS to SSS ratio ({}) exceeds the maximum ({}).",
                  beta_pss,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(beta_pss >= std::numeric_limits<int16_t>::min(),
                  "PSS to SSS ratio ({}) does not reach the minimum ({}).",
                  beta_pss,
                  std::numeric_limits<int16_t>::min());

    v3.beta_pss_profile_sss = static_cast<int16_t>(beta_pss);

    return *this;
  }

  //: TODO: params v4 - MU-MIMO.
  // :TODO: beamforming.
private:
  dl_ssb_pdu&            pdu;
  dl_ssb_maintenance_v3& v3;
};

/// Helper class to fill in the DL DCI PDU parameters specified in SCF-222 v4.0 section 3.4.2.1, including the PDCCH PDU
/// maintenance FAPIv3 and PDCCH PDU FAPIv4 parameters.
class dl_dci_pdu_builder
{
public:
  dl_dci_pdu_builder(dl_dci_pdu&                                    pdu,
                     dl_pdcch_pdu_maintenance_v3::maintenance_info& pdu_v3,
                     dl_pdcch_pdu_parameters_v4::dci_params&        pdu_v4) :
    pdu(pdu), pdu_v3(pdu_v3), pdu_v4(pdu_v4)
  {
    pdu_v3.pdcch_data_power_offset_profile_sss = std::numeric_limits<int16_t>::min();
    pdu_v3.pdcch_dmrs_power_offset_profile_sss = std::numeric_limits<int16_t>::min();
  }

  /// Sets the basic parameters for the fields of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_basic_parameters(rnti_t   rnti,
                                           uint16_t nid_pdcch_data,
                                           uint16_t nrnti_pdcch_data,
                                           uint8_t  cce_index,
                                           uint8_t  aggregation_level)
  {
    pdu.rnti              = rnti;
    pdu.nid_pdcch_data    = nid_pdcch_data;
    pdu.nrnti_pdcch_data  = nrnti_pdcch_data;
    pdu.cce_index         = cce_index;
    pdu.aggregation_level = aggregation_level;

    return *this;
  }

  /// Sets the transmission power info parameters for the fields of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_tx_power_info_parameter(optional<float> power_control_offset_ss_profile_nr_dB)
  {
    int value = (power_control_offset_ss_profile_nr_dB)
                    ? static_cast<int>(power_control_offset_ss_profile_nr_dB.value())
                    : -127;

    srsran_assert(value <= std::numeric_limits<int8_t>::max(),
                  "SS profile NR ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int8_t>::max());
    srsran_assert(value >= std::numeric_limits<int8_t>::min(),
                  "SS profile NR ({}) does not reach the minimum ({}).",
                  value,
                  std::numeric_limits<int8_t>::min());

    pdu.power_control_offset_ss_profile_nr = static_cast<int8_t>(value);

    return *this;
  }

  /// Sets the payload of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_payload(span<const uint8_t> payload)
  {
    // :TODO: Confirm that the bit order is: bit0-bit7 are mapped to first byte of MSB - LSB.
    srsran_assert(payload.size() <= pdu.payload.capacity(), "Payload size exceeds maximum expected size");
    std::copy(payload.begin(), payload.end(), pdu.payload.begin());

    return *this;
  }

  // :TODO: Beamforming.

  /// Sets the maintenance v3 DCI parameters of the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU maintenance FAPIv3.
  dl_dci_pdu_builder& set_maintenance_v3_dci_parameters(bool            collocated_al16_candidate_present,
                                                        optional<float> pdcch_dmrs_power_offset_profile_sss_dB,
                                                        optional<float> pdcch_data_power_offset_profile_sss_dB)
  {
    pdu_v3.collocated_AL16_candidate = (collocated_al16_candidate_present) ? 1U : 0U;

    static constexpr int USE_OTHER_FIELDS = std::numeric_limits<int16_t>::min();

    int value = (pdcch_dmrs_power_offset_profile_sss_dB)
                    ? static_cast<int>(pdcch_dmrs_power_offset_profile_sss_dB.value() * 1000)
                    : USE_OTHER_FIELDS;

    srsran_assert(value <= std::numeric_limits<int16_t>::max(),
                  "PDCCH DMRS power offset profile SSS ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(value >= std::numeric_limits<int16_t>::min(),
                  "PDCCH DMRS power offset profile SSS ({}) exceeds the minimum ({}).",
                  value,
                  std::numeric_limits<int16_t>::min());

    pdu_v3.pdcch_dmrs_power_offset_profile_sss = static_cast<int16_t>(value);

    value = (pdcch_data_power_offset_profile_sss_dB)
                ? static_cast<int>(pdcch_data_power_offset_profile_sss_dB.value() * 1000)
                : USE_OTHER_FIELDS;

    srsran_assert(value <= std::numeric_limits<int16_t>::max(),
                  "PDCCH data power offset profile SSS ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(value >= std::numeric_limits<int16_t>::min(),
                  "PDCCH data power offset profile SSS ({}) exceeds the minimum ({}).",
                  value,
                  std::numeric_limits<int16_t>::min());

    pdu_v3.pdcch_data_power_offset_profile_sss = static_cast<int16_t>(value);

    return *this;
  }

  /// Sets the DCI parameters of the PDCCH parameters v4.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU parameters FAPIv4.
  dl_dci_pdu_builder& set_parameters_v4_dci(uint16_t nid_pdcch_dmrs)
  {
    pdu_v4.nid_pdcch_dmrs = nid_pdcch_dmrs;

    return *this;
  }

private:
  dl_dci_pdu&                                    pdu;
  dl_pdcch_pdu_maintenance_v3::maintenance_info& pdu_v3;
  dl_pdcch_pdu_parameters_v4::dci_params&        pdu_v4;
};

/// Helper class to fill in the DL PDCCH PDU parameters specified in SCF-222 v4.0 section 3.4.2.1.
class dl_pdcch_pdu_builder
{
public:
  explicit dl_pdcch_pdu_builder(dl_pdcch_pdu& pdu) : pdu(pdu) {}

  /// Sets the BWP parameters for the fields of the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_pdcch_pdu_builder& set_bwp_parameters(uint16_t           coreset_bwp_size,
                                           uint16_t           coreset_bwp_start,
                                           subcarrier_spacing scs,
                                           cyclic_prefix_type prefix)
  {
    pdu.coreset_bwp_size  = coreset_bwp_size;
    pdu.coreset_bwp_start = coreset_bwp_start;
    pdu.scs               = scs;
    pdu.cyclic_prefix     = prefix;

    return *this;
  }

  /// Sets the coreset parameters for the fields of the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_pdcch_pdu_builder& set_coreset_parameters(uint8_t                   start_symbol_index,
                                               uint8_t                   duration_symbols,
                                               span<const uint8_t>       freq_domain_resource,
                                               cce_to_reg_mapping_type   cce_req_mapping_type,
                                               uint8_t                   reg_bundle_size,
                                               uint8_t                   interleaver_size,
                                               pdcch_coreset_type        coreset_type,
                                               uint16_t                  shift_index,
                                               precoder_granularity_type precoder_granularity)
  {
    pdu.start_symbol_index   = start_symbol_index;
    pdu.duration_symbols     = duration_symbols;
    pdu.cce_reg_mapping_type = cce_req_mapping_type;
    pdu.reg_bundle_size      = reg_bundle_size;
    pdu.interleaver_size     = interleaver_size;
    pdu.coreset_type         = coreset_type;
    pdu.shift_index          = shift_index;
    pdu.precoder_granularity = precoder_granularity;

    // :TODO: confirm the format of the freq_domain_resource. Here it's expecting the LSB of the first Byte of the array
    // to contain the first bit of the frequency domain resources, and so on.
    srsran_assert(freq_domain_resource.size() == pdu.freq_domain_resource.size(), "Unexpected size mismatch");
    std::copy(freq_domain_resource.begin(), freq_domain_resource.end(), pdu.freq_domain_resource.begin());

    return *this;
  }

  /// Adds a DL DCI PDU to the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_dci_pdu_builder add_dl_dci()
  {
    // Save the size as the index value for the DL DCI.
    unsigned dci_id = pdu.dl_dci.size();

    pdu.dl_dci.emplace_back();
    pdu.maintenance_v3.info.emplace_back();
    pdu.parameters_v4.params.emplace_back();

    // Set the DL DCI index.
    dl_pdcch_pdu_maintenance_v3::maintenance_info& info = pdu.maintenance_v3.info.back();
    info.dci_index                                      = dci_id;

    dl_dci_pdu_builder builder(pdu.dl_dci.back(), info, pdu.parameters_v4.params.back());

    return builder;
  }

private:
  dl_pdcch_pdu& pdu;
};

/// Changes the value of a bit in the bitmap. When enable is true, it sets the bit, otherwise it clears the bit.
template <typename T>
void change_bitmap_status(T& bitmap, unsigned bit, bool enable)
{
  if (enable) {
    bitmap |= (1U << bit);
  } else {
    bitmap &= ~(1U << bit);
  }
}

/// Builder that helps to fill the parameters of a DL PDSCH codeword.
class dl_pdsch_codeword_builder
{
public:
  dl_pdsch_codeword_builder(dl_pdsch_codeword& cw, uint8_t& cbg_tx_information) :
    cw(cw), cbg_tx_information(cbg_tx_information)
  {}

  /// Sets the codeword basic parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder& set_basic_parameters(uint16_t target_code,
                                                  uint8_t  qam_mod,
                                                  uint8_t  mcs_index,
                                                  uint8_t  mcs_table,
                                                  uint8_t  rv_index,
                                                  uint32_t tb_size)
  {
    cw.target_code_rate = target_code;
    cw.qam_mod_order    = qam_mod;
    cw.mcs_index        = mcs_index;
    cw.mcs_table        = mcs_table;
    cw.rv_index         = rv_index;
    cw.tb_size          = tb_size;

    return *this;
  }

  /// Sets the maintenance v3 parameters of the codeword.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters V3.
  dl_pdsch_codeword_builder& set_maintenance_v3_parameters(uint8_t cbg_tx_info)
  {
    cbg_tx_information = cbg_tx_info;

    return *this;
  }

private:
  dl_pdsch_codeword& cw;
  uint8_t&           cbg_tx_information;
};

/// DL PDSCH PDU builder that helps to fill the parameters specified in SCF-222 v4.0 section 3.4.2.2.
class dl_pdsch_pdu_builder
{
public:
  explicit dl_pdsch_pdu_builder(dl_pdsch_pdu& pdu) : pdu(pdu)
  {
    pdu.pdu_bitmap                           = 0U;
    pdu.is_last_cb_present                   = 0U;
    pdu.pdsch_maintenance_v3.tb_crc_required = 0U;
  }

  /// Sets the basic parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_basic_parameters(bool enable_ptrs, bool enable_cbg_retx, rnti_t rnti)
  {
    static constexpr auto     PTRS_BIT          = 0U;
    static constexpr unsigned CBG_RETX_CTRL_BIT = 1U;

    change_bitmap_status<uint16_t>(pdu.pdu_bitmap, PTRS_BIT, enable_ptrs);
    change_bitmap_status<uint16_t>(pdu.pdu_bitmap, CBG_RETX_CTRL_BIT, enable_cbg_retx);

    pdu.rnti = rnti;

    return *this;
  }

  /// Sets the BWP parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder&
  set_bwp_parameters(uint16_t bwp_size, uint16_t bwp_start, subcarrier_spacing scs, cyclic_prefix_type prefix)
  {
    pdu.bwp_size      = bwp_size;
    pdu.bwp_start     = bwp_start;
    pdu.scs           = scs;
    pdu.cyclic_prefix = prefix;

    return *this;
  }

  /// Adds a codeword to the PDSCH PDU and returns a codeword builder to fill the codeword parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder add_codeword()
  {
    pdu.cws.emplace_back();
    pdu.pdsch_maintenance_v3.cbg_tx_information.emplace_back();

    dl_pdsch_codeword_builder builder(pdu.cws.back(), pdu.pdsch_maintenance_v3.cbg_tx_information.back());

    return builder;
  }

  /// Sets the codeword information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_codeword_information_parameters(uint16_t             n_id_pdsch,
                                                            uint8_t              num_layers,
                                                            uint8_t              trasnmission_scheme,
                                                            pdsch_ref_point_type ref_point)
  {
    pdu.nid_pdsch           = n_id_pdsch;
    pdu.num_layers          = num_layers;
    pdu.transmission_scheme = trasnmission_scheme;
    pdu.ref_point           = ref_point;

    return *this;
  }

  /// Sets the DMRS parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_dmrs_parameters(uint16_t                 dl_dmrs_symb_pos,
                                            dmrs_type                dmrs_config_type,
                                            uint16_t                 pdsch_dmrs_scrambling_id,
                                            uint16_t                 pdsch_dmrs_scrambling_id_complement,
                                            pdsch_low_papr_dmrs_type low_parp_dmrs,
                                            uint8_t                  nscid,
                                            uint8_t                  num_dmrs_cdm_groups_no_data,
                                            uint16_t                 dmrs_ports)
  {
    pdu.dl_dmrs_symb_pos               = dl_dmrs_symb_pos;
    pdu.dmrs_config_type               = dmrs_config_type;
    pdu.pdsch_dmrs_scrambling_id       = pdsch_dmrs_scrambling_id;
    pdu.pdsch_dmrs_scrambling_id_compl = pdsch_dmrs_scrambling_id_complement;
    pdu.low_papr_dmrs                  = low_parp_dmrs;
    pdu.nscid                          = nscid;
    pdu.num_dmrs_cdm_grps_no_data      = num_dmrs_cdm_groups_no_data;
    pdu.dmrs_ports                     = dmrs_ports;

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 0 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_0(span<const uint8_t>           rb_map,
                                                                 pdsch_vrb_to_prb_mapping_type vrb_to_prb_mapping)
  {
    pdu.resource_alloc     = pdsch_allocation_type::type_0;
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    srsran_assert(rb_map.size() <= dl_pdsch_pdu::MAX_SIZE_RB_BITMAP,
                  "[PDSCH Builder] - Incoming RB bitmap size {} exceeds FAPI bitmap field {}",
                  rb_map.size(),
                  dl_pdsch_pdu::MAX_SIZE_RB_BITMAP);

    std::copy(rb_map.begin(), rb_map.end(), pdu.rb_bitmap.begin());

    // Filling these fields, although they belong to allocation type 1.
    pdu.rb_start = 0;
    pdu.rb_size  = 0;

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 1 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_1(uint16_t                      rb_start,
                                                                 uint16_t                      rb_size,
                                                                 pdsch_vrb_to_prb_mapping_type vrb_to_prb_mapping)
  {
    pdu.resource_alloc     = pdsch_allocation_type::type_1;
    pdu.rb_start           = rb_start;
    pdu.rb_size            = rb_size;
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    return *this;
  }

  /// Sets the PDSCH allocation in time parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_time_parameters(uint8_t start_symbol_index, uint8_t nof_symbols)
  {
    pdu.start_symbol_index = start_symbol_index;
    pdu.nr_of_symbols      = nof_symbols;

    return *this;
  }

  // :TODO: PTRS.
  // :TODO: Beamforming.

  /// Sets the Tx Power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_tx_power_info_parameters(optional<int>      power_control_offset_profile_nr,
                                                     ss_profile_nr_type ss_profile_nr)
  {
    unsigned power_profile_nr =
        power_control_offset_profile_nr ? static_cast<unsigned>(power_control_offset_profile_nr.value() + 8U) : 255U;

    srsran_assert(power_profile_nr <= std::numeric_limits<uint8_t>::max(),
                  "Power control offset Profile NR value exceeds the maximum ({}).",
                  power_profile_nr);

    pdu.power_control_offset_profile_nr = static_cast<uint8_t>(power_profile_nr);

    pdu.power_control_offset_ss_profile_nr = ss_profile_nr;

    return *this;
  }

  /// Sets the CBG ReTx Ctrl parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_cbg_re_tx_ctrl_parameters(bool                 last_cb_present_first_tb,
                                                      bool                 last_cb_present_second_tb,
                                                      inline_tb_crc_type   tb_crc,
                                                      span<const uint32_t> dl_tb_crc_cw)
  {
    change_bitmap_status<uint8_t>(pdu.is_last_cb_present, 0, last_cb_present_first_tb);
    change_bitmap_status<uint8_t>(pdu.is_last_cb_present, 1, last_cb_present_second_tb);

    pdu.is_inline_tb_crc = tb_crc;

    srsran_assert(dl_tb_crc_cw.size() <= dl_pdsch_pdu::MAX_SIZE_DL_TB_CRC,
                  "[PDSCH Builder] - Incoming DL TB CRC size ({}) out of bounds ({})",
                  dl_tb_crc_cw.size(),
                  dl_pdsch_pdu::MAX_SIZE_DL_TB_CRC);
    std::copy(dl_tb_crc_cw.begin(), dl_tb_crc_cw.end(), pdu.dl_tb_crc_cw.begin());

    return *this;
  }

  /// Sets the maintenance v3 BWP information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_bwp_parameters(dl_pdsch_trans_type pdsch_trans_type,
                                                          uint16_t            coreset_start_point,
                                                          uint16_t            initial_dl_bwp_size)
  {
    pdu.pdsch_maintenance_v3.pdsch_trans_type    = pdsch_trans_type;
    pdu.pdsch_maintenance_v3.coreset_start_point = coreset_start_point;
    pdu.pdsch_maintenance_v3.initial_dl_bwp_size = initial_dl_bwp_size;

    return *this;
  }

  /// Sets the maintenance v3 codeword information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_codeword_parameters(ldpc_base_graph_type ldpc_base_graph,
                                                               uint32_t             tb_size_lbrm_bytes,
                                                               bool                 tb_crc_first_tb_required,
                                                               bool                 tb_crc_second_tb_required)
  {
    pdu.pdsch_maintenance_v3.ldpc_base_graph    = ldpc_base_graph;
    pdu.pdsch_maintenance_v3.tb_size_lbrm_bytes = tb_size_lbrm_bytes;

    // Fill the bitmap.
    change_bitmap_status<uint8_t>(pdu.pdsch_maintenance_v3.tb_crc_required, 0, tb_crc_first_tb_required);
    change_bitmap_status<uint8_t>(pdu.pdsch_maintenance_v3.tb_crc_required, 1, tb_crc_second_tb_required);

    return *this;
  }

  /// Sets the maintenance v3 rate matching references parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder&
  set_maintenance_v3_rm_references_parameters(span<const uint16_t> ssb_pdus_for_rm,
                                              uint16_t             ssb_config_for_rm,
                                              span<const uint8_t>  prb_sym_rm_pattern_bitmap_by_reference,
                                              uint16_t             pdcch_pdu_index,
                                              uint16_t             dci_index,
                                              span<const uint8_t>  lte_crs_rm_pattern,
                                              span<const uint16_t> csi_rs_for_rm)
  {
    srsran_assert(ssb_pdus_for_rm.size() <= dl_pdsch_maintenance_parameters_v3::MAX_SIZE_SSB_PDU_FOR_RM,
                  "[PDSCH Builder] - Incoming SSB PDUs for RM matching size ({}) doesn't fit the field ({})",
                  ssb_pdus_for_rm.size(),
                  dl_pdsch_maintenance_parameters_v3::MAX_SIZE_SSB_PDU_FOR_RM);
    std::copy(
        ssb_pdus_for_rm.begin(), ssb_pdus_for_rm.end(), pdu.pdsch_maintenance_v3.ssb_pdus_for_rate_matching.begin());

    pdu.pdsch_maintenance_v3.ssb_config_for_rate_matching = ssb_config_for_rm;

    pdu.pdsch_maintenance_v3.prb_sym_rm_patt_bmp_byref.assign(prb_sym_rm_pattern_bitmap_by_reference.begin(),
                                                              prb_sym_rm_pattern_bitmap_by_reference.end());

    // These two parameters are set to 0 for this release FAPI v4.
    pdu.pdsch_maintenance_v3.num_prb_sym_rm_patts_by_value = 0U;
    pdu.pdsch_maintenance_v3.num_coreset_rm_patterns       = 0U;

    pdu.pdsch_maintenance_v3.pdcch_pdu_index = pdcch_pdu_index;
    pdu.pdsch_maintenance_v3.dci_index       = dci_index;

    pdu.pdsch_maintenance_v3.lte_crs_rm_pattern.assign(lte_crs_rm_pattern.begin(), lte_crs_rm_pattern.end());
    pdu.pdsch_maintenance_v3.csi_for_rm.assign(csi_rs_for_rm.begin(), csi_rs_for_rm.end());

    return *this;
  }

  /// Sets the maintenance v3 Tx power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_tx_power_info_parameters(optional<float> dmrs_power_offset_profile_sss,
                                                                    optional<float> data_power_offset_profile_sss)
  {
    static constexpr int USE_OTHER_FIELDS = std::numeric_limits<int16_t>::min();

    int value = (dmrs_power_offset_profile_sss) ? static_cast<int>(dmrs_power_offset_profile_sss.value() * 1000)
                                                : USE_OTHER_FIELDS;

    srsran_assert(value <= std::numeric_limits<int16_t>::max(),
                  "PDSCH DMRS power offset profile SSS ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(value >= std::numeric_limits<int16_t>::min(),
                  "PDSCH DMRS power offset profile SSS ({}) exceeds the minimum ({}).",
                  value,
                  std::numeric_limits<int16_t>::min());

    pdu.pdsch_maintenance_v3.pdsch_dmrs_power_offset_profile_sss = static_cast<int16_t>(value);

    value = (data_power_offset_profile_sss) ? static_cast<int>(data_power_offset_profile_sss.value() * 1000)
                                            : USE_OTHER_FIELDS;

    srsran_assert(value <= std::numeric_limits<int16_t>::max(),
                  "PDSCH data power offset profile SSS ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int16_t>::max());
    srsran_assert(value >= std::numeric_limits<int16_t>::min(),
                  "PDSCH data power offset profile SSS ({}) exceeds the minimum ({}).",
                  value,
                  std::numeric_limits<int16_t>::min());

    pdu.pdsch_maintenance_v3.pdsch_data_power_offset_profile_sss = static_cast<int16_t>(value);

    return *this;
  }

  /// Sets the maintenance v3 CBG retx control parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_cbg_tx_crtl_parameters(uint8_t max_num_cbg_per_tb)
  {
    pdu.pdsch_maintenance_v3.max_num_cbg_per_tb = max_num_cbg_per_tb;

    return *this;
  }

  /// Sets the PDSCH-PTRS Tx power info parameter for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PTRS maintenance
  /// parameters v3.
  dl_pdsch_pdu_builder& set_ptrs_maintenance_v3_tx_power_info_parameters(optional<float> ptrs_power_offset_profile_sss)
  {
    static constexpr int USE_OTHER_FIELDS = std::numeric_limits<int16_t>::min();

    int value = ptrs_power_offset_profile_sss ? static_cast<int>(ptrs_power_offset_profile_sss.value() * 1000)
                                              : USE_OTHER_FIELDS;

    srsran_assert(value <= std::numeric_limits<int16_t>::max(),
                  "PDSCH PTRS power offset profile SSS ({}) exceeds the maximum ({}).",
                  value,
                  std::numeric_limits<int16_t>::max());

    srsran_assert(value >= std::numeric_limits<int16_t>::min(),
                  "PDSCH PTRS power offset profile SSS ({}) exceeds the minimum ({}).",
                  value,
                  std::numeric_limits<int16_t>::min());

    pdu.ptrs_maintenance_v3.pdsch_ptrs_power_offset_profile_sss = static_cast<int16_t>(value);

    return *this;
  }

  /// Sets the PDSCH maintenance v4 basic parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance FAPIv4.
  dl_pdsch_pdu_builder& set_maintenance_v4_basic_parameters(span<const uint8_t> coreset_rm_pattern_bitmap_by_reference,
                                                            uint8_t             lte_crs_mbsfn_derivation_method,
                                                            span<const uint8_t> lte_crs_mbsfn_pattern)
  {
    pdu.pdsch_parameters_v4.lte_crs_mbsfn_derivation_method = lte_crs_mbsfn_derivation_method;

    pdu.pdsch_parameters_v4.coreset_rm_pattern_bmp_by_ref.assign(coreset_rm_pattern_bitmap_by_reference.begin(),
                                                                 coreset_rm_pattern_bitmap_by_reference.end());

    // :TODO: check the incoming data format for this field. It will probably change once the MAC structure is defined.
    pdu.pdsch_parameters_v4.lte_crs_mbsfn_pattern.assign(lte_crs_mbsfn_pattern.begin(), lte_crs_mbsfn_pattern.end());

    return *this;
  }

  // :TODO: FAPIv4 MU-MIMO.

private:
  dl_pdsch_pdu& pdu;
};

/// Helper class to fill in the DL CSI-RS PDU parameters specified in SCF-222 v4.0 section 3.4.2.3.
class dl_csi_rs_pdu_builder
{
public:
  explicit dl_csi_rs_pdu_builder(dl_csi_rs_pdu& pdu) : pdu(pdu) {}

  // :TODO: add rest of parameters.
  dl_csi_rs_pdu_builder& set_basic_parameters(subcarrier_spacing scs)
  {
    pdu.scs = scs;

    return *this;
  }

private:
  dl_csi_rs_pdu& pdu;
};

/// DL_TTI.request message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.2.
class dl_tti_request_message_builder
{
  /// Maximum number of DL PDU types supported. The value is specified in SCF-222 v4.0 section 3.4.2.
  static constexpr unsigned NUM_DL_TYPES = 5;

public:
  /// Constructs a builder that will help to fill the given DL TTI request message.
  explicit dl_tti_request_message_builder(dl_tti_request_message& msg) : msg(msg) { msg.num_dl_types = NUM_DL_TYPES; }

  /// Sets the DL TTI request basic parameters and returns a reference to the builder.
  /// \note nPDUs and nPDUsOfEachType properties are filled by the add_*_pdu() functions.
  /// \note these parameters are specified in SCF-222 v4.0 section 3.4.2 in table DL_TTI.request message body.
  dl_tti_request_message_builder& set_basic_parameters(uint16_t sfn, uint16_t slot, uint16_t n_group)
  {
    msg.sfn        = sfn;
    msg.slot       = slot;
    msg.num_groups = n_group;

    return *this;
  }

  /// Adds a PDCCH PDU to the message, fills its basic parameters using the given arguments and returns a PDCCH PDU
  /// builder.
  dl_pdcch_pdu_builder add_pdcch_pdu()
  {
    // Add a new pdu.
    msg.pdus.emplace_back();
    dl_tti_request_pdu& pdu = msg.pdus.back();

    // Fill the pdcch pdu index value. The index value will be the index of the pdu in the array of PDCCH pdus.
    dl_pdcch_pdu_maintenance_v3& info          = pdu.pdcch_pdu.maintenance_v3;
    auto&                        num_pdcch_pdu = msg.num_pdus_of_each_type[static_cast<int>(dl_pdu_type::PDCCH)];
    info.pdcch_pdu_index                       = num_pdcch_pdu;

    // Increase the number of SSB pdus in the request.
    ++num_pdcch_pdu;

    pdu.pdu_type = dl_pdu_type::PDCCH;

    dl_pdcch_pdu_builder builder(pdu.pdcch_pdu);

    return builder;
  }

  /// Adds a PDSCH PDU to the message, fills its basic parameters using the given arguments and returns a PDSCH PDU
  /// builder.
  dl_pdsch_pdu_builder add_pdsch_pdu(bool enable_ptrs, bool enable_cbg_retx, rnti_t rnti)
  {
    // Add a new PDU.
    msg.pdus.emplace_back();
    dl_tti_request_pdu& pdu = msg.pdus.back();

    // Fill the SSB PDU index value. The index value will be the index of the PDU in the array of SSB pdus.
    auto& num_pdsch_pdu     = msg.num_pdus_of_each_type[static_cast<int>(dl_pdu_type::PDSCH)];
    pdu.pdsch_pdu.pdu_index = num_pdsch_pdu;

    // Increase the number of PDSCH PDU.
    ++num_pdsch_pdu;

    pdu.pdu_type = dl_pdu_type::PDSCH;

    dl_pdsch_pdu_builder builder(pdu.pdsch_pdu);

    builder.set_basic_parameters(enable_ptrs, enable_cbg_retx, rnti);

    return builder;
  }

  /// Adds a CSI-RS PDU to the message and returns a CSI-RS PDU builder.
  dl_csi_rs_pdu_builder add_csi_rs_pdu()
  {
    ++msg.num_pdus_of_each_type[static_cast<int>(dl_pdu_type::CSI_RS)];

    // Add a new PDU.
    msg.pdus.emplace_back();
    dl_tti_request_pdu& pdu = msg.pdus.back();
    pdu.pdu_type            = dl_pdu_type::CSI_RS;

    dl_csi_rs_pdu_builder builder(pdu.csi_rs_pdu);

    return builder;
  }

  /// Adds a SSB PDU to the message, fills its basic parameters using the given arguments and returns a SSB PDU builder.
  dl_ssb_pdu_builder add_ssb_pdu(pci_t                 phys_cell_id,
                                 beta_pss_profile_type beta_pss_profile_nr,
                                 uint8_t               ssb_block_index,
                                 uint8_t               ssb_subcarrier_offset,
                                 uint16_t              ssb_offset_pointA)
  {
    // Add a new PDU.
    msg.pdus.emplace_back();
    dl_tti_request_pdu& pdu = msg.pdus.back();

    // Fill the SSB PDU index value. The index value will be the index of the PDU in the array of SSB pdus.
    dl_ssb_maintenance_v3& info        = pdu.ssb_pdu.ssb_maintenance_v3;
    auto&                  num_ssb_pdu = msg.num_pdus_of_each_type[static_cast<int>(dl_pdu_type::SSB)];
    info.ssb_pdu_index                 = num_ssb_pdu;

    // Increase the number of SSB PDUs in the request.
    ++num_ssb_pdu;

    pdu.pdu_type = dl_pdu_type::SSB;

    dl_ssb_pdu_builder builder(pdu.ssb_pdu);

    // Fill the PDU basic parameters.
    builder.set_basic_parameters(
        phys_cell_id, beta_pss_profile_nr, ssb_block_index, ssb_subcarrier_offset, ssb_offset_pointA);

    return builder;
  }

  //: TODO: groups array
  //: TODO: top level rate match patterns

private:
  dl_tti_request_message& msg;
};

} // namespace fapi
} // namespace srsgnb

#endif // SRSGNB_FAPI_MESSAGE_BUILDERS_H
