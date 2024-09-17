/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../slicing/ran_slice_candidate.h"
#include "ue_allocator.h"

namespace srsran {

/// This struct provides a view of the current resource grid state to the PDSCH and PUSCH allocators.
class ue_resource_grid_view
{
public:
  void add_cell(const cell_resource_allocator& cell_grid)
  {
    cell_res_grids.emplace(cell_grid.cell_index(), &cell_grid);
  }

  slot_point get_pdcch_slot(du_cell_index_t cell_index) const { return cell_res_grids[cell_index]->slot_tx(); }

  slot_point get_pusch_slot(du_cell_index_t cell_index, unsigned k2) const
  {
    return (*cell_res_grids[cell_index])[k2].slot;
  }

  const cell_configuration& get_cell_cfg_common(du_cell_index_t cell_index) const
  {
    return cell_res_grids[cell_index]->cfg;
  }

  span<const pdcch_dl_information> get_dl_pdcch_sched_results(du_cell_index_t cell_index) const
  {
    return (*cell_res_grids[cell_index])[0].result.dl.dl_pdcchs;
  }

  const cell_resource_allocator& get_grid(du_cell_index_t cell_index) const { return *cell_res_grids[cell_index]; }

  const cell_slot_resource_grid& get_pdcch_grid(du_cell_index_t cell_index) const
  {
    return (*cell_res_grids[cell_index])[0].dl_res_grid;
  }

  const cell_slot_resource_grid& get_pdsch_grid(du_cell_index_t cell_index, unsigned k0) const
  {
    return (*cell_res_grids[cell_index])[k0].dl_res_grid;
  }

  const cell_slot_resource_grid& get_pusch_grid(du_cell_index_t cell_index, unsigned k2) const
  {
    return (*cell_res_grids[cell_index])[k2].ul_res_grid;
  }

  span<const dl_msg_alloc> get_ue_pdsch_grants(du_cell_index_t cell_index, unsigned k0) const
  {
    return (*cell_res_grids[cell_index])[k0].result.dl.ue_grants;
  }

  bool has_ue_dl_pdcch(du_cell_index_t cell_index, rnti_t rnti) const
  {
    const auto& pdcchs = (*cell_res_grids[cell_index])[0].result.dl.dl_pdcchs;
    return std::any_of(
        pdcchs.begin(), pdcchs.end(), [rnti](const pdcch_dl_information& pdcch) { return pdcch.ctx.rnti == rnti; });
  }

  bool has_ue_ul_pdcch(du_cell_index_t cell_index, rnti_t rnti) const
  {
    const auto& pdcchs = (*cell_res_grids[cell_index])[0].result.dl.ul_pdcchs;
    return std::any_of(
        pdcchs.begin(), pdcchs.end(), [rnti](const pdcch_ul_information& pdcch) { return pdcch.ctx.rnti == rnti; });
  }

  bool has_ue_dl_grant(du_cell_index_t cell_index, rnti_t rnti, unsigned k0) const
  {
    const auto& grants = (*cell_res_grids[cell_index])[k0].result.dl.ue_grants;
    return std::any_of(
        grants.begin(), grants.end(), [rnti](const auto& grant) { return grant.pdsch_cfg.rnti == rnti; });
  }

  bool has_ue_ul_grant(du_cell_index_t cell_index, rnti_t rnti, unsigned k2) const
  {
    const auto& puschs = (*cell_res_grids[cell_index])[k2].result.ul.puschs;
    return std::any_of(
        puschs.begin(), puschs.end(), [rnti](const auto& pusch) { return pusch.pusch_cfg.rnti == rnti; });
  }

  unsigned nof_cells() const { return cell_res_grids.size(); }

private:
  slotted_array<const cell_resource_allocator*, MAX_NOF_DU_CELLS> cell_res_grids;
};

/// Interface of data scheduler that is used to allocate UE DL and UL grants in a given slot
/// The data_scheduler object will be common to all cells and slots.
class scheduler_policy
{
public:
  virtual ~scheduler_policy() = default;

  /// Schedule UE DL grants for a given slot and one or more cells.
  /// \param[out] pdsch_alloc PDSCH grant allocator. This object provides a handle to allocate PDSCH grants in the
  ///                            gNB resource grid.
  /// \param[in] res_grid View of the current resource grid occupancy state for all gnb cells.
  /// \param[in] slice_candidate Slice candidate to be scheduled in the given slot.
  /// \param[in] harq_pending_retx_list List of DL HARQs pending retransmission.
  virtual void dl_sched(ue_pdsch_allocator&          pdsch_alloc,
                        const ue_resource_grid_view& res_grid,
                        dl_ran_slice_candidate&      slice_candidate,
                        dl_harq_pending_retx_list    harq_pending_retx_list) = 0;

  /// Schedule UE UL grants for a given {slot, cell}.
  /// \param[out] pusch_alloc PUSCH grant allocator. This object provides a handle to allocate PUSCH grants in the
  ///                            gNB resource grid.
  /// \param[in] res_grid View of the current resource grid occupancy state for all gnb cells.
  /// \param[in] slice_candidate Slice candidate to be scheduled in the given slot.
  /// \param[in] harq_pending_retx_list List of UL HARQs pending retransmission.
  virtual void ul_sched(ue_pusch_allocator&          pusch_alloc,
                        const ue_resource_grid_view& res_grid,
                        ul_ran_slice_candidate&      slice_candidate,
                        ul_harq_pending_retx_list    harq_pending_retx_list) = 0;
};

} // namespace srsran
