#ifndef REGULATOR_H
#define REGULATOR_H

#include "hardware.h"
#include <inttypes.h>
#include <stdbool.h>

struct regulator;
typedef struct regulator const __memx * regptr;

// General regulator class
struct regulator {
    // Initialize the driver including any MCU hardware.
    // @return true on error
    bool (*probe)(regptr reg);

    // Enable the regulator. If already enabled, switch synchronization
    // mode.
    // @param sync - iff true and the regulator supports it, run
    //  synchronous to a local timer. If the regulator does not
    //  support synchronization, ignore this.
    // @return true on error
    bool (*enable)(regptr reg, bool sync);

    // Disable the regulator.
    // @return true on error
    bool (*disable)(regptr reg);

    // Return whether the regulator is enabled.
    bool (*is_enabled)(regptr reg);

    // Return whether power is good. This should return *false* if the
    // regulator is disabled.
    bool (*is_power_good)(regptr reg);
};

#define reg_probe(reg)          ((reg)->probe((reg)))
#define reg_enable(reg, sync)   ((reg)->enable((reg), (sync)))
#define reg_disable(reg)        ((reg)->disable((reg)))
#define reg_is_enabled(reg)     ((reg)->is_enabled((reg)))
#define reg_is_power_good(reg)  ((reg)->is_power_good((reg)))

struct reg_buck {
    struct regulator base;
    PORT_t volatile * sync_port;
    PORT_t volatile * pg_port;
    uint8_t sync_bm;
    uint8_t pg_bm;
    uint8_t sync_cc_bm;
    bool    phase_180;
};

struct reg_inv {
    struct regulator base;
    PORT_t volatile * en_port;
    PORT_t volatile * pg_port;
    uint8_t en_bm;
    uint8_t pg_bm;
};

#define reg_type        struct regulator const __memx
#define reg_buck_type   struct reg_buck const __memx
#define reg_inv_type    struct reg_inv const __memx

#define reg__buck(reg)  ((struct reg_buck const __memx *) (reg))
#define reg__inv(reg)   ((struct reg_inv const __memx *) (reg))

extern reg_type * reg_P5A;
extern reg_type * reg_P5B;
extern reg_type * reg_P3A;
extern reg_type * reg_P3B;
extern reg_type * reg_N12;

#endif // REGULATOR_H
