#include "regulator.h"

static bool reg_buck_probe(regptr reg);
static bool reg_buck_enable(regptr reg, bool sync);
static bool reg_buck_disable(regptr reg);
static bool reg_buck_is_enabled(regptr reg);
static bool reg_buck_is_power_good(regptr reg);

static bool reg_inv_probe(regptr reg);
static bool reg_inv_enable(regptr reg, bool sync);
static bool reg_inv_disable(regptr reg);
static bool reg_inv_is_enabled(regptr reg);
static bool reg_inv_is_power_good(regptr reg);

#define def_buck(name) \
    static reg_buck_type CONCAT(CONCAT(reg_, name), _) = { \
        .base = { \
            reg_buck_probe, \
            reg_buck_enable, \
            reg_buck_disable, \
            reg_buck_is_enabled, \
            reg_buck_is_power_good }, \
        .sync_port = &CONCAT(name, _SYNC_PORT), \
        .pg_port   = &CONCAT(name, _PG_PORT), \
        .sync_bm   = bm(CONCAT(name, _SYNC_bp)), \
        .pg_bm     = bm(CONCAT(name, _PG_bp)), \
        .sync_cc_bm = bm(CONCAT(name, _SYNC_CC_bp)), \
        .phase_180 = CONCAT(name, _PHASE_180), \
    }; \
    reg_type * CONCAT(reg_, name) = &CONCAT(CONCAT(reg_, name), _).base;

def_buck(P5A)
def_buck(P5B)
def_buck(P3A)
def_buck(P3B)

#define def_inv(name) \
    static reg_inv_type CONCAT(CONCAT(reg_, name), _) = { \
        .base = { \
            reg_inv_probe, \
            reg_inv_enable, \
            reg_inv_disable, \
            reg_inv_is_enabled, \
            reg_inv_is_power_good }, \
        .en_port = &CONCAT(name, _EN_PORT), \
        .pg_port = &CONCAT(name, _PG_PORT), \
        .en_bm   = bm(CONCAT(name, _EN_bp)), \
        .pg_bm   = bm(CONCAT(name, _PG_bp)), \
    }; \
    reg_type * CONCAT(reg_, name) = &CONCAT(CONCAT(reg_, name), _).base;

def_inv(N12)

static bool reg_buck_probe(regptr reg)
{
    (void) reg;
    return false;
}

static bool reg_buck_enable(regptr reg, bool sync)
{
    if (sync) {
        PORTCFG.MPCMASK = reg__buck(reg)->sync_bm;
        reg__buck(reg)->sync_port->PIN0CTRL =
            PORT_OPC_TOTEM_gc |
            (reg__buck(reg)->phase_180 ? PORT_INVEN_bm : 0);
        DCDC_TIMER.CTRLE |= reg__buck(reg)->sync_cc_bm;
    } else {
        reg__buck(reg)->sync_port->OUTSET = reg__buck(reg)->sync_bm;
        DCDC_TIMER.CTRLE &= ~(reg__buck(reg)->sync_cc_bm);
    }
    return false;
}

static bool reg_buck_disable(regptr reg)
{
    DCDC_TIMER.CTRLE &= ~(reg__buck(reg)->sync_cc_bm);
    PORTCFG.MPCMASK = reg__buck(reg)->sync_bm;
    reg__buck(reg)->sync_port->PIN0CTRL = PORT_OPC_TOTEM_gc;
    reg__buck(reg)->sync_port->OUTCLR = reg__buck(reg)->sync_bm;
    return false;
}

static bool reg_buck_is_enabled(regptr reg)
{
    return
        (DCDC_TIMER.CTRLE & reg__buck(reg)->sync_cc_bm) ||
        (reg__buck(reg)->sync_port->OUT & reg__buck(reg)->sync_bm);
}

static bool reg_buck_is_power_good(regptr reg)
{
    return
        (reg__buck(reg)->pg_port->IN & reg__buck(reg)->pg_bm) &&
        reg_buck_is_enabled(reg);
}

static bool reg_inv_probe(regptr reg)
{
    (void) reg;
    return false;
}

static bool reg_inv_enable(regptr reg, bool sync)
{
    (void) sync;
    reg__inv(reg)->en_port->OUTSET = reg__inv(reg)->en_bm;
    return false;
}

static bool reg_inv_disable(regptr reg)
{
    reg__inv(reg)->en_port->OUTCLR = reg__inv(reg)->en_bm;
    return false;
}

static bool reg_inv_is_enabled(regptr reg)
{
    return reg__inv(reg)->en_port->OUT & reg__inv(reg)->en_bm;
}

static bool reg_inv_is_power_good(regptr reg)
{
    return
        (reg__inv(reg)->pg_port->IN & reg__inv(reg)->pg_bm) &&
        reg_is_power_good(reg_P3B) &&   // Dependency for clock to drive DC restorer
        (reg_is_power_good(reg_P5A) ||  // Dependency for power
         reg_is_power_good(reg_P5B));   // Dependency for power
}
