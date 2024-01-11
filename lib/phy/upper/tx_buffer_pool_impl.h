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

#include "tx_buffer_codeblock_pool.h"
#include "tx_buffer_impl.h"
#include "srsran/phy/upper/trx_buffer_identifier.h"
#include "srsran/phy/upper/tx_buffer_pool.h"
#include "srsran/phy/upper/unique_tx_buffer.h"
#include "srsran/ran/slot_point.h"
#include "srsran/srslog/logger.h"
#include "srsran/srslog/srslog.h"
#include <atomic>
#include <vector>

namespace srsran {

/// Implements a PDSCH rate matcher buffer pool.
class tx_buffer_pool_impl : public tx_buffer_pool_controller, private tx_buffer_pool
{
private:
  /// No expiration time value.
  const slot_point null_expiration = slot_point();

  /// Set to true when the buffer pool stopped.
  std::atomic<bool> stopped = {false};
  /// Code block buffer pool.
  tx_buffer_codeblock_pool codeblock_pool;
  /// Actual buffer pool.
  std::vector<tx_buffer_impl> buffers;
  /// \brief List of buffer identifiers.
  ///
  /// Maps buffer identifiers to buffers. Each position indicates the buffer identifier assign to each of the buffers.
  /// Buffers with \c trx_buffer_identifier::invalid() identifier are not reserved.
  std::vector<trx_buffer_identifier> identifiers;
  /// Tracks expiration times.
  std::vector<slot_point> expirations;

  /// Indicates the lifetime of a buffer reservation as a number of slots.
  unsigned expire_timeout_slots;
  /// Logger.
  srslog::basic_logger& logger;

  // See interface for documentation.
  unique_tx_buffer reserve(const slot_point& slot, trx_buffer_identifier id, unsigned nof_codeblocks) override;

  // See interface for documentation.
  unique_tx_buffer reserve(const slot_point& slot, unsigned nof_codeblocks) override;

  // See interface for documentation.
  void run_slot(const slot_point& slot) override;

public:
  /// \brief Creates a generic receiver buffer pool.
  /// \param[in] config Provides the pool required parameters.
  tx_buffer_pool_impl(const tx_buffer_pool_config& config) :
    codeblock_pool(config.nof_codeblocks, config.max_codeblock_size, config.external_soft_bits),
    buffers(config.nof_buffers, tx_buffer_impl(codeblock_pool)),
    identifiers(config.nof_buffers, trx_buffer_identifier::invalid()),
    expirations(config.nof_buffers, null_expiration),
    expire_timeout_slots(config.expire_timeout_slots),
    logger(srslog::fetch_basic_logger("PHY", true))
  {
  }

  // See tx_buffer_pool_controller interface for documentation.
  ~tx_buffer_pool_impl() override
  {
    srsran_assert(std::none_of(buffers.begin(), buffers.end(), [](const auto& buffer) { return buffer.is_locked(); }),
                  "A buffer is still locked.");
  }

  // See tx_buffer_pool_controller interface for documentation.
  tx_buffer_pool& get_pool() override;

  // See tx_buffer_pool_controller interface for documentation.
  void stop() override;
};

} // namespace srsran
