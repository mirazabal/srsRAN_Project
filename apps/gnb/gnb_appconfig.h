/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/adt/variant.h"
#include "srsran/ran/band_helper.h"
#include "srsran/ran/bs_channel_bandwidth.h"
#include "srsran/ran/cyclic_prefix.h"
#include "srsran/ran/five_qi.h"
#include "srsran/ran/pci.h"
#include "srsran/ran/pdcch/search_space.h"
#include "srsran/ran/pdsch/pdsch_mcs.h"
#include "srsran/ran/pusch/pusch_mcs.h"
#include "srsran/ran/rnti.h"
#include "srsran/ran/subcarrier_spacing.h"
#include <string>
#include <thread>
#include <vector>

namespace srsran {

/// PRACH application configuration.
struct prach_appconfig {
  /// PRACH configuration index.
  unsigned prach_config_index = 1;
  /// PRACH root sequence index.
  unsigned prach_root_sequence_index = 1;
  /// Zero correlation zone
  unsigned zero_correlation_zone = 0;
  unsigned fixed_msg3_mcs        = 0;
  unsigned max_msg3_harq_retx    = 4;
  /// Total number of PRACH preambles used for contention based and contention free 4-step or 2-step random access.
  optional<unsigned> total_nof_ra_preambles;
  /// Offset of lowest PRACH transmission occasion in frequency domain respective to PRB 0. To minimize interference
  /// with the PUCCH, the user should leave some guardband between the PUCCH CRBs and the PRACH PRBs.
  /// Possible values: {0,...,MAX_NOF_PRB - 1}.
  unsigned prach_frequency_start = 6;
};

/// TDD configuration. See TS 38.331, \c TDD-UL-DL-Pattern.
struct tdd_ul_dl_appconfig {
  /// Periodicity of the DL-UL pattern in Milliseconds. Values {0.5, 0.625, 1, 1.25, 2, 2.5, 5, 10}.
  float dl_ul_tx_period = 5.0F;
  /// Values: {0,...,maxNrofSlots=80}.
  unsigned nof_dl_slots = 6;
  /// Values: {0,...,maxNrofSymbols-1=13}.
  unsigned nof_dl_symbols = 0;
  /// Values: {0,...,maxNrofSlots=80}.
  unsigned nof_ul_slots = 3;
  /// Values: {0,...,maxNrofSymbols-1=13}.
  unsigned nof_ul_symbols = 0;
};

/// PDCCH application configuration.
struct pdcch_appconfig {
  /// Use an UE-dedicated or Common Search Space.
  search_space_configuration::type_t ue_ss_type = search_space_configuration::type_t::ue_dedicated;
  /// Flag specifying whether to use non-fallback or fallback DCI format in UE dedicated SearchSpace.
  bool dci_format_0_1_and_1_1 = true;
};

/// PDSCH application configuration.
struct pdsch_appconfig {
  /// Minimum modulation and coding scheme index for C-RNTI PDSCH allocations. Note that setting a high minimum MCS
  /// may lead to a high BLER if the SINR is low.
  unsigned min_ue_mcs = 0;
  /// Maximum modulation and coding scheme index for C-RNTI PDSCH allocations. To set a fixed MCS, set \c min_ue_mcs
  /// equal to the \c max_ue_mcs.
  unsigned max_ue_mcs = 28;
  /// RAR modulation and coding scheme index.
  unsigned fixed_rar_mcs = 0;
  /// SI modulation and coding scheme index.
  unsigned fixed_sib1_mcs = 5;
  /// Number of UE DL HARQ processes.
  unsigned nof_harqs = 16;
  /// Maximum number of consecutive DL KOs before an RLF is reported.
  unsigned max_consecutive_kos = 100;
  /// Redundancy version sequence to use. Each element can have one of the following values: {0, 1, 2, 3}.
  std::vector<unsigned> rv_sequence = {0, 2, 3, 1};
  /// MCS table to use for PDSCH
  pdsch_mcs_table mcs_table = pdsch_mcs_table::qam64;
};

/// PUSCH application configuration.
struct pusch_appconfig {
  /// \brief Minimum modulation and coding scheme index for C-RNTI PUSCH allocations. Note that setting a high minimum
  /// MCS may lead to a high BLER if the SINR is low.
  unsigned min_ue_mcs = 0;
  /// Maximum modulation and coding scheme index for C-RNTI PUSCH allocations. To set a fixed MCS, set \c min_ue_mcs
  /// equal to the \c max_ue_mcs.
  unsigned max_ue_mcs = 28;
  /// Maximum number of consecutive UL KOs before an RLF is reported.
  unsigned max_consecutive_kos = 100;
  /// Redundancy version sequence to use. Each element can have one of the following values: {0, 1, 2, 3}.
  std::vector<unsigned> rv_sequence = {0};
  /// MCS table to use for PUSCH
  pusch_mcs_table mcs_table = pusch_mcs_table::qam64;
};

/// Amplitude control application configuration.
struct amplitude_control_appconfig {
  /// Baseband gain back-off. This accounts for the signal PAPR and is applied regardless of clipping settings.
  float gain_backoff_dB = 12.0F;
  /// Power ceiling in dB, relative to the full scale amplitude of the radio.
  float power_ceiling_dBFS = -0.1F;
  /// Clipping of the baseband samples. If enabled, the samples that exceed the power ceiling are clipped.
  bool enable_clipping = false;
};

/// Base cell configuration.
struct base_cell_appconfig {
  /// Physical cell identifier.
  pci_t pci = 1;
  /// Downlink arfcn.
  unsigned dl_arfcn = 536020;
  /// NR band.
  optional<nr_band> band;
  /// Channel bandwidth in MHz.
  bs_channel_bandwidth_fr1 channel_bw_mhz = bs_channel_bandwidth_fr1::MHz20;
  /// Number of antennas in downlink.
  unsigned nof_antennas_dl = 1;
  /// Number of antennas in uplink.
  unsigned nof_antennas_ul = 1;
  /// Human readable full PLMN (without possible filler digit).
  std::string plmn = "00101";
  /// TAC.
  unsigned tac = 7;
  /// SSB period in milliseconds.
  unsigned ssb_period_msec = 10;
  /// PDCCH configuration.
  pdcch_appconfig pdcch_cfg;
  /// PDSCH configuration.
  pdsch_appconfig pdsch_cfg;
  /// PRACH configuration.
  prach_appconfig prach_cfg;
  /// PUSCH configuration.
  pusch_appconfig pusch_cfg;
  /// Common subcarrier spacing for the entire resource grid. It must be supported by the band SS raster.
  subcarrier_spacing common_scs = subcarrier_spacing::kHz15;
  /// TDD slot configuration.
  optional<tdd_ul_dl_appconfig> tdd_ul_dl_cfg;
};

/// Cell configuration
struct cell_appconfig {
  /// Cell configuration.
  base_cell_appconfig cell;
};

/// RLC UM TX configuration
struct rlc_tx_um_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_reassembly;    ///< Timer used by rx to detect PDU loss (ms)
};

