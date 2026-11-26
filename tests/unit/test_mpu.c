/**
 * @file test_mpu.c
 * @brief MPU shadow-config integrity and region-validation tests (host).
 *
 * Traceability: SSR-004 (MPU partition enforcement), SSR-005 (integrity).
 * Uses a simulated register file through safety_mpu_ops_t.
 */
#include "unit_test.h"
#include "arch/mpu.h"

/* ---- simulated MPU register file -------------------------------------- */

typedef struct
{
    uint32_t type;
    uint32_t ctrl;
    uint32_t base[SAFETY_MPU_REGION_COUNT];
    uint32_t rasr[SAFETY_MPU_REGION_COUNT];
    uint32_t barrier_calls;
} sim_mpu_t;

static uint32_t sim_read_type(void *ctx)
{
    return ((sim_mpu_t *)ctx)->type;
}

static void sim_write_region(void *ctx, uint32_t rnr, uint32_t base,
                             uint32_t rasr)
{
    sim_mpu_t *sim = (sim_mpu_t *)ctx;
    if (rnr < SAFETY_MPU_REGION_COUNT)
    {
        sim->base[rnr] = base;
        sim->rasr[rnr] = rasr;
    }
}

static void sim_read_region(void *ctx, uint32_t rnr, uint32_t *base,
                            uint32_t *rasr)
{
    sim_mpu_t *sim = (sim_mpu_t *)ctx;
    if (rnr < SAFETY_MPU_REGION_COUNT)
    {
        *base = sim->base[rnr];
        *rasr = sim->rasr[rnr];
    }
}

static void sim_write_ctrl(void *ctx, uint32_t ctrl)
{
    ((sim_mpu_t *)ctx)->ctrl = ctrl;
}

static void ignore_ctrl_write(void *ctx, uint32_t ctrl)
{
    (void)ctx;
    (void)ctrl;
}

static uint32_t sim_read_ctrl(void *ctx)
{
    return ((sim_mpu_t *)ctx)->ctrl;
}

static void sim_barrier(void *ctx)
{
    ((sim_mpu_t *)ctx)->barrier_calls++;
}

/* Build an example 8-region table with valid ARMv7-M encodings
 * (STM32F4-class example addresses; real values come from the linker
 * script — see ADR-002). */
