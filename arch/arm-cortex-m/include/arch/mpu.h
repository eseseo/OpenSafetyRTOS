/**
 * @file mpu.h
 * @brief ARMv7-M MPU configuration and runtime integrity checking.
 *
 * Register definitions are written locally against the ARMv7-M Architecture
 * Reference Manual (B3.5) — no CMSIS dependency.
 *
 * Key facts this API is designed around (see ADR-005):
 *  - ARMv7-M has NO hardware MPU lock: any privileged code can rewrite the
 *    MPU registers. MPU protection is therefore PREVENTIVE against
 *    unprivileged QM tasks (AoU-10) and DETECTIVE against privileged-kernel
 *    faults.
 *  - Detection: every safety cycle, safety_mpu_integrity_check() compares
 *    the live MPU registers against a CRC-protected shadow configuration
 *    and re-initializes the MPU on any mismatch. The detection latency is
 *    bounded by the safety cycle period and budgeted within the FTTI.
 *
 * Register access is injected via safety_mpu_ops_t so the shadow/validation
 * logic is host-testable; safety_mpu_hw_ops() provides the real register
 * back-end on ARM targets.
 *
 * @safety_req SSR-004 (MPU partition enforcement), SSR-005 (MPU integrity)
 */
#ifndef ARCH_ARM_CORTEX_M_MPU_H
#define ARCH_ARM_CORTEX_M_MPU_H

#include "safety/safety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of MPU regions used (Cortex-M4 provides exactly 8). */
#define SAFETY_MPU_REGION_COUNT  (8U)

/* ---- RASR (Region Attribute and Size Register) field encodings -------- */
#define SAFETY_MPU_RASR_ENABLE      (1UL << 0)
#define SAFETY_MPU_RASR_SIZE_SHIFT  (1U)   /* SIZE[5:1]: 2^(SIZE+1) bytes  */
#define SAFETY_MPU_RASR_SIZE_4GB    (31UL) /* SIZE field for a 4 GB region */
#define SAFETY_MPU_RASR_B           (1UL << 16)
#define SAFETY_MPU_RASR_C           (1UL << 17)
#define SAFETY_MPU_RASR_S           (1UL << 18)
#define SAFETY_MPU_RASR_AP_SHIFT    (24U)
#define SAFETY_MPU_RASR_XN          (1UL << 28)

/* AP[2:0] access permission encodings (privileged / unprivileged). */
#define SAFETY_MPU_AP_NO_ACCESS     (0UL) /* no access / no access  */
#define SAFETY_MPU_AP_PRIV_RW       (1UL) /* RW        / no access  */
#define SAFETY_MPU_AP_PRIV_RW_URO   (2UL) /* RW        / RO         */
#define SAFETY_MPU_AP_FULL_ACCESS   (3UL) /* RW        / RW         */
#define SAFETY_MPU_AP_PRIV_RO       (5UL) /* RO        / no access  */
#define SAFETY_MPU_AP_RO            (6UL) /* RO        / RO         */

/* CTRL bits. PRIVDEFENA deliberately NOT set: with a deny-all background
 * (Region 0) every access must match an explicit region (SSR-004). */
#define SAFETY_MPU_CTRL_ENABLE      (1UL << 0)
#define SAFETY_MPU_CTRL_HFNMIENA    (1UL << 1)
#define SAFETY_MPU_CTRL_PRIVDEFENA  (1UL << 2)

/** One MPU region: base address (32-byte aligned minimum) plus RASR value. */
typedef struct
{
    uint32_t base;  /**< Region base address (naturally aligned to size) */
    uint32_t rasr;  /**< RASR value including SIZE, AP, XN, ENABLE */
} safety_mpu_region_cfg_t;

/**
 * CRC-protected shadow of the intended MPU configuration.
 * Lives in ASIL-D memory; the reference for integrity checking (SSR-005).
 */
typedef struct
{
    safety_mpu_region_cfg_t region[SAFETY_MPU_REGION_COUNT];
    uint32_t ctrl;  /**< Intended MPU_CTRL value */
    uint32_t crc;   /**< CRC-32 over region[] and ctrl */
} safety_mpu_shadow_t;

