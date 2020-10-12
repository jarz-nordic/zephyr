/*$$$LICENCE_NORDIC_STANDARD<2020>$$$*/

#include <nrf_gpms.h>
#include <string.h>


#include <logging/log.h>
LOG_MODULE_REGISTER(GPMS, 3);

static uint8_t depth = 0;

uint8_t nrf_gpms_cons_mode_num(const nrf_gpms_consumer_t * cons)
{
    uint8_t i = 0;
    while((cons->modes[i].name != NULL) && (i < NRF_GPMS_MAX_MODES))
    {
        i++;
    }
    return i;
}

uint8_t nrf_gpms_cons_req_num(const nrf_gpms_cons_mode_t * mode)
{
    uint8_t i = 0;
    while(((mode->requirements[i].max_volt > 0) || (mode->requirements[i].max_volt > 0)) &&
          (i < NRF_GPMS_MAX_REQUIREMENTS))
    {
        i++;
    }
    return i;
}

uint8_t nrf_gpms_reg_out_num(const nrf_gpms_regulator_t * reg)
{
    uint8_t i = 0;
    while((reg->outs[i].name != NULL) && (i < NRF_GPMS_MAX_OUTS))
    {
        i++;
    }
    return i;
}

uint8_t nrf_gpms_node_parents_num(nrf_gpms_cb_t *         cb,
                                  const nrf_gpms_node_t * node)
{
    __ASSERT_NO_MSG(cb->stat[node->id][0] == node);
    uint8_t i = 1; /* Own instance is at offset 0 */
    while(cb->stat[node->id][i] != NULL)
    {
        i++;
    }
    return i-1;
}

static inline void nrf_gpms_cons_print(const nrf_gpms_consumer_t * cons)
{
    LOG_INF("\t[>] Modes:");
    for(uint8_t i = 0; i < nrf_gpms_cons_mode_num(cons); i++)
    {
        const nrf_gpms_cons_mode_t * cur_mode = nrf_gpms_cons_mode_get(cons, i);
        LOG_INF("\t\t[*] %s", cur_mode->name);

        for(uint8_t i = 0; i < nrf_gpms_cons_req_num(cur_mode); i++)
        {
            const nrf_gpms_requirement_t * cur_req = nrf_gpms_cons_req_get(cur_mode, i);
            LOG_INF("\t\t\t Min: %d Max: %d Clean %d",
                          cur_req->min_volt, cur_req->max_volt, cur_req->clean);
        }
    }
}

static inline void nrf_gpms_reg_print(const nrf_gpms_regulator_t * reg)
{
	LOG_INF("\t[>] Clean capable: %d", reg->clean);
	LOG_INF("\t[>] Outputs:");
    for(uint8_t i = 0; i < nrf_gpms_reg_out_num(reg); i++)
    {
        const nrf_gpms_reg_out_t * cur_out = nrf_gpms_reg_out_get(reg, i);
        LOG_INF("\t\t[*] %s", cur_out->name);
        LOG_INF("\t\t\t Min: %d Max: %d", cur_out->min_volt, cur_out->max_volt);
        LOG_INF("\t\t\t Efficiency: %d%%", cur_out->efficiency);
        LOG_INF("\t\t\t Rail ID: %d", cur_out->rail_id);
    }
}

void nrf_gpms_node_print(nrf_gpms_cb_t * cb,
                         const nrf_gpms_node_t * node)
{
    LOG_INF("[!] %s [%d]", node->name, node->id);

    LOG_INF("\t[>] Parents:");
    for(uint8_t i = 0; i < nrf_gpms_node_parents_num(cb, node); i++)
    {
        const nrf_gpms_node_t * parent =
            nrf_gpms_node_parent_get(cb, node, i);
        LOG_INF("\t\t[*] %s", parent->name);
    }

    if(nrf_gpms_node_parents_num(cb, node) == 0)
    {
        LOG_INF("\t\tCORE");
    }

    switch(node->type)
    {
        case NRF_GPMS_DEVICE_NONE:
            LOG_INF("\t[>] Type: NONE");
        break;
        case NRF_GPMS_DEVICE_REGULATOR:
            LOG_INF("\t[>] Type: regulator");
            nrf_gpms_reg_print(nrf_gpms_node_reg_get(node));
        break;
        case NRF_GPMS_DEVICE_RAIL:
            LOG_INF("\t[>] Type: rail");
            //TODO
        break;
        case NRF_GPMS_DEVICE_CONSUMER:
            LOG_INF("\t[>] Type: consumer");
            nrf_gpms_cons_print(nrf_gpms_node_cons_get(node));
        break;
        default:
            __ASSERT_NO_MSG(false);
        break;
    }
}