static void build_regions(safety_mpu_region_cfg_t r[SAFETY_MPU_REGION_COUNT])
{
    /* 0: deny-all background */
    UT_CHECK(safety_mpu_background_region(&r[0]) == SAFETY_OK);
    /* 1: ASIL-D code, flash, 64 KB, priv RO+X */
    UT_CHECK(safety_mpu_region_make(
                 0x08000000U, 0x10000U,
                 (SAFETY_MPU_AP_PRIV_RO << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_C,
                 &r[1]) == SAFETY_OK);
    /* 2: ASIL-D data+stack, SRAM, 16 KB, priv RW, XN */
    UT_CHECK(safety_mpu_region_make(
                 0x20000000U, 0x4000U,
                 (SAFETY_MPU_AP_PRIV_RW << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_XN | SAFETY_MPU_RASR_C |
                     SAFETY_MPU_RASR_S,
                 &r[2]) == SAFETY_OK);
    /* 3: QM code, flash, 256 KB, RO+X for all */
    UT_CHECK(safety_mpu_region_make(
                 0x08040000U, 0x40000U,
                 (SAFETY_MPU_AP_RO << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_C,
                 &r[3]) == SAFETY_OK);
    /* 4: QM data, SRAM, 16 KB, RW for all, XN */
    UT_CHECK(safety_mpu_region_make(
                 0x20008000U, 0x4000U,
                 (SAFETY_MPU_AP_FULL_ACCESS << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_XN | SAFETY_MPU_RASR_C |
                     SAFETY_MPU_RASR_S,
                 &r[4]) == SAFETY_OK);
    /* 5: mailbox, 1 KB, RW for all, XN */
    UT_CHECK(safety_mpu_region_make(
                 0x2000C000U, 0x400U,
                 (SAFETY_MPU_AP_FULL_ACCESS << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_XN | SAFETY_MPU_RASR_S,
                 &r[5]) == SAFETY_OK);
    /* 6: stack guard, 32 B, no access */
    UT_CHECK(safety_mpu_region_make(
                 0x20004000U, 0x20U,
                 (SAFETY_MPU_AP_NO_ACCESS << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_XN,
                 &r[6]) == SAFETY_OK);
    /* 7: peripherals, 512 MB, RW, XN */
    UT_CHECK(safety_mpu_region_make(
                 0x40000000U, 0x20000000U,
                 (SAFETY_MPU_AP_FULL_ACCESS << SAFETY_MPU_RASR_AP_SHIFT) |
                     SAFETY_MPU_RASR_XN | SAFETY_MPU_RASR_B |
                     SAFETY_MPU_RASR_S,
                 &r[7]) == SAFETY_OK);
}

int main(void)
{
    sim_mpu_t sim;
    safety_mpu_ops_t ops;
    safety_mpu_region_cfg_t regions[SAFETY_MPU_REGION_COUNT];
    safety_mpu_shadow_t shadow;
    const uint32_t ctrl = SAFETY_MPU_CTRL_ENABLE;

    ops.read_type = sim_read_type;
    ops.write_region = sim_write_region;
    ops.read_region = sim_read_region;
    ops.write_ctrl = sim_write_ctrl;
    ops.read_ctrl = sim_read_ctrl;
    ops.barrier = sim_barrier;
    ops.ctx = &sim;

    /* --- size field / region encoding helpers ---------------------------- */
    {
        uint32_t f = 0U;
        UT_CHECK(safety_mpu_size_field(32U, &f) == SAFETY_OK);
        UT_CHECK_EQ_U32(f, 4U);   /* 2^(4+1) = 32 */
        UT_CHECK(safety_mpu_size_field(0x100000U, &f) == SAFETY_OK);
        UT_CHECK_EQ_U32(f, 19U);  /* 1 MB */
        UT_CHECK(safety_mpu_size_field(16U, &f) == SAFETY_ERR_PARAM);
        UT_CHECK(safety_mpu_size_field(48U, &f) == SAFETY_ERR_PARAM);
        UT_CHECK(safety_mpu_size_field(0U, &f) == SAFETY_ERR_PARAM);
    }

    /* Misaligned base rejected (power-of-2 natural alignment). */
    {
        safety_mpu_region_cfg_t cfg;
        UT_CHECK(safety_mpu_region_make(0x20001000U, 0x4000U, 0U, &cfg) ==
                 SAFETY_ERR_RANGE);
        UT_CHECK(safety_mpu_region_make(0x20004000U, 0x4000U, 0U, &cfg) ==
                 SAFETY_OK);
    }

    /* --- shadow build + self-verify --------------------------------------- */
    build_regions(regions);
    UT_CHECK(safety_mpu_shadow_build(&shadow, regions, ctrl) == SAFETY_OK);
    UT_CHECK(safety_mpu_shadow_verify(&shadow) == SAFETY_OK);

    /* Shadow corruption is detected by its CRC. */
    shadow.region[2].rasr ^= (1UL << 24);
    UT_CHECK(safety_mpu_shadow_verify(&shadow) == SAFETY_ERR_INTEGRITY);
    shadow.region[2].rasr ^= (1UL << 24);
    UT_CHECK(safety_mpu_shadow_verify(&shadow) == SAFETY_OK);

    /* Invalid region table rejected at build time. */
    {
        safety_mpu_shadow_t bad_shadow;
        safety_mpu_region_cfg_t bad[SAFETY_MPU_REGION_COUNT];
        uint32_t i;
        for (i = 0U; i < SAFETY_MPU_REGION_COUNT; i++)
        {
            bad[i] = regions[i];
        }
        bad[3].base = 0x08041000U; /* not aligned to 256 KB */
        UT_CHECK(safety_mpu_shadow_build(&bad_shadow, bad, ctrl) ==
                 SAFETY_ERR_RANGE);
    }

    /* --- init programs the simulated registers ---------------------------- */
    sim.type = (8U << 8);  /* MPU_TYPE.DREGION = 8 */
    sim.ctrl = 0U;
    sim.barrier_calls = 0U;
    UT_CHECK(safety_mpu_init(&ops, &shadow) == SAFETY_OK);
    UT_CHECK_EQ_U32(sim.ctrl, ctrl);
    UT_CHECK_EQ_U32(sim.base[1], 0x08000000U);
    UT_CHECK(sim.barrier_calls > 0U);

    /* Hardware without 8 regions is rejected. */
    {
        sim_mpu_t sim0 = sim;
        sim0.type = 0U; /* no MPU */
        safety_mpu_ops_t ops0 = ops;
        ops0.ctx = &sim0;
        UT_CHECK(safety_mpu_init(&ops0, &shadow) == SAFETY_ERR_STATE);
    }

    {
        safety_mpu_ops_t stuck = ops;
        stuck.write_ctrl = ignore_ctrl_write;
        sim.ctrl = 0U;
        UT_CHECK(safety_mpu_init(&stuck, &shadow) == SAFETY_ERR_INTEGRITY);
        sim.type = 16U << 8;
        UT_CHECK(safety_mpu_init(&ops, &shadow) == SAFETY_ERR_STATE);
        sim.type = 8U << 8;
        UT_CHECK(safety_mpu_init(&ops, &shadow) == SAFETY_OK);
        UT_CHECK(safety_mpu_shadow_build(&shadow, regions, 0U) == SAFETY_ERR_PARAM);
    }

    /* Corrupted shadow is refused for programming. */
    {
        safety_mpu_shadow_t bad_shadow = shadow;
        bad_shadow.ctrl ^= 1U;
        UT_CHECK(safety_mpu_init(&ops, &bad_shadow) == SAFETY_ERR_INTEGRITY);
    }

    /* --- integrity check: clean pass --------------------------------------- */
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) == SAFETY_OK);

    /* --- integrity check: region tampering detected and repaired ---------- */
    sim.rasr[2] |= (SAFETY_MPU_AP_FULL_ACCESS << SAFETY_MPU_RASR_AP_SHIFT);
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) ==
             SAFETY_ERR_INTEGRITY);
    /* repaired: */
    UT_CHECK_EQ_U32(sim.rasr[2], shadow.region[2].rasr);
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) == SAFETY_OK);

    /* --- integrity check: CTRL tampering (e.g. MPU disabled) -------------- */
    sim.ctrl = 0U;
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) ==
             SAFETY_ERR_INTEGRITY);
    UT_CHECK_EQ_U32(sim.ctrl, ctrl);
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) == SAFETY_OK);

    /* --- integrity check: base address tampering --------------------------- */
    sim.base[5] = 0x2000E000U;
    UT_CHECK(safety_mpu_integrity_check(&ops, &shadow) ==
             SAFETY_ERR_INTEGRITY);
    UT_CHECK_EQ_U32(sim.base[5], shadow.region[5].base);

    /* --- parameter validation ---------------------------------------------- */
    UT_CHECK(safety_mpu_init(NULL, &shadow) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mpu_init(&ops, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mpu_integrity_check(NULL, &shadow) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mpu_integrity_check(&ops, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mpu_shadow_verify(NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mpu_background_region(NULL) == SAFETY_ERR_PARAM);

    return ut_summary("mpu");
}
