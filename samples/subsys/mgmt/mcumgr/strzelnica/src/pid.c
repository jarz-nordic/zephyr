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
#include "config.h"

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
static int32_t k_i;
static int32_t k_p;

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
	const uint32_t *ptr;

	m_sum_error = 0;

	ptr = config_param_get(CONFIG_PARAM_PID_KP);
	k_p = *ptr;
	ptr = config_param_get(CONFIG_PARAM_PID_KI);
	k_i = *ptr;
}

//=============================================================================
// FUNCTION: PID_Controller
//=============================================================================
uint16_t PID_Controller(pid_data_t *pid)
{
	int32_t error;
	int32_t term_p;
	int32_t regulator;
	int32_t integrator;

	if (NULL == pid)
	{
		return 0;
	}

	error = (int32_t)((int32_t)pid->set_value - (int32_t)pid->measured_value);
	term_p = k_p * error;
	m_sum_error += error;

	LOG_INF("set = %d | measured = %d | sum_error = %d",
		 pid->set_value, pid->measured_value, m_sum_error);

	integrator = m_sum_error/k_i;
	regulator = (term_p + integrator);

	if(regulator > pid->max_val)
	{
		regulator = pid->max_val;
	}
	else if(regulator < pid->min_val)
	{
		regulator = pid->min_val;
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
	shell_print(shell, "K_P = %d | K_I = %d", k_p, k_i);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pid,
	SHELL_CMD(kp, NULL, "Update K_P factor", cmd_pid_kp),
	SHELL_CMD(ki, NULL, "Update K_I factor.", cmd_pid_ki),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(pid, &sub_pid, "Print PID parameters", cmd_pid);
//=============================================================================
// Date         ver     subm        Content
//=============================================================================
//-----------------------------------------------------------------------------
// 17-01-22     01      JRZ         Basic functionality
//-----------------------------------------------------------------------------