/// RLC UM RX configuration
struct rlc_rx_um_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_reassembly;    ///< Timer used by rx to detect PDU loss (ms)
};

/// RLC UM configuration
struct rlc_um_appconfig {
  rlc_tx_um_appconfig tx;
  rlc_rx_um_appconfig rx;
};

/// RLC UM TX configuration
struct rlc_tx_am_appconfig {
  uint16_t sn_field_length; ///< Number of bits used for sequence number
  int32_t  t_poll_retx;     ///< Poll retx timeout (ms)
  uint32_t max_retx_thresh; ///< Max retx threshold
  int32_t  poll_pdu;        ///< Insert poll bit after this many PDUs
  int32_t  poll_byte;       ///< Insert poll bit after this much data (bytes)
};

/// RLC UM RX configuration
struct rlc_rx_am_appconfig {
  uint16_t sn_field_length;   ///< Number of bits used for sequence number
  int32_t  t_reassembly;      ///< Timer used by rx to detect PDU loss (ms)
  int32_t  t_status_prohibit; ///< Timer used by rx to prohibit tx of status PDU (ms)
};

/// RLC AM configuration
struct rlc_am_appconfig {
  rlc_tx_am_appconfig tx;
  rlc_rx_am_appconfig rx;
};

/// RLC configuration
struct rlc_appconfig {
  std::string      mode = "am";
  rlc_um_appconfig um;
  rlc_am_appconfig am;
};

