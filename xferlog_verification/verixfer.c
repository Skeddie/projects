/*
 * Author: Sked
 * Version: 20160531
 * Description: Verifies glftpd's xferlog for incorrect lines and outputs offending lines.
 * 		Output is "<linenumber>-<fieldnumber>: <line>"
 *		Fields are numbered from 1 to 20, delimited by spaces and colons.
 */

#include <stdio.h>	/* printf, fopen, fclose, getline */
#include <stdlib.h>	/* free, strtoll */
#include <sys/types.h>	/* stat */
#include <sys/stat.h>	/* stat */
#include <unistd.h>	/* stat */
#include <errno.h>	/* errno */
#include <string.h>	/* strerror, strtok, strcpy, strcmp */

#define MONS 12
static char *months[MONS] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
#define WKDAYS 7
static char *weekdays[WKDAYS] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

void errexit(const char *call, const char *file) {
	printf("Could not %s %s: %s\n", call, file, strerror(errno));
	exit(1);
}

short process_line(const char *line, size_t len) {
	char *tok, *e = NULL;
	short i = 0;
	long long num; /* not unsigned as strtoull negates a -, which we need to catch */
	char temp[len];

	/* len is buf size of *line and thus temp has enough space */
	strcpy(temp, line);

	/* day of week as string */
	tok = strtok(temp, " ");
	if (tok) {
		for (i = 0; i < WKDAYS && strcmp(tok, weekdays[i]); ++i);
		if (i == WKDAYS)
			return 1;
	} else
		return 1;

	/* month as string */
	tok = strtok(NULL, " ");
	if (tok) {
		for (i = 0; i < MONS && strcmp(tok, months[i]); ++i);
		if (i == MONS)
			return 2;
	} else
		return 2;

	/* day of month as num */
	tok = strtok(NULL, " ");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (*e || num < 0 || num > 31 || errno)
			return 3;
	} else
		return 3;

	/* hour as num */
	tok = strtok(NULL, ":");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (tok[2] != '\0' || *e || num < 0 || num > 24 || errno)
			return 4;
	} else
		return 4;

	/* minute as num */
	tok = strtok(NULL, ":");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (tok[2] != '\0' || *e || num < 0 || num > 60 || errno)
			return 5;
	} else
		return 5;

	/* second as num */
	tok = strtok(NULL, " ");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (tok[2] != '\0' || *e || num < 0 || num > 60 || errno)
			return 6;
	} else
		return 6;

	/* year as num */
	tok = strtok(NULL, " ");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (*e || num < 1980 || num > 2100 || errno)
			return 7;
	} else
		return 7;

	/* transfertime as num */
	tok = strtok(NULL, " ");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (*e || num < 0 || errno)
			return 8;
	} else
		return 8;

	/* hostname as string */
	tok = strtok(NULL, " ");
	if (!tok)
		return 9;

	/* filesize as num */
	tok = strtok(NULL, " ");
	if (tok) {
		errno = 0;
		e = NULL;
		num = strtoll(tok, &e, 10);
		if (*e || num < 0 || errno)
			return 10;
	} else
		return 10;

	/* filepath as string */
	tok = strtok(NULL, " ");
	if (!tok)
		return 11;

	/* transfermode as char */
	tok = strtok(NULL, " ");
	if (!tok || tok[1] != '\0' || (tok[0] != 'a' && tok[0] != 'b'))
		return 12;

	/* dummy 1 as char */
	tok = strtok(NULL, " ");
	if (!tok || tok[1] != '\0' || tok[0] != '_')
		return 13;

	/* transferdirection as char */
	tok = strtok(NULL, " ");
	if (!tok || tok[1] != '\0' || (tok[0] != 'i' && tok[0] != 'o'))
		return 14;

	/* dummy 2 as char */
	tok = strtok(NULL, " ");
	if (!tok || tok[1] != '\0' || tok[0] != 'r')
		return 15;

	/* username as string */
	tok = strtok(NULL, " ");
	if (!tok)
		return 16;

	/* groupname as string */
	tok = strtok(NULL, " ");
	if (!tok)
		return 17;

	/* identindicator as num */
	tok = strtok(NULL, " ");
	if (!tok || tok[1] != '\0' || (tok[0] != '0' && tok[0] != '1'))
		return 18;

	/* ident as string */
	tok = strtok(NULL, " ");
	if (!tok)
		return 19;

	/* rest as string */
	tok = strtok(NULL, " ");
	if (tok)
		return 20;

	return 0;
}

int main(int argc, char **argv) {

	struct stat buf;
	FILE *log;
	char *lineptr = NULL;
	size_t n = 0;
	unsigned long long linenum = 0;
	short res = 0;

	if (argc != 2) {
		printf("Usage: %s <xferlog>\n", argv[0]);
		exit(1);
	}

	if (stat(argv[1], &buf)) {
		errexit("stat", argv[1]);
	} else if (!(S_ISREG(buf.st_mode))) {
		printf("%s is not a regular file\n", argv[1]);
		exit(1);
	}

	if (!(log = fopen(argv[1], "r"))) {
		errexit("fopen", argv[1]);
	}

	/* n is allocated size, which includes \0 and is >= strlen+1 */
	while (getline(&lineptr, &n, log) > 0) {
		++linenum;

		if ((res = process_line(lineptr, n-1))) {
			/* no \n as *lineptr contains it */
			printf("%llu-%d: %s", linenum, res, lineptr);
		}

		if (!lineptr) {
			free(lineptr);
			lineptr = NULL;
		}
	}

	if (fclose(log)) {
		errexit("fclose", argv[1]);
	}
}
