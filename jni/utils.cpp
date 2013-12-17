/*
 * utils.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "includes/utils.h"

int
exit_error (const char * msg, int exit_code) {
	perror (msg);
	/*exit (exit_code); do not exit for now; need to delete the iptables rules maybe TODO */
	return exit_code;
}

int
return_error (const char * msg, int return_code) {
	perror (msg);
	return return_code;
}