uint8_t nrf_gpms_dyn_rail_enf_req_num(nrf_gpms_dyn_rail_t * rail)
{
    uint8_t i = 0;
    while((rail->enforced_reqs[i].node != NULL) && (i < NRF_GPMS_MAX_ENFORCED))
    {
        i++;
    }
    return i;
}

//TODO: Enforcing
// - should signal return a linked list of actions or should it be done here?
// - include times in that linked list and sum them to see if its worth it
uint8_t nrf_gpms_req_traverse(nrf_gpms_cb_t *                cb,
                              const nrf_gpms_node_t *        node_req,
                              const nrf_gpms_node_t *        node_consumer,
                              const nrf_gpms_requirement_t * requirement,
                              bool                           apply)
{
    depth++;
    /* Check if this is core node */
    if(nrf_gpms_node_parents_num(cb, node_req) == 0)
    {
        /* This should be a regulator, return its best fit */
        if(node_req->type != NRF_GPMS_DEVICE_REGULATOR)
        {
            LOG_ERR("[!] Core node %s is not regulator", node_req->name);
            depth--;
            return 0;
        }
        else
        {
            const nrf_gpms_regulator_t * regulator = nrf_gpms_node_reg_get(node_req);

            nrf_gpms_dyn_reg_t * dyn_reg = nrf_gpms_dyn_reg_get(cb, node_req);
            if(!dyn_reg->enabled)
            {
                LOG_INF("[.] Core node %s disabled", node_req->name);
                depth--;
                return 0;
            }

            /* Find best output that fits requirements */
            const nrf_gpms_reg_out_t * best = NULL;

            for(uint8_t i = 0; i < nrf_gpms_reg_out_num(regulator); i++)
            {
                const nrf_gpms_reg_out_t * out = nrf_gpms_reg_out_get(regulator, i);


                /* Fits requirements? */
                if(!((requirement->min_volt > out->max_volt) || (requirement->max_volt < out->min_volt))
                   && (node_consumer->id == out->rail_id)
                   && (!requirement->clean || (requirement->clean == regulator->clean)))
                {
                    /* Better fit? */
                    if((best == NULL) || (out->efficiency > best->efficiency))
                    {
                        best = out;
                    }
                }
            }

            if(best == NULL)
            {
                LOG_INF("[.] No fitting output in core node %s", node_req->name);
                depth--;
                return 0;
            }

            LOG_INF("[*] Reached core node: %s, output %s with efficiency %d",
                          node_req->name, best->name, best->efficiency);
            depth--;
            return best->efficiency;
        }
    }
    else
    {
        if(node_req->type == NRF_GPMS_DEVICE_RAIL)
        {
            nrf_gpms_dyn_rail_t * dyn_rail = nrf_gpms_dyn_rail_get(cb, node_req);
            /* Check if requirement is compatible with other requirements on rail */
            bool compatible = true;

            for(uint8_t i = 0; i < nrf_gpms_dyn_rail_enf_req_num(dyn_rail); i++)
            {
                const nrf_gpms_requirement_t * enf_req = dyn_rail->enforced_reqs[i].req;

                if((enf_req->min_volt > requirement->max_volt) ||
                   (enf_req->max_volt < requirement->min_volt))
                {
                    compatible = false;
                }
            }

            if(compatible)
            {
                LOG_DBG("[*] Found compatible rail %s", node_req->name);
            }
            else
            {
                LOG_DBG("[.] Rail %s has incompatible requirements", node_req->name);
                depth--;
                return 0;
            }

            uint8_t max_eff = 0;
            /* Find all parent regulators */
            for(uint8_t i = 0; i < nrf_gpms_node_parents_num(cb, node_req); i++)
            {
                /* Forward the requirement to them */
                const nrf_gpms_node_t * up = nrf_gpms_node_parent_get(cb, node_req, i);
                uint8_t this_eff = nrf_gpms_req_traverse(cb, up, node_req, requirement, apply);

                LOG_DBG("[*] Received efficiency %d from %s", this_eff, up->name);

                if(this_eff > max_eff)
                {
                    max_eff = this_eff;
                }
            }

            /* Return best value */
            LOG_DBG("[*] Node %s returning max_efficiency: %d", node_req->name, max_eff);

            depth--;
            return max_eff;
        }
        else if(node_req->type == NRF_GPMS_DEVICE_REGULATOR)
        {

            const nrf_gpms_regulator_t * regulator = nrf_gpms_node_reg_get(node_req);

            /* Find best output that fits requirements */
            const nrf_gpms_reg_out_t * best = NULL;
            uint32_t max_eff = 0;
            LOG_DBG("[>] Entering %s intermediate", node_req->name);

            for(uint8_t i = 0; i < nrf_gpms_reg_out_num(regulator); i++)
            {
                const nrf_gpms_reg_out_t * out = nrf_gpms_reg_out_get(regulator, i);

                /* Fits requirements? */
                if(!((requirement->min_volt > out->max_volt) || (requirement->max_volt < out->min_volt))
                   && (node_consumer->id == out->rail_id)
                   && (!requirement->clean || (requirement->clean == regulator->clean)))
                {
                    /* Regulator should have only one parent */
                    if(nrf_gpms_node_parents_num(cb, node_req) > 1)
                    {
                        LOG_ERR("[!] Regulator %s has more than one parent", node_req->name);
                        depth--;
                        return 0;
                    }

                    /* Change requirement to own and get efficiency from parent rail */
                    nrf_gpms_requirement_t own_req;

                    if(out->bypass)
                    {
                        own_req.min_volt = requirement->min_volt;
                        own_req.max_volt = requirement->max_volt;

                        /* If this is bypass output and clean is required,
                         * parent should provide clean too */
                        own_req.clean = (requirement->clean ? true : false);
                    }
                    else
                    {
                        own_req.min_volt = requirement->min_volt + out->drop;
                        own_req.max_volt = requirement->max_volt + out->drop;
                        own_req.clean    = false;
                    }

                    const nrf_gpms_node_t * parent_rail = nrf_gpms_node_parent_get(cb, node_req, 0);

                    uint8_t own_eff = nrf_gpms_req_traverse(cb, parent_rail, node_req, &own_req, apply);

                    if((best == NULL) || ((out->efficiency*own_eff) > best->efficiency))
                    {
                        best = out;
                        LOG_INF("[*] Found intermediate output with eff %d, up_eff %d",
                                       out->efficiency, own_eff);
                        max_eff = (out->efficiency * own_eff)/100;
                    }
                }
            }

            if(best == NULL)
            {
                LOG_INF("[.] No fitting output in node %s", node_req->name);
                depth--;
                return 0;
            }

            LOG_DBG("[<] Leaving regulator %s, output %s returning %d",
                         node_req->name, best->name, max_eff);

            depth--;
            return max_eff;
        }
        else
        {
            LOG_ERR("[!] Invalid node type on %s, should be RAIL/REGULATOR", node_req->name);
            depth--;
            return 0;
        }
    }
}


