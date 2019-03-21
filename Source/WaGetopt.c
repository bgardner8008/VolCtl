/*
 * WaGetopt.c - a simple implementation of the getopt shipped with IRIX.
 * I've added the ':' return to indicate a missing option argument.
 * This may not be exactly like IRIX.
 *
 * All legal option characters are passed via optstr. If an option
 * requires an argument, the character is followed by a ':' in optstr.
 * getopt() returns the option character parsed, if it requires an
 * argument, the global optarg will point the the argument. getopt()
 * returns -1 when all options parsed, in which case remaining arguments
 * can be found at argv[optind]. getopt returns '?' if parsing an unknown
 * option, and ':' if an option is missing its argument.
 *
 * Unlike IRIX version, this getopt() checks if an option argument is
 * itself an option. The call getopt(argc, argv, "a:b") parsing "-a -b"
 * will return the ':' error, because "-a" expects an argument. The IRIX
 * version would return "-b" as argument to "-a" option. This version
 * also checks for unary negation, so "-a -5 -b" would be parsed correctly,
 * i.e., -5 is the argument to "-a".
 *
 * Bill Gardner, May, 1998.
 */
#include <string.h>
#include <ctype.h>
#include "WaGetopt.h"

#define START_INDEX 1	/* skip over program name */

char *optarg;
int optind = START_INDEX;
int opterr;	/* not used */
int optopt;

/*
 * Call until WaGetopt() returns -1. Returns the option character parsed.
 */
int WaGetopt(int argc,  char **argv,  char *optstr)
{
	char *arg;
	char *s;
	if (optind < argc) {
		arg = argv[optind++];
		if (arg[0] == '-') {
			/* this argument is an option */
			optopt = arg[1];
			if (s = strchr(optstr, optopt)) {
				/* OK, option found */
				if (*(s+1) == ':') {
					/* option requires argument */
					if (optind < argc) {
						/* OK, point optarg to next argument */
						optarg = argv[optind++];
						if (*optarg == '-' && !isdigit(*(optarg+1))) {
							/* this argument is another option! */
							return ':';
						}
						else {
							/* OK, legal argument */
							return optopt;
						}
					}
					else {
						/* no argument for option */
						return ':';
					}
				}
				else {
					/* OK, option doesn't require argument */
					return optopt;
				}
			}
			else {
				/* unknown option */
				return '?';
			}
		}
		else {
			/* argument is not an option, done */
			optind--;
			return -1;
		}
	}
	/* no more arguments, done */
	return -1;
}

void WaGetoptReset()
{
	optind = START_INDEX;
	optopt = 0;
	optarg = NULL;
}
