/**
 * @file mpu.c
 * @brief ARMv7-M MPU configuration, shadow CRC, and integrity checking.
 *
 * Pure logic (region validation, shadow build/verify, integrity compare) is
 * host-compilable for unit testing; only safety_mpu_hw_ops() touches real
 * registers and is compiled for ARM targets exclusively.
 *
 * @safety_req SSR-004, SSR-005
 */
#include "arch/mpu.h"
#include "safety/crc32.h"

/* RBAR address field mask (ADDR[31:5]; VALID and REGION bits excluded). */
#define MPU_RBAR_ADDR_MASK  (0xFFFFFFE0UL)

/* MPU_TYPE.DREGION field. */
#define MPU_TYPE_DREGION_SHIFT  (8U)
#define MPU_TYPE_DREGION_MASK   (0xFFUL)

/* ------------------------------------------------------------------ */
/* Shadow CRC helper                                                    */
/* ------------------------------------------------------------------ */

static uint32_t shadow_compute_crc(const safety_mpu_shadow_t *shadow)
{
    uint32_t crc = SAFETY_CRC32_INIT;
    uint32_t i;

    for (i = 0U; i < SAFETY_MPU_REGION_COUNT; i++)
    {
        uint8_t word[8];
        uint32_t b = shadow->region[i].base;
        uint32_t r = shadow->region[i].rasr;

        /* Serialize field-by-field (endian-stable, padding-independent). */
        word[0] = (uint8_t)(b & 0xFFU);
        word[1] = (uint8_t)((b >> 8U) & 0xFFU);
        word[2] = (uint8_t)((b >> 16U) & 0xFFU);
        word[3] = (uint8_t)((b >> 24U) & 0xFFU);
        word[4] = (uint8_t)(r & 0xFFU);
        word[5] = (uint8_t)((r >> 8U) & 0xFFU);
        word[6] = (uint8_t)((r >> 16U) & 0xFFU);
        word[7] = (uint8_t)((r >> 24U) & 0xFFU);
        crc = safety_crc32_update(crc, word, sizeof(word));
    }

    {
        uint8_t cw[4];
        cw[0] = (uint8_t)(shadow->ctrl & 0xFFU);
        cw[1] = (uint8_t)((shadow->ctrl >> 8U) & 0xFFU);
        cw[2] = (uint8_t)((shadow->ctrl >> 16U) & 0xFFU);
        cw[3] = (uint8_t)((shadow->ctrl >> 24U) & 0xFFU);
        crc = safety_crc32_update(crc, cw, sizeof(cw));
    }

    return safety_crc32_final(crc);
}