void nrf_gpms_consumer_mode_set(nrf_gpms_cb_t *         cb,
                                const nrf_gpms_node_t * node,
                                uint8_t mode)
{
    __ASSERT_NO_MSG(mode < NRF_GPMS_MAX_MODES);
    depth++;
    nrf_gpms_dyn_cons_t * dyn_cons = nrf_gpms_dyn_cons_get(cb, node);
    const nrf_gpms_consumer_t * stat_cons = nrf_gpms_node_cons_get(node);

    if(stat_cons->modes[mode].name == NULL)
    {
        LOG_WRN("[!] Trying to set invalid mode %d to %s", mode, node->name);
        depth--;
        return;
    }

    LOG_INF("[*] Setting %s to mode %s", node->name, stat_cons->modes[mode].name);

    /* Clear all modes */
    for(uint8_t i = 0; i < nrf_gpms_cons_mode_num(stat_cons); i++)
    {
        dyn_cons->modes_state[i] = 0;
    }

    /* Set new mode */
    dyn_cons->modes_state[mode] = 1;

    uint8_t efficiency[NRF_GPMS_MAX_REQUIREMENTS];

    /* Call parent rail to check for new efficiency for each requirement */
    for(uint8_t i = 0; i < nrf_gpms_cons_req_num(&stat_cons->modes[mode]); i++)
    {
        const nrf_gpms_requirement_t * rq = nrf_gpms_cons_req_get(&stat_cons->modes[mode], i);
        const nrf_gpms_node_t        * rail = nrf_gpms_id_node_get(cb, rq->rail_id);

        LOG_DBG("[>] Looking for <%d, %d> (%s) on %s from %s",
                     rq->min_volt, rq->max_volt, rq->clean?"clean":"dirty", rail->name, node->name);

        efficiency[i] = nrf_gpms_req_traverse(cb, rail, node, rq, false);

        LOG_DBG("[<] Efficiency[%d] = %d", i, efficiency[i]);
    }


    bool found = true; 
    /* Fatal error if none can guarantee the requirement */
    for(uint8_t i = 0; i < nrf_gpms_cons_req_num(&stat_cons->modes[mode]); i++)
    {
        if(efficiency[i] == 0)
        {
            const nrf_gpms_requirement_t * rq   = nrf_gpms_cons_req_get(&stat_cons->modes[mode], i);
            const nrf_gpms_node_t        * rail = nrf_gpms_id_node_get(cb, rq->rail_id);

            LOG_ERR("[!] Unable to guarantee requirement <%d, %d> (%s) on %s from %s",
                         rq->min_volt, rq->max_volt, rq->clean?"clean":"dirty", rail->name, node->name);
            found = false;
        }
    }

    if(!found)
    {
        /* Return to safe mode in future */
        depth--;
        return;
    }

    /* Enforce the new settings */
    for(uint8_t i = 0; i < nrf_gpms_cons_req_num(&stat_cons->modes[mode]); i++)
    {
        //const nrf_gpms_requirement_t * rq = nrf_gpms_cons_req_get(&stat_cons->modes[mode], i);
        //TODO: enforce, rework whole thing
        //nrf_gpms_req_traverse(cb, nrf_gpms_id_node_get(cb, rq->rail_id), node, rq, true);
    }
    depth--;
}