/// F1-U configuration at DU side
struct f1u_du_appconfig {
  int32_t t_notify; ///< Maximum backoff time for transmit/delivery notifications from DU to CU_UP (ms)
};

/// F1-U configuration at CU_UP side
struct f1u_cu_up_appconfig {
  int32_t t_notify; ///< Maximum backoff time for discard notifications from CU_UP to DU (ms)
};

struct pdcp_rx_appconfig {
  uint16_t sn_field_length;       ///< Number of bits used for sequence number
  int32_t  t_reordering;          ///< Timer used to detect PDUs losses (ms)
  bool     out_of_order_delivery; ///< Whether out-of-order delivery to upper layers is enabled
};

struct pdcp_tx_appconfig {
  uint16_t sn_field_length;        ///< Number of bits used for sequence number
  int32_t  discard_timer;          ///< Timer used to notify lower layers to discard PDUs (ms)
  bool     status_report_required; ///< Whether PDCP status report is required
};

struct pdcp_appconfig {
  bool              integrity_protection_required; ///< Whether DRB integrity is required
  pdcp_tx_appconfig tx;
  pdcp_rx_appconfig rx;
};
/// QoS configuration
struct qos_appconfig {
  five_qi_t           five_qi = uint_to_five_qi(9);
  rlc_appconfig       rlc;
  f1u_du_appconfig    f1u_du;
  f1u_cu_up_appconfig f1u_cu_up;
  pdcp_appconfig      pdcp;
};

struct amf_appconfig {
  std::string ip_addr                = "127.0.0.1";
  uint16_t    port                   = 38412;
  std::string bind_addr              = "127.0.0.1";
  int         sctp_rto_initial       = 120;
  int         sctp_rto_min           = 120;
  int         sctp_rto_max           = 500;
  int         sctp_init_max_attempts = 3;
  int         sctp_max_init_timeo    = 500;
};

struct cu_cp_appconfig {
  int inactivity_timer = 7200; // in seconds
};

struct log_appconfig {
  std::string filename    = "/tmp/gnb.log"; // Path to write log file or "stdout" to print to console.
  std::string all_level   = "warning";      // Default log level for all layers.
  std::string lib_level   = "warning"; // Generic log level assigned to library components without layer-specific level.
  std::string du_level    = "warning";
  std::string cu_level    = "warning";
  std::string phy_level   = "warning";
  std::string radio_level = "info";
  std::string mac_level   = "warning";
  std::string rlc_level   = "warning";
  std::string f1ap_level  = "warning";
  std::string f1u_level   = "warning";
  std::string pdcp_level  = "warning";
  std::string rrc_level   = "warning";
  std::string ngap_level  = "warning";
  std::string sdap_level  = "warning";
  std::string gtpu_level  = "warning";
  std::string sec_level   = "warning";
  std::string fapi_level  = "warning";
  int         hex_max_size      = 0;     // Maximum number of bytes to write when dumping hex arrays.
  bool        broadcast_enabled = false; // Set to true to log broadcasting messages and all PRACH opportunities.
  std::string phy_rx_symbols_filename;   // Set to a valid file path to print the received symbols.
};

struct pcap_appconfig {
  struct {
    std::string filename = "/tmp/gnb_ngap.pcap";
    bool        enabled  = false;
  } ngap;
  struct {
    std::string filename = "/tmp/gnb_e1ap.pcap";
    bool        enabled  = false;
  } e1ap;
  struct {
    std::string filename = "/tmp/gnb_f1ap.pcap";
    bool        enabled  = false;
  } f1ap;
  struct {
    std::string filename = "/tmp/gnb_mac.pcap";
    bool        enabled  = false;
  } mac;
};

/// Lower physical layer thread profiles.
enum class lower_phy_thread_profile {
  /// Same task worker as the rest of the PHY (ZMQ only).
  blocking = 0,
  /// Single task worker for all the lower physical layer task executors.
  single,
  /// Two task workers - one for the downlink and one for the uplink.
  dual,
  /// Dedicated task workers for each of the subtasks (downlink processing, uplink processing, reception and
  /// transmission).
  quad
};

