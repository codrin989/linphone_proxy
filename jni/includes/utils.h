/*
 * utils.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_PACKET_SIZE 1450

/* error printing macro */
#define ERR(call_description) \
	do { \
		fprintf(stderr, "(%s, %s, %d): ", \
				__FILE__, __FUNCTION__, __LINE__); \
		perror(call_description); \
	} while (0)

/* print error (call ERR) and exit */
#define DIE(assertion, call_description) \
	do { \
		if (assertion) { \
			ERR(call_description); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#define KICK(assertion, call_description) \
	do { \
		if (assertion) { \
			fprintf(stderr, "(%s, %s, %d): ", \
					__FILE__, __FUNCTION__, __LINE__); \
			fprintf(stderr, "%s\n", call_description); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#define eprintf(...) \
	do { \
		fprintf(stdout, ##__VA_ARGS__); \
		fflush(stdout); \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */
