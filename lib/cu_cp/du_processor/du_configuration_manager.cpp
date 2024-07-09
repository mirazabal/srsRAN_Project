/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_configuration_manager.h"
#include "srsran/rrc/rrc_config.h"

using namespace srsran;
using namespace srs_cu_cp;

static error_type<du_setup_result::rejected> validate_cell_config(const cu_cp_du_served_cells_item& served_cell)
{
  if (not served_cell.served_cell_info.five_gs_tac.has_value()) {
    return make_unexpected(du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state,
                                                     fmt::format("Missing TAC for cell")});
  }

  if (not served_cell.gnb_du_sys_info.has_value()) {
    return make_unexpected(du_setup_result::rejected{cause_protocol_t::semantic_error,
                                                     fmt::format("Missing system information for cell")});
  }

  return {};
}

class du_configuration_manager::du_configuration_handler_impl : public du_configuration_handler
{
public:
  du_configuration_handler_impl(du_configuration_manager& parent_) : parent(parent_) {}
  ~du_configuration_handler_impl() override
  {
    if (ctxt != nullptr) {
      parent.rem_du(this->ctxt->id);
    }
  }

  validation_result handle_new_du_config(const du_setup_request& req) override
  {
    if (this->ctxt != nullptr) {
      return make_unexpected(
          du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state, "DU already configured"});
    }
    auto ret = parent.add_du_config(req);
    if (ret.has_value()) {
      this->ctxt = ret.value();
      return {};
    }
    return make_unexpected(ret.error());
  }

  validation_result handle_du_config_update(const du_config_update_request& req) override
  {
    if (this->ctxt == nullptr) {
      return make_unexpected(du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state,
                                                       "DU with same gNB-DU-Id was not setup"});
    }

    // Reconfiguration.
    auto ret = parent.handle_du_config_update(*this->ctxt, req);
    if (not ret.has_value()) {
      return make_unexpected(ret.error());
    }
    this->ctxt = ret.value();
    return {};
  }

private:
  du_configuration_manager& parent;
};

du_configuration_manager::du_configuration_manager(const rrc_cfg_t& rrc_cfg_) :
  rrc_cfg(rrc_cfg_), logger(srslog::fetch_basic_logger("CU-CP"))
{
}

std::unique_ptr<du_configuration_handler> du_configuration_manager::create_du_handler()
{
  return std::make_unique<du_configuration_handler_impl>(*this);
}

static du_cell_configuration create_du_cell_config(du_cell_index_t                   cell_idx,
                                                   const cu_cp_du_served_cells_item& f1ap_cell_cfg)
{
  const auto&           cell_req = f1ap_cell_cfg.served_cell_info;
  du_cell_configuration cell;
  cell.cell_index = cell_idx;
  cell.cgi        = cell_req.nr_cgi;
  cell.tac        = cell_req.five_gs_tac.value();
  cell.pci        = cell_req.nr_pci;
  // Add band information.
  if (cell_req.nr_mode_info.fdd.has_value()) {
    for (const auto& band : cell_req.nr_mode_info.fdd.value().dl_nr_freq_info.freq_band_list_nr) {
      cell.bands.push_back(uint_to_nr_band(band.freq_band_ind_nr));
    }
  } else if (cell_req.nr_mode_info.tdd.has_value()) {
    for (const auto& band : cell_req.nr_mode_info.tdd.value().nr_freq_info.freq_band_list_nr) {
      cell.bands.push_back(uint_to_nr_band(band.freq_band_ind_nr));
    }
  }
  // Add packed MIB and SIB1
  cell.sys_info.packed_mib  = f1ap_cell_cfg.gnb_du_sys_info->mib_msg.copy();
  cell.sys_info.packed_sib1 = f1ap_cell_cfg.gnb_du_sys_info->sib1_msg.copy();
  return cell;
}

expected<const du_configuration_context*, du_setup_result::rejected>
du_configuration_manager::add_du_config(const du_setup_request& req)
{
  // Validate config.
  auto result = validate_new_du_config(req);
  if (not result.has_value()) {
    return make_unexpected(result.error());
  }

  // Create new DU config context.
  auto                      ret  = dus.emplace(req.gnb_du_id, du_configuration_context{});
  du_configuration_context& ctxt = ret.first->second;
  ctxt.id                        = req.gnb_du_id;
  ctxt.name                      = req.gnb_du_name;
  ctxt.rrc_version               = req.gnb_du_rrc_version;
  ctxt.served_cells.resize(req.gnb_du_served_cells_list.size());
  for (unsigned i = 0; i != ctxt.served_cells.size(); ++i) {
    ctxt.served_cells[i] = create_du_cell_config(uint_to_du_cell_index(i), req.gnb_du_served_cells_list[i]);
  }
  return &ctxt;
}