/** Injected MPU register access (host-testable back-end boundary). */
typedef struct
{
    uint32_t (*read_type)(void *ctx);                     /* MPU_TYPE  */
    void (*write_region)(void *ctx, uint32_t rnr,
                         uint32_t base, uint32_t rasr);   /* RNR/RBAR/RASR */
    void (*read_region)(void *ctx, uint32_t rnr,
                        uint32_t *base, uint32_t *rasr);
    void (*write_ctrl)(void *ctx, uint32_t ctrl);         /* MPU_CTRL  */
    uint32_t (*read_ctrl)(void *ctx);
    void (*barrier)(void *ctx);                           /* DSB; ISB (may be NULL) */
    void *ctx;
} safety_mpu_ops_t;

/**
 * @brief Compose the RASR SIZE field value for a power-of-2 region size.
 * @param size_bytes Region size; must be a power of 2 and >= 32.
 * @param size_field Receives the SIZE field value (already shifted OUT of
 *                   position; caller shifts by SAFETY_MPU_RASR_SIZE_SHIFT).
 * @return SAFETY_OK, or SAFETY_ERR_PARAM for an invalid size.
 */
safety_status_t safety_mpu_size_field(uint32_t size_bytes,
                                      uint32_t *size_field);

/**
 * @brief Build and validate one region configuration.
 *
 * Enforces the ARMv7-M constraints: size is a power of 2, >= 32 bytes, and
 * the base address is naturally aligned to the size.
 *
 * @param base       Region base address.
 * @param size_bytes Region size (power of 2, >= 32).
 * @param attr       RASR attribute bits (AP, XN, C/B/S) WITHOUT size/enable.
 * @param out        Receives the validated configuration (ENABLE set).
 */
safety_status_t safety_mpu_region_make(uint32_t base, uint32_t size_bytes,
                                       uint32_t attr,
                                       safety_mpu_region_cfg_t *out);

/**
 * @brief Region 0 deny-all background: base 0, 4 GB, no access, XN.
 * Consistent with SSR-004: any access not matching an explicit region faults.
 */
safety_status_t safety_mpu_background_region(safety_mpu_region_cfg_t *out);

/**
 * @brief Build the CRC-protected shadow from a full 8-region table.
 *
 * Validates every enabled region (power-of-2 size >= 32, natural alignment)
 * and computes the shadow CRC.
 *
 * @return SAFETY_OK, SAFETY_ERR_PARAM on NULL, or SAFETY_ERR_RANGE on an
 *         invalid region encoding.
 */
safety_status_t safety_mpu_shadow_build(
    safety_mpu_shadow_t *shadow,
    const safety_mpu_region_cfg_t regions[SAFETY_MPU_REGION_COUNT],
    uint32_t ctrl);

/**
 * @brief Verify the shadow's own CRC (detects shadow corruption).
 * @return SAFETY_OK or SAFETY_ERR_INTEGRITY.
 */
safety_status_t safety_mpu_shadow_verify(const safety_mpu_shadow_t *shadow);

/**
 * @brief Program the MPU from a verified shadow and enable it.
 *
 * Order: verify shadow CRC -> check MPU_TYPE.DREGION == SAFETY_MPU_REGION_COUNT -> program all
 * regions -> write CTRL -> barrier -> verify live register values.
 *
 * @return SAFETY_OK, SAFETY_ERR_PARAM, SAFETY_ERR_INTEGRITY (shadow CRC or hardware readback),
 *         or SAFETY_ERR_STATE (hardware region count differs from the shadow layout).
 */
safety_status_t safety_mpu_init(const safety_mpu_ops_t *ops,
                                const safety_mpu_shadow_t *shadow);

/**
 * @brief Per-cycle MPU integrity check (ADR-005 §D1d, SSR-005).
 *
 * Verifies the shadow CRC, then compares every live region register and
 * MPU_CTRL against the shadow. On any mismatch the MPU is immediately
 * re-initialized from the shadow and SAFETY_ERR_INTEGRITY is returned so
 * the caller can raise a safe-state event (detected privileged-code
 * interference; detection latency bounded by the safety cycle).
 *
 * @return SAFETY_OK if the live configuration matches;
 *         SAFETY_ERR_INTEGRITY on mismatch (after re-initialization);
 *         SAFETY_ERR_PARAM on invalid arguments.
 */
safety_status_t safety_mpu_integrity_check(const safety_mpu_ops_t *ops,
                                           const safety_mpu_shadow_t *shadow);

#if defined(__arm__)
/** @brief Real ARMv7-M MPU register back-end (target builds only). */
const safety_mpu_ops_t *safety_mpu_hw_ops(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ARCH_ARM_CORTEX_M_MPU_H */
