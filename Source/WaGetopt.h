/** Command line option parser

WaGetopt.c - a simple implementation of the getopt shipped with IRIX.
I've added the ':' return to indicate a missing option argument.
This may not be exactly like IRIX.

All legal option characters are passed via optstr. If an option
requires an argument, the character is followed by a ':' in optstr.
getopt() returns the option character parsed, if it requires an
argument, the global optarg will point the the argument. getopt()
returns -1 when all options parsed, in which case remaining arguments
can be found at argv[optind]. getopt returns '?' if parsing an unknown
option, and ':' if an option is missing its argument.

Unlike IRIX version, this getopt() checks if an option argument is
itself an option. The call getopt(argc, argv, "a:b") parsing "-a -b"
will return the ':' error, because "-a" expects an argument. The IRIX
version would return "-b" as argument to "-a" option. This version
also checks for unary negation, so "-a -5 -b" would be parsed correctly,
i.e., -5 is the argument to "-a".

@file WaGetopt.h
*/
#ifndef _WA_GETOPT_H
#define _WA_GETOPT_H

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Header for WaGetopt.c
 */
int WaGetopt(int argc,  char **argv,  char *optstr);
void WaGetoptReset();

extern char *optarg;
extern int optind, opterr, optopt;

#ifdef  __cplusplus
}
#endif

#endif