expected<const du_configuration_context*, du_setup_result::rejected>
du_configuration_manager::handle_du_config_update(const du_configuration_context& current_ctxt,
                                                  const du_config_update_request& req)
{
  if (current_ctxt.id != req.gnb_du_id) {
    logger.warning("du_id={}: Failed to update DU. Cause: DU ID mismatch", current_ctxt.id);
    return nullptr;
  }
  auto it = dus.find(current_ctxt.id);
  if (it == dus.end()) {
    logger.error("du_id={}: DU config update called for non-existent DU", current_ctxt.id);
    return nullptr;
  }

  // Validate config.
  if (not validate_du_config_update(req)) {
    return nullptr;
  }

  // Update DU config.
  du_configuration_context& du_context = it->second;
  // > Remove cells.
  for (const nr_cell_global_id_t& cgi : req.served_cells_to_rem) {
    auto cell_it = std::find_if(it->second.served_cells.begin(),
                                it->second.served_cells.end(),
                                [&cgi](const du_cell_configuration& item) { return item.cgi == cgi; });
    if (cell_it != it->second.served_cells.end()) {
      du_context.served_cells.erase(cell_it);
    } else {
      logger.warning(
          "du_id={}: Failed to remove cell nci={}. Cause: It was not previously set", current_ctxt.id, cgi.nci);
    }
  }
  // > Add new cells.
  for (const auto& cell_to_add : req.served_cells_to_add) {
    // Allocate cell index.
    du_cell_index_t cell_idx = du_cell_index_t::invalid;
    for (unsigned i = 0; i != MAX_NOF_DU_CELLS; ++i) {
      if (std::none_of(
              du_context.served_cells.begin(), du_context.served_cells.end(), [i](const du_cell_configuration& item) {
                return item.cell_index == uint_to_du_cell_index(i);
              })) {
        cell_idx = uint_to_du_cell_index(i);
        break;
      }
    }
    // Note: Existence of a free cell index should be guaranteed during validation.
    srsran_assert(cell_idx != du_cell_index_t::invalid, "Failed to allocate cell index");

    du_context.served_cells.push_back(create_du_cell_config(cell_idx, cell_to_add));
  }
  return &it->second;
}

void du_configuration_manager::rem_du(gnb_du_id_t du_id)
{
  auto it = dus.find(du_id);
  if (it == dus.end()) {
    logger.warning("du={}: Failed to remove DU. Cause: DU not found", du_id);
    return;
  }
  dus.erase(it);
}

error_type<du_setup_result::rejected>
du_configuration_manager::validate_new_du_config(const du_setup_request& req) const
{
  if (req.gnb_du_served_cells_list.size() > MAX_NOF_DU_CELLS) {
    return make_unexpected(
        du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state, "Too many served cells"});
  }

  // Validate served cell configurations provided in the request.
  for (const auto& served_cell : req.gnb_du_served_cells_list) {
    auto ret = validate_cell_config_request(served_cell);
    if (not ret.has_value()) {
      return ret;
    }
  }

  // Ensure the DU config does not collide with other DUs.
  for (const auto& [du_id, du_cfg] : dus) {
    if (du_cfg.id == req.gnb_du_id) {
      return make_unexpected(
          du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state, "Duplicate DU ID"});
    }
    for (const auto& cell : du_cfg.served_cells) {
      for (const auto& new_cell : req.gnb_du_served_cells_list) {
        if (cell.cgi == new_cell.served_cell_info.nr_cgi) {
          return make_unexpected(du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state,
                                                           "Duplicate served cell CGI"});
        }
      }
    }
  }

  return {};
}

error_type<du_setup_result::rejected>
du_configuration_manager::validate_du_config_update(const du_config_update_request& req) const
{
  // TODO
  return {};
}

error_type<du_setup_result::rejected>
du_configuration_manager::validate_cell_config_request(const cu_cp_du_served_cells_item& cell_req) const
{
  auto ret = validate_cell_config(cell_req);
  if (not ret.has_value()) {
    return make_unexpected(ret.error());
  }

  // Ensure NCIs match the gNB-Id.
  if (cell_req.served_cell_info.nr_cgi.nci.gnb_id(rrc_cfg.gnb_id.bit_length) != rrc_cfg.gnb_id) {
    return make_unexpected(
        du_setup_result::rejected{cause_protocol_t::msg_not_compatible_with_receiver_state,
                                  fmt::format("NCI {:#x} of the served Cell does not match gNB-Id {:#x}",
                                              cell_req.served_cell_info.nr_cgi.nci,
                                              rrc_cfg.gnb_id.id)});
  }

  return {};
}