/// Expert upper physical layer configuration.
struct expert_upper_phy_appconfig {
  /// Number of threads for processing PUSCH and PUCCH. It is set to 4 by default unless the available hardware
  /// concurrency is limited, in which case the most suitable number of threads between one and three will be selected.
  unsigned nof_ul_threads = std::min(4U, std::max(std::thread::hardware_concurrency(), 4U) - 3U);
  /// Number of PUSCH LDPC decoder iterations.
  unsigned pusch_decoder_max_iterations = 6;
  /// Set to true to enable the PUSCH LDPC decoder early stop.
  bool pusch_decoder_early_stop = true;
};

struct test_mode_ue_appconfig {
  /// C-RNTI to assign to the test UE.
  rnti_t rnti = INVALID_RNTI;
  /// Whether PDSCH grants are automatically assigned to the test UE.
  bool pdsch_active = true;
  /// Whether PUSCH grants are automatically assigned to the test UE.
  bool pusch_active = true;
};

/// gNB app Test Mode configuration.
struct test_mode_appconfig {
  /// Creates a UE with the given the given params for testing purposes.
  test_mode_ue_appconfig test_ue;
};

/// Expert generic Radio Unit configuration.
struct ru_gen_expert_appconfig {
  /// Lower physical layer thread profile.
  lower_phy_thread_profile lphy_executor_profile = lower_phy_thread_profile::dual;
};

/// gNB app generic Radio Unit cell configuration.
struct ru_gen_cell_appconfig {
  /// Amplitude control configuration.
  amplitude_control_appconfig amplitude_cfg;
};

/// gNB app generic Radio Unit configuration.
struct ru_gen_appconfig {
  /// Sampling frequency in MHz.
  double srate_MHz = 61.44;
  /// RF driver name.
  std::string device_driver = "uhd";
  /// RF driver arguments.
  std::string device_arguments = "";
  /// All transmit channel gain in decibels.
  double tx_gain_dB = 50.0;
  /// All receive channel gain in decibels.
  double rx_gain_dB = 60.0;
  /// Center frequency offset in hertz applied to all radio channels.
  double center_freq_offset_Hz = 0.0;
  /// Clock calibration in Parts Per Million (PPM). It is applied to the carrier frequency.
  double calibrate_clock_ppm = 0.0;
  /// LO Offset in MHz. It shifts the LO from the center frequency for moving the LO leakage out of the channel.
  double lo_offset_MHz = 0.0;
  /// \brief Rx to Tx radio time alignment calibration in samples.
  ///
  /// Compensates for the reception and transmission time misalignment inherent to the RF device. Setting this parameter
  /// overrides the default calibration, which is dependent on the selected RF device driver. Positive values reduce the
  /// RF transmission delay with respect to the RF reception. Since the UE receives the DL signal earlier, an incoming
  /// PRACH will also be detected earlier within the reception window. Negative values have the opposite effect, for
  /// example, a value of -1000 at a sample rate of 61.44 MHz increases the transmission delay and causes an incoming
  /// PRACH to be detected 16.3 us later within the reception window.
  optional<int> time_alignment_calibration;
  /// Synchronization source.
  std::string synch_source = "default";
  /// Clock source.
  std::string clock_source = "default";
  /// Over-the wire format. Determines the format in which samples are transported from the radio to the host.
  std::string otw_format = "default";
  /// Expert generic Radio Unit settings.
  ru_gen_expert_appconfig expert_cfg;
  /// Generic Radio Unit cells configuration.
  std::vector<ru_gen_cell_appconfig> cells = {{}};
};

/// gNB app Open Fronthaul cell configuration.
struct ru_ofh_cell_appconfig {
  /// Ethernet network interface name.
  std::string network_interface = "enp1s0f0";
  /// Radio Unit MAC address.
  std::string ru_mac_address = "70:b3:d5:e1:5b:06";
  /// Distributed Unit MAC address.
  std::string du_mac_address = "00:11:22:33:00:77";
  /// V-LAN Tag control information field.
  uint16_t vlan_tag = 1U;
  /// RU PRACH port.
  unsigned ru_prach_port_id = 4U;
  /// RU Downlink ports.
  std::vector<unsigned> ru_dl_ports = {0, 1};
  /// RU Uplink port.
  unsigned ru_ul_port = 0U;
};

