/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SHELL_PRIV_H
#define SHELL_PRIV_H

#include <acrn/spinlock.h>

#define SHELL_CMD_MAX_LEN		100U
#define SHELL_STRING_MAX_LEN		(PAGE_SIZE << 2U)

extern int16_t console_vmid;

/* Shell Command Function */
typedef int32_t (*shell_cmd_fn_t)(int32_t argc, char **argv);

/* Shell Command */
struct shell_cmd {
	char *str;		/* Command string */
	char *cmd_param;	/* Command parameter string */
	char *help_str;		/* Help text associated with the command */
	shell_cmd_fn_t fcn;	/* Command call-back function */

};

/* Shell Control Block */
struct shell {
	char input_line[2][SHELL_CMD_MAX_LEN + 1U];	/* current & last */
	uint32_t input_line_len;	/* Length of current input line */
	uint32_t input_line_active;	/* Active input line index */
	struct shell_cmd *cmds;	/* cmds supported */
	uint32_t cmd_count;		/* Count of cmds supported */
};

/* Shell Command list with parameters and help description */
#define SHELL_CMD_HELP			"help"
#define SHELL_CMD_HELP_PARAM		NULL
#define SHELL_CMD_HELP_HELP		"Display information about supported hypervisor shell commands"

#endif /* SHELL_PRIV_H */
