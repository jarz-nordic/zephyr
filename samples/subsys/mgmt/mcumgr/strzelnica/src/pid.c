//-----------------------------------------------------------------------------
//
//  Copyright 2017 Jakub Rzeszutko, All Rights Reserved
//
//  Module:         Pid.c
//  Creator:        Jakub Rzeszutko
//  email:          projekty@rzeszutko.eu
//
//-----------------------------------------------------------------------------
//  Project:        ComShooting - Transporter
//-----------------------------------------------------------------------------
//
//
//
//----------------- INCLUDE DIRECTIVES FOR OTHER HEADERS ----------------------
#include <stdlib.h>
#include <shell/shell.h>
#include "pid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(pid);

//---------------------- LOCAL DEFINES FOR CONSTANTS --------------------------

//---------------------- LOCAL DEFINE MACROS (#, ##)---------------------------

//-----------------------------------------------------------------------------
//---------------------- LOCAL TYPEDEF DECLARATIONS ---------------------------
//-----------------------------------------------------------------------------

//-------------------------------- ENUMS --------------------------------------

//------------------------------- STRUCT --------------------------------------

//------------------------------- UNIONS --------------------------------------

//-----------------------------------------------------------------------------
//-------------------------- OBJECT DEFINITIONS -------------------------------
//-----------------------------------------------------------------------------

//--------------------------- EXPORTED OBJECTS --------------------------------

//---------------------------- LOCAL OBJECTS ----------------------------------
static int32_t m_sum_error;

//-----------------------------------------------------------------------------
//----------------------- LOCAL FUNCTION PROTOTYPES ---------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//---------------------------- LOCAL FUNCTIONS --------------------------------
//-----------------------------------------------------------------------------

//=============================================================================
// FUNCTION: Fo
//-----------------------------------------------------------------------------
// DESCRIPTION:
//   <Description>
//
// PREEMPTABLE: No
// REENTRANT:   No
//
// PARAMETERS:   void
//
//
// RETURN VALUE: void
//
//=============================================================================

//-----------------------------------------------------------------------------
//--------------------------- EXPORTED FUNCTIONS ------------------------------
//-----------------------------------------------------------------------------


//=============================================================================
// FUNCTION: PID_Init
//=============================================================================
void PID_Init(void)
{
    m_sum_error = 0;
}

static int32_t k_i = K_I;
static int32_t k_p = K_P;
//=============================================================================
// FUNCTION: PID_Controller
//=============================================================================
uint16_t PID_Controller(pid_data_t * p_pid)
{
    int32_t error;
    int32_t term_p;
    int32_t regulator;
    int32_t integrator;

    if (NULL == p_pid)
    {
        return 0;
    }

    error = (int32_t)((int32_t)p_pid->set_value - (int32_t)p_pid->measured_value);
    term_p = k_p * error;
    m_sum_error += error;

	LOG_INF("set = %d | measured = %d | sum_error = %d",
		 p_pid->set_value, p_pid->measured_value, m_sum_error);

    integrator = m_sum_error/k_i;
    regulator = (term_p + integrator);

    if(regulator > p_pid->max_val)
    {
        regulator = p_pid->max_val;
    }
    else if(regulator < p_pid->min_val)
    {
        regulator = p_pid->min_val;
    }

    return regulator;
}

static int cmd_pid_kp(const struct shell *shell, size_t argc, char **argv)
{
	k_p = (int32_t)strtol(argv[1], NULL, 10);
	return 0;
}

static int cmd_pid_ki(const struct shell *shell, size_t argc, char **argv)
{
	k_i = (int32_t)strtol(argv[1], NULL, 10);
	return 0;
}

static int cmd_pid(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "kp = %d | ki = %d", k_p, k_i);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pid,
	SHELL_CMD(kp, NULL, "Hexdump params command.", cmd_pid_kp),
	SHELL_CMD(ki, NULL, "Hexdump params command.", cmd_pid_ki),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(pid, &sub_pid, NULL, cmd_pid);
//=============================================================================
// Date         ver     subm        Content
//=============================================================================
//-----------------------------------------------------------------------------
// 17-01-22     01      JRZ         Basic functionality
//-----------------------------------------------------------------------------
