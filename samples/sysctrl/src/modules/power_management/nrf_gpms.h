/*$$$LICENCE_NORDIC_STANDARD<2020>$$$*/

#ifndef NRF_GPMS_H_
#define NRF_GPMS_H_

#include <kernel.h>
#include <pmgt_defines.h>

#ifdef __cplusplus
extern "C" {
#endif

//TODO: clean -> bitmap to make it futureproof?

#define NRF_GPMS_MAX_ENFORCED       10

typedef enum
{
    NRF_GPMS_DEVICE_NONE,
    NRF_GPMS_DEVICE_REGULATOR,
    NRF_GPMS_DEVICE_RAIL,
    NRF_GPMS_DEVICE_CONSUMER,
} nrf_gpms_device_t;

typedef struct
{
    uint32_t min_volt; /*< Minimum voltage supported in mV. */
    uint32_t max_volt; /*< Maximum voltage supported in mV. */
    bool     clean;    /*< Is the voltage required to be clean. */
    uint8_t  rail_id;  /*< Node ID of rail that this requirement concerns. */
} nrf_gpms_requirement_t;

/* REGULATOR */

typedef struct
{
    const char *   name;
    const uint32_t min_volt;   /*< Minimum voltage generated in mV. */
    const uint32_t max_volt;   /*< Maximum voltage generated in mV. */
    const uint32_t drop;       /*< Voltage drop. */
    const uint8_t  efficiency; /*< Mode efficiency in %. */
    const uint8_t  rail_id;    /*< Node ID of rail connected to this mode. */
    const bool     bypass;     /*< Bypass. */
} nrf_gpms_reg_out_t;

typedef struct
{
    const nrf_gpms_reg_out_t outs[NRF_GPMS_MAX_OUTS];
    const bool     clean;      /*< Is the voltage clean. */
} nrf_gpms_regulator_t;

/* RAIL */
typedef struct
{
} nrf_gpms_rail_t;

/* CONSUMER */

typedef struct
{
    const char *                 name;
    const nrf_gpms_requirement_t requirements[NRF_GPMS_MAX_REQUIREMENTS];
} nrf_gpms_cons_mode_t;

typedef struct
{
    const nrf_gpms_cons_mode_t modes[NRF_GPMS_MAX_MODES];
} nrf_gpms_consumer_t;

/* GENERIC */
typedef union
{
    const nrf_gpms_regulator_t    reg;
    const nrf_gpms_rail_t         rail;
    const nrf_gpms_consumer_t     cons;
} nrf_gpms_data_t;

typedef struct
{
    const char *            name;
    const uint8_t           id;
    const nrf_gpms_device_t type;
    const nrf_gpms_data_t   data;
} nrf_gpms_node_t;

/* DYNAMIC */
typedef struct
{
    uint8_t modes_state[NRF_GPMS_MAX_MODES];
} nrf_gpms_dyn_cons_t;

typedef struct
{
    uint8_t modes_state[NRF_GPMS_MAX_MODES];
    bool enabled;
} nrf_gpms_dyn_reg_t;

typedef struct
{
    nrf_gpms_node_t * node;
    nrf_gpms_requirement_t * req;
} nrf_gpms_req_enf_t;

typedef struct
{
    uint32_t cur_voltage;
    nrf_gpms_req_enf_t enforced_reqs[NRF_GPMS_MAX_ENFORCED];
} nrf_gpms_dyn_rail_t;

typedef struct
{
    const nrf_gpms_node_t * (*stat)[NRF_GPMS_MAX_PARENTS+1];
    void ** dyn;
    nrf_gpms_dyn_cons_t * cons;
    nrf_gpms_dyn_reg_t  * reg;
    nrf_gpms_dyn_rail_t * rail;
} nrf_gpms_cb_t;

/* =================== */

/* REGULATOR */
#define NRF_GPMS_REGULATOR_DEFINE(num, _name, _clean, ...)    \
	const nrf_gpms_node_t _name = {                       \
	    .name           = STRINGIFY(_name),               \
	    .id             = num,                            \
	    .type           = NRF_GPMS_DEVICE_REGULATOR,      \
	    .data.reg.clean = _clean,                         \
	    .data.reg.outs  = { __VA_ARGS__ } \
	};

#define NRF_GPMS_REGULATOR_OUT_ADD(outname, min, max, eff, vdrop, pass, rail) \
        {                                                                     \
            .name       = STRINGIFY(outname),                                 \
            .min_volt   = min,                                                \
            .max_volt   = max,                                                \
            .efficiency = eff,                                                \
            .rail_id    = rail,                                               \
            .drop       = vdrop,                                              \
            .bypass = pass,                                                   \
        }


/* RAIL */
#define NRF_GPMS_RAIL_INIT(num, _name)  \
const nrf_gpms_node_t _name = {         \
    .name = STRINGIFY(_name),           \
    .id   = num,                        \
    .type = NRF_GPMS_DEVICE_RAIL,       \
};

/* CONSUMER */
#define NRF_GPMS_CONSUMER_BEGIN(num, _name) \
const nrf_gpms_node_t _name = {             \
    .name = STRINGIFY(_name),               \
    .id   = num,                            \
    .type = NRF_GPMS_DEVICE_CONSUMER,       \
    .data.cons.modes = {

#define NRF_GPMS_CONSUMER_MODE_BEGIN(modename)  \
        {                                       \
            .name = STRINGIFY(modename),        \
            .requirements = {

#define NRF_GPMS_CONSUMER_VOLT_ADD(min, max, pure, rail)    \
                {                                           \
                    .min_volt = min,                        \
                    .max_volt = max,                        \
                    .clean    = pure,                       \
                    .rail_id  = rail,                       \
                },

#define NRF_GPMS_CONSUMER_VOLT_OFF(rail) NRF_GPMS_CONSUMER_VOLT_ADD(0, 5000, false, rail)

#define NRF_GPMS_CONSUMER_MODE_END()    \
            }                           \
        },

#define NRF_GPMS_CONSUMER_END() \
    }                           \
};

/* LOOKUP */
#define NRF_GPMS_LOOKUP_BEGIN()                                                 \
const nrf_gpms_node_t * lookup[NRF_GPMS_MAX_NODES][NRF_GPMS_MAX_PARENTS+1] = {

#define NRF_GPMS_LOOKUP_END()   \
};

#define NRF_GPMS_LOOKUP_NODE_BEGIN(id, _name)   \
    [id] = { &_name,

#define NRF_GPMS_LOOKUP_NODE_END() \
    },

#define NRF_GPMS_LOOKUP_PARENT_ADD(_name)   &_name,

static inline const nrf_gpms_cons_mode_t * nrf_gpms_cons_mode_get(const nrf_gpms_consumer_t * cons,
                                                                  uint8_t id)
{
    __ASSERT_NO_MSG(id < NRF_GPMS_MAX_MODES);
    return &cons->modes[id];
}

static inline const nrf_gpms_requirement_t * nrf_gpms_cons_req_get(const nrf_gpms_cons_mode_t * mode,
                                                                   uint8_t id)
{
    __ASSERT_NO_MSG(id < NRF_GPMS_MAX_REQUIREMENTS);
    return &mode->requirements[id];
}

static inline const nrf_gpms_consumer_t * nrf_gpms_node_cons_get(const nrf_gpms_node_t * node)
{
    __ASSERT_NO_MSG(node->type == NRF_GPMS_DEVICE_CONSUMER);
    return &node->data.cons;
}

static inline const nrf_gpms_rail_t * nrf_gpms_node_rail_get(const nrf_gpms_node_t * node)
{
    __ASSERT_NO_MSG(node->type == NRF_GPMS_DEVICE_RAIL);
    return &node->data.rail;
}

static inline const nrf_gpms_regulator_t * nrf_gpms_node_reg_get(const nrf_gpms_node_t * node)
{
    __ASSERT_NO_MSG(node->type == NRF_GPMS_DEVICE_REGULATOR);
    return &node->data.reg;
}

static inline const nrf_gpms_reg_out_t * nrf_gpms_reg_out_get(const nrf_gpms_regulator_t * reg,
                                                                uint8_t id)
{
    __ASSERT_NO_MSG(id < NRF_GPMS_MAX_OUTS);
    return &reg->outs[id];
}

static inline const nrf_gpms_node_t * nrf_gpms_node_parent_get(nrf_gpms_cb_t * cb,
    const nrf_gpms_node_t * node, uint8_t id)
{
    __ASSERT_NO_MSG(id < NRF_GPMS_MAX_PARENTS+1);
    __ASSERT_NO_MSG(node->id < NRF_GPMS_MAX_NODES);
    return cb->stat[node->id][id+1];
}

static inline const nrf_gpms_node_t * nrf_gpms_id_node_get(nrf_gpms_cb_t * cb, uint8_t id)
{
    return cb->stat[id][0];
}

//TODO: container_of & cons as argument?
static inline nrf_gpms_dyn_cons_t * nrf_gpms_dyn_cons_get(nrf_gpms_cb_t *cb,
                                                          const nrf_gpms_node_t * node)
{
    return (nrf_gpms_dyn_cons_t *)cb->dyn[node->id];
}

static inline nrf_gpms_dyn_rail_t * nrf_gpms_dyn_rail_get(nrf_gpms_cb_t *cb,
                                                          const nrf_gpms_node_t * node)
{
    return (nrf_gpms_dyn_rail_t *)cb->dyn[node->id];
}

static inline nrf_gpms_dyn_reg_t * nrf_gpms_dyn_reg_get(nrf_gpms_cb_t *cb,
                                                        const nrf_gpms_node_t * node)
{
    return (nrf_gpms_dyn_reg_t *)cb->dyn[node->id];
}

uint8_t nrf_gpms_cons_mode_num(const nrf_gpms_consumer_t * cons);
uint8_t nrf_gpms_cons_req_num(const nrf_gpms_cons_mode_t * mode);

uint8_t nrf_gpms_reg_mode_num(const nrf_gpms_regulator_t * reg);
uint8_t nrf_gpms_dyn_rail_enf_req_num(nrf_gpms_dyn_rail_t * rail);

/* LOOKUP */
uint8_t nrf_gpms_node_parents_num(nrf_gpms_cb_t *         cb,
                                  const nrf_gpms_node_t * node);

void nrf_gpms_node_print(nrf_gpms_cb_t *         cb,
                         const nrf_gpms_node_t * node);

/* RUNTIME */

void nrf_gpms_initialize(nrf_gpms_cb_t * cb);

void nrf_gpms_consumer_mode_set(nrf_gpms_cb_t *         cb,
                                const nrf_gpms_node_t * node,
                                uint8_t                 mode);

#ifdef __cplusplus
}
#endif

#endif /* NRF_GPMS_H_ */