/* Validate one region cfg against ARMv7-M encoding rules. */
static safety_status_t region_validate(const safety_mpu_region_cfg_t *cfg)
{
    safety_status_t status = SAFETY_OK;

    if ((cfg->rasr & SAFETY_MPU_RASR_ENABLE) != 0U)
    {
        uint32_t size_field =
            (cfg->rasr >> SAFETY_MPU_RASR_SIZE_SHIFT) & 0x1FU;

        if (size_field < 4U)
        {
            /* Regions below 32 bytes are architecturally invalid. */
            status = SAFETY_ERR_RANGE;
        }
        else
        {
            /* Natural alignment: base must be aligned to region size.
             * Use 64-bit arithmetic so the 4 GB encoding (field 31) works. */
            uint64_t size = (uint64_t)1U << (size_field + 1U);
            if (((uint64_t)cfg->base & (size - 1U)) != 0U)
            {
                status = SAFETY_ERR_RANGE;
            }
            else if ((cfg->base & ~MPU_RBAR_ADDR_MASK) != 0U)
            {
                status = SAFETY_ERR_RANGE;
            }
            else
            {
                status = SAFETY_OK;
            }
        }
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

safety_status_t safety_mpu_size_field(uint32_t size_bytes,
                                      uint32_t *size_field)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((size_field != NULL) && (size_bytes >= 32U) &&
        ((size_bytes & (size_bytes - 1U)) == 0U))
    {
        uint32_t field = 0U;
        uint32_t s = size_bytes;

        while (s > 1U)
        {
            s >>= 1U;
            field++;
        }
        *size_field = field - 1U;  /* 2^(SIZE+1) bytes */
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_mpu_region_make(uint32_t base, uint32_t size_bytes,
                                       uint32_t attr,
                                       safety_mpu_region_cfg_t *out)
{
    safety_status_t status = SAFETY_ERR_PARAM;
    uint32_t size_field;

    if (out != NULL)
    {
        status = safety_mpu_size_field(size_bytes, &size_field);
        if (status == SAFETY_OK)
        {
            if ((base & (size_bytes - 1U)) != 0U)
            {
                status = SAFETY_ERR_RANGE;  /* not naturally aligned */
            }
            else
            {
                out->base = base;
                out->rasr = attr |
                            (size_field << SAFETY_MPU_RASR_SIZE_SHIFT) |
                            SAFETY_MPU_RASR_ENABLE;
                status = SAFETY_OK;
            }
        }
    }

    return status;
}

safety_status_t safety_mpu_background_region(safety_mpu_region_cfg_t *out)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if (out != NULL)
    {
        /* Region 0: base 0, 4 GB, no access for anyone, execute-never.
         * Deny-all background per SSR-004 (PRIVDEFENA not used). */
        out->base = 0U;
        out->rasr = (SAFETY_MPU_AP_NO_ACCESS << SAFETY_MPU_RASR_AP_SHIFT) |
                    SAFETY_MPU_RASR_XN |
                    (SAFETY_MPU_RASR_SIZE_4GB << SAFETY_MPU_RASR_SIZE_SHIFT) |
                    SAFETY_MPU_RASR_ENABLE;
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_mpu_shadow_build(
    safety_mpu_shadow_t *shadow,
    const safety_mpu_region_cfg_t regions[SAFETY_MPU_REGION_COUNT],
    uint32_t ctrl)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((shadow != NULL) && (regions != NULL))
    {
        uint32_t i;

        status = SAFETY_OK;
        for (i = 0U; (i < SAFETY_MPU_REGION_COUNT) && (status == SAFETY_OK);
             i++)
        {
            status = region_validate(&regions[i]);
        }

        if (status == SAFETY_OK)
        {
            for (i = 0U; i < SAFETY_MPU_REGION_COUNT; i++)
            {
                shadow->region[i] = regions[i];
            }
            shadow->ctrl = ctrl;
            shadow->crc = shadow_compute_crc(shadow);
        }
    }

    return status;
}

safety_status_t safety_mpu_shadow_verify(const safety_mpu_shadow_t *shadow)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if (shadow != NULL)
    {
        status = (shadow_compute_crc(shadow) == shadow->crc)
                     ? SAFETY_OK
                     : SAFETY_ERR_INTEGRITY;
    }

    return status;
}

/* Program all regions and CTRL from the shadow (no CRC verification —
 * callers verify first). */
static void mpu_program(const safety_mpu_ops_t *ops,
                        const safety_mpu_shadow_t *shadow)
{
    uint32_t i;

    /* Disable while reprogramming. */
    ops->write_ctrl(ops->ctx, 0U);

    for (i = 0U; i < SAFETY_MPU_REGION_COUNT; i++)
    {
        ops->write_region(ops->ctx, i, shadow->region[i].base,
                          shadow->region[i].rasr);
    }

    ops->write_ctrl(ops->ctx, shadow->ctrl);

    if (ops->barrier != NULL)
    {
        ops->barrier(ops->ctx);
    }
}

safety_status_t safety_mpu_init(const safety_mpu_ops_t *ops,
                                const safety_mpu_shadow_t *shadow)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((ops != NULL) && (shadow != NULL) && (ops->read_type != NULL) &&
        (ops->write_region != NULL) && (ops->write_ctrl != NULL))
    {
        status = safety_mpu_shadow_verify(shadow);

        if (status == SAFETY_OK)
        {
            uint32_t dregion = (ops->read_type(ops->ctx) >>
                                MPU_TYPE_DREGION_SHIFT) &
                               MPU_TYPE_DREGION_MASK;

            if (dregion < SAFETY_MPU_REGION_COUNT)
            {
                status = SAFETY_ERR_STATE;  /* hardware lacks MPU regions */
            }
            else
            {
                mpu_program(ops, shadow);
                status = SAFETY_OK;
            }
        }
    }

    return status;
}

safety_status_t safety_mpu_integrity_check(const safety_mpu_ops_t *ops,
                                           const safety_mpu_shadow_t *shadow)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((ops != NULL) && (shadow != NULL) && (ops->read_region != NULL) &&
        (ops->read_ctrl != NULL) && (ops->write_region != NULL) &&
        (ops->write_ctrl != NULL))
    {
        status = safety_mpu_shadow_verify(shadow);

        if (status == SAFETY_OK)
        {
            bool match = true;
            uint32_t i;

            for (i = 0U; i < SAFETY_MPU_REGION_COUNT; i++)
            {
                uint32_t base = 0U;
                uint32_t rasr = 0U;

                ops->read_region(ops->ctx, i, &base, &rasr);
                if (((base & MPU_RBAR_ADDR_MASK) !=
                     (shadow->region[i].base & MPU_RBAR_ADDR_MASK)) ||
                    (rasr != shadow->region[i].rasr))
                {
                    match = false;
                }
            }

            if (ops->read_ctrl(ops->ctx) != shadow->ctrl)
            {
                match = false;
            }

            if (!match)
            {
                /* Detected interference with the live MPU configuration:
                 * restore immediately, then report so the caller raises a
                 * safe-state event (ADR-005 §D1d). */
                mpu_program(ops, shadow);
                status = SAFETY_ERR_INTEGRITY;
            }
        }
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* Target-only hardware back-end (ARMv7-M registers at 0xE000ED90)     */
/* ------------------------------------------------------------------ */
#if defined(__arm__)

typedef struct
{
    volatile uint32_t TYPE;  /* 0xE000ED90 */
    volatile uint32_t CTRL;  /* 0xE000ED94 */
    volatile uint32_t RNR;   /* 0xE000ED98 */
    volatile uint32_t RBAR;  /* 0xE000ED9C */
    volatile uint32_t RASR;  /* 0xE000EDA0 */
} mpu_regs_t;

#define MPU_REGS  ((mpu_regs_t *)0xE000ED90UL)

static uint32_t hw_read_type(void *ctx)
{
    (void)ctx;
    return MPU_REGS->TYPE;
}

static void hw_write_region(void *ctx, uint32_t rnr, uint32_t base,
                            uint32_t rasr)
{
    (void)ctx;
    MPU_REGS->RNR = rnr;
    MPU_REGS->RBAR = base & MPU_RBAR_ADDR_MASK;
    MPU_REGS->RASR = rasr;
}

static void hw_read_region(void *ctx, uint32_t rnr, uint32_t *base,
                           uint32_t *rasr)
{
    (void)ctx;
    MPU_REGS->RNR = rnr;
    *base = MPU_REGS->RBAR & MPU_RBAR_ADDR_MASK;
    *rasr = MPU_REGS->RASR;
}

static void hw_write_ctrl(void *ctx, uint32_t ctrl)
{
    (void)ctx;
    MPU_REGS->CTRL = ctrl;
}

static uint32_t hw_read_ctrl(void *ctx)
{
    (void)ctx;
    return MPU_REGS->CTRL;
}

static void hw_barrier(void *ctx)
{
    (void)ctx;
    __asm volatile("dsb 0xF" ::: "memory");
    __asm volatile("isb 0xF" ::: "memory");
}

static const safety_mpu_ops_t mpu_hw_ops = {
    hw_read_type, hw_write_region, hw_read_region,
    hw_write_ctrl, hw_read_ctrl, hw_barrier, NULL
};

const safety_mpu_ops_t *safety_mpu_hw_ops(void)
{
    return &mpu_hw_ops;
}

#endif /* __arm__ */
