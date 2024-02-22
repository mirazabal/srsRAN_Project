/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/srsvec/dot_prod.h"
#include "simd.h"

using namespace srsran;
using namespace srsvec;

cf_t srsran::srsvec::dot_prod(span<const cf_t> x, span<const cf_t> y)
{
  srsran_srsvec_assert_size(x, y);

  cf_t     result = 0;
  unsigned i      = 0;
  unsigned len    = x.size();

#if SRSRAN_SIMD_CF_SIZE
  if (len >= SRSRAN_SIMD_CF_SIZE) {
    simd_cf_t simd_result = srsran_simd_cf_zero();
    for (unsigned simd_end = SRSRAN_SIMD_CF_SIZE * (len / SRSRAN_SIMD_CF_SIZE); i != simd_end;
         i += SRSRAN_SIMD_CF_SIZE) {
      simd_cf_t simd_x = srsran_simd_cfi_loadu(x.data() + i);
      simd_cf_t simd_y = srsran_simd_cfi_loadu(y.data() + i);

      simd_result = srsran_simd_cf_add(srsran_simd_cf_conjprod(simd_x, simd_y), simd_result);
    }

    alignas(SIMD_BYTE_ALIGN) std::array<cf_t, SRSRAN_SIMD_CF_SIZE> simd_vector_sum;
    srsran_simd_cfi_store(simd_vector_sum.data(), simd_result);
    result = std::accumulate(simd_vector_sum.begin(), simd_vector_sum.end(), cf_t());
  }
#endif // SRSRAN_SIMD_CF_SIZE

  for (; i != len; ++i) {
    result += x[i] * std::conj(y[i]);
  }

  return result;
}