void nrf_gpms_initialize(nrf_gpms_cb_t * cb)
{
    /* Create runtime structures */
    uint8_t rgs = 0, cos = 0, rls = 0;

    for(uint8_t i = 0; i < NRF_GPMS_MAX_NODES; i++)
    {
        if(cb->stat[i][0] == NULL)
        {
            LOG_DBG("[.] Skipping empty ID %d", i);
            continue;
        }

        switch(cb->stat[i][0]->type)
        {
            case NRF_GPMS_DEVICE_REGULATOR:
                LOG_DBG("[*] Creating dynamic regulator with ID %d", i);
                cb->dyn[i] = &cb->reg[rgs];
                memset(&cb->reg[rgs], 0, sizeof(nrf_gpms_dyn_reg_t));
                if((i == 0) || (i == 1) || (i == 4) || (i == 6) || (i == 9) || (i == 23))//TODO: move out, customize
                {
                    /* Enable non external core regulators -- TODO: only cores or all could be
                     * enabled?  */
                    cb->reg[rgs].enabled = true;
                }
                //TODO: regulator mode set 0
                //nrf_gpms_consumer_mode_set(cb, cb->stat[i][0], 0);
                rgs++;
            break;
            case NRF_GPMS_DEVICE_RAIL:
                LOG_DBG("[*] Creating dynamic rail with ID %d", i);
                cb->dyn[i] = &cb->rail[rls];
                memset(&cb->rail[rls], 0, sizeof(nrf_gpms_dyn_rail_t));
                rls++;
            break;
            case NRF_GPMS_DEVICE_CONSUMER:
                LOG_DBG("[*] Creating dynamic consumer with ID %d", i);
                cb->dyn[i] = &cb->cons[cos];
                memset(&cb->cons[cos], 0, sizeof(nrf_gpms_dyn_cons_t));
                nrf_gpms_consumer_mode_set(cb, cb->stat[i][0], 0);//TODO move to separate after init
                cos++;
            break;
            default:
                LOG_WRN("[!] Invalid node type! ID: %d", i);
            break;
        }
    }

}