/// gNB app Open Fronthaul Radio Unit configuration.
struct ru_ofh_appconfig {
  /// Sets the maximum allowed processing delay in slots.
  unsigned max_processing_delay_slots = 2U;
  /// GPS Alpha - Valid value range: [0, 1.2288e7].
  unsigned gps_Alpha = 0;
  /// GPS Beta - Valid value range: [-32768, 32767].
  int gps_Beta = 0;
  /// \brief RU operating bandwidth.
  ///
  /// Set this option when the operating bandwidth of the RU is larger than the configured bandwidth of the cell.
  optional<bs_channel_bandwidth_fr1> ru_operating_bw;
  /// T1a maximum parameter for downlink Control-Plane in microseconds.
  unsigned T1a_max_cp_dl = 500U;
  /// T1a minimum parameter for downlink Control-Plane in microseconds.
  unsigned T1a_min_cp_dl = 258U;
  /// T1a maximum parameter for uplink Control-Plane in microseconds.
  unsigned T1a_max_cp_ul = 500U;
  /// T1a minimum parameter for uplink Control-Plane in microseconds.
  unsigned T1a_min_cp_ul = 285U;
  /// T1a maximum parameter for downlink User-Plane in microseconds.
  unsigned T1a_max_up = 300U;
  /// T1a minimum parameter for downlink User-Plane in microseconds.
  unsigned T1a_min_up = 85U;
  /// Enables the Control-Plane PRACH message signalling.
  bool is_prach_control_plane_enabled = false;
  /// \brief Downlink broadcast flag.
  ///
  /// If enabled, broadcasts the contents of a single antenna port to all downlink RU eAXCs.
  bool is_downlink_broadcast_enabled = false;
  /// Uplink compression method.
  std::string compression_method_ul = "bfp";
  /// Uplink compression bitwidth.
  unsigned compresion_bitwidth_ul = 9;
  /// Downlink compression method.
  std::string compression_method_dl = "bfp";
  /// Downlink compression bitwidth.
  unsigned compresion_bitwidth_dl = 9;
  /// IQ data scaling to be applied prior to Downlink data compression.
  float iq_scaling = 0.35F;
  /// Individual Open Fronthaul cells configurations.
  std::vector<ru_ofh_cell_appconfig> cells = {{}};
};

/// gNB app Radio Unit configuration.
struct ru_appconfig {
  variant<ru_gen_appconfig, ru_ofh_appconfig> ru_cfg = {ru_gen_appconfig{}};
};

/// Monolithic gnb application configuration.
struct gnb_appconfig {
  /// Logging configuration.
  log_appconfig log_cfg;
  /// PCAP configuration.
  pcap_appconfig pcap_cfg;
  /// gNodeB identifier.
  uint32_t gnb_id = 411;
  /// Length of gNB identity in bits. Values {22,...,32}.
  uint8_t gnb_id_bit_length = 32;
  /// Node name.
  std::string ran_node_name = "srsgnb01";
  /// AMF configuration.
  amf_appconfig amf_cfg;
  /// CU-CP configuration.
  cu_cp_appconfig cu_cp_cfg;
  /// Radio Unit configuration.
  ru_appconfig ru_cfg;
  /// \brief Base cell application configuration.
  ///
  /// \note When a cell is added, it will use the values of this base cell as default values for its base cell
  /// configuration. This parameter usage is restricted for filling cell information in the \remark cell_cfg variable.
  base_cell_appconfig common_cell_cfg;
  /// \brief Cell configuration.
  ///
  /// \note Add one cell by default.
  std::vector<cell_appconfig> cells_cfg = {{}};

  /// \brief QoS configuration.
  std::vector<qos_appconfig> qos_cfg;

  /// Expert physical layer configuration.
  expert_upper_phy_appconfig expert_phy_cfg;

  /// Configuration for testing purposes.
  test_mode_appconfig test_mode_cfg = {};
};

} // namespace srsran
