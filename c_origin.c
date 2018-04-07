#include "c_origin.h"

#ifdef __sun
#define _STRUCTURED_PROC 1
#include <sys/fcntl.h>
#include <procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#else
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bsd/string.h>
#include <unistd.h>
#endif /* __sun aka Solaris */
#include <stdlib.h>
#include <sys/param.h>
#include <stdio.h>

static char *
get_origin_impl(int up, const char *tail, const char *fallback, int retry)
{
#if defined(__linux) || defined(__sun)
  char my_origin[MAXPATHLEN] = { '\0' };
  int olen = 0;
#ifdef __sun
  /* get the name from initial argv[0] */
  psinfo_t ps;
  int fd;
  char **argv;

  if ( (fd = open("/proc/self/psinfo", O_RDONLY)) > -1 ) {
    const char *execname;
    int res = read( fd, &ps, sizeof(psinfo_t) );
    if (res > 0) {
      argv = (char **) ps.pr_argv;
      /* always try argv[0] first */
      if (realpath(argv[0], my_origin) && (my_origin[0] == '/')) {
        olen = strlen(my_origin);
      } else if ((execname = getexecname())){
        /* no absolute name - so probably invoked like
           "eval 'exec perl -S $0 ${1+"$@"}'"    or
           "#!/bin/env perl"
         */
        realpath(execname, my_origin);
        olen = strlen(my_origin);
      }
    }
    close(fd);
  }
#else /* assume this is __linux */
  olen = readlink("/proc/self/exe", my_origin, sizeof(my_origin));
  if ( olen < 1 || my_origin[0] == '[') {
    olen = 0;
  }
#endif /* __linux */
  if (olen > 0) {
    if (my_origin[0] != '/' && retry != 0) {
      /* Process probably did a chdir() => can't resolve relative stuff.
		 So we lookup the PWD value on prog start, chdir() back to it and
		 try to resolve from there. */
      FILE *fp = fopen("/proc/self/environ", "r");
      if (fp != NULL) {
        size_t lsz;
        char *l = NULL;
        while (getdelim(&l, &lsz, 0, fp) != -1) {
          if ((strlen(l) > 4) && l[3] == '=' && l[2] == 'D' && l[1] == 'W'
              && l[0] == 'P')
          {
            char cwd[MAXPATHLEN] = { '\0' };
            char *alt;
            getcwd(cwd, sizeof(cwd));
            if ((getcwd(cwd, sizeof(cwd)) != NULL) && chdir(l+4) == 0) {
              if ((alt = get_origin_impl(0, "dummy", NULL, 0)) != NULL) {
                olen = strlen(alt);
				if (olen > 0)
                  strlcpy(my_origin, alt, sizeof(my_origin));
                free(alt);
              }
              chdir(cwd);
            }
          }
        }
        free(l);
        fclose(fp);
      }
    }
    /* assert(my_origin[0] == '/'); - nodeJS+icu would break here on tests */
	if (my_origin[0] == '/') {
      /* remove everything after the last / incl. the / itself */
      /* +1 for the basename */
      if (up <= 0) {
        up = 1;
      } else {
        up++;
      }
      for (;up != 0 && olen > 1; up--) {
        while (olen > 1 && my_origin[olen - 1] != '/')
          --olen;
        my_origin[--olen] = '\0';
      }
      if (tail != NULL) {
        if (olen > 1)
          strlcat(my_origin, "/", MAXPATHLEN);
        strlcat(my_origin, tail, MAXPATHLEN);
      }
      return strdup(my_origin);
	}
  }
#endif
/* end  __linux || __sun */
  return fallback != NULL ? strdup(fallback) : NULL;
}

/**
 * get_origin_rel:
 *
 * Get the ORIGIN aka path to this running program, cut off the given number
 * of directories starting from the end and finally appends the given tail.
 * No checks are made, whether the thus constructed path actually exists!
 *
 * @param up   .. number of directories to cut off. Values &lt; 1 are treated
 *                as 0, i.e. just the basename of this application gets removed.
 *				  If path consists of a "/" only, cutting off stops no matter,
 *                whether <var>up</var> sub pathes have been cut off or not. 
 * @param tail .. path to append, after the required parts have been cut off.
 *                If %NULL or <var>up</var> == 0 or exec path cannot be
 *				  determined, nothing gets append. Otherwise, a '/' (if the
 *				  resulting path is != '/') and the tail gets append.
 * @param fallback	if the origin cannot be determined, return a duplicate
 *					of this parameter (see strdup);
 *
 * E.g. if started via /opt/apps/bin/myapp it would return
 * /opt/apps/bin for get_origin_rel(0, NULL) or /opt/apps/lib for
 * get_origin_rel(1, "lib").
 *
 * Return value: a newly-allocated string that must be freed with free(),
 *   which is a copy of %fallback if the path cannot be determined.
 */
char *
get_origin_rel(int up, const char *tail, const char *fallback)
{
	return get_origin_impl(up, tail, fallback, 1);
}

/**
 * get_origin:
 *
 * A convinience function for get_origin_rel(0, NULL, NULL).
 *
 * Return value: a newly-allocated string that must be freed with free() or
 *   %NULL if unable to determine.
 */
char *
get_origin()
{
	return get_origin_impl(0, NULL, NULL, 1);
}

/* Preparation:
 *   mkdir -p ${TESTDIR}
 *   mkdir -p ${BINARY%/ *}		# remove the space before the '*' - gcc fuck
 *   cc -m64 -g  -DORIGIN_TEST_MAIN c_origin.c               OR on Linux
 *	 	gcc -m64 -g -DORIGIN_TEST_MAIN c_origin.c -lbsd
 *   mv a.out ${BINARY}
 *   ln -sf ${BINARY}
 *
 * Run:
 *   ./a.out
 */
#ifdef ORIGIN_TEST_MAIN

#define ORIGIN_TESTDIR "test/addons/01_callbacks/"
#define ORIGIN_BINARY "out/Release/a.out"

int
main(int argc, char **argv) {
	char *adir;
	char rp[MAXPATHLEN] = { '\0' };
	char cwd[MAXPATHLEN] = { '\0' };
	char ep[MAXPATHLEN] = { '\0' };
	FILE *fp;

	if (argc > 1)
	  chdir(ORIGIN_TESTDIR); // would fail on Solaris w/o a 2nd try from $PWD
	realpath(ORIGIN_BINARY, rp);
	getcwd(cwd, sizeof(cwd));
	printf("realpath('%s') = '%s'\ncwd = '%s'\n", ORIGIN_BINARY, rp, cwd);
   	adir = get_origin_rel(1, "share/icu", "");
	printf("DATADIR = '%s'\n", adir);
	cwd[0]='\0';
	getcwd(cwd, sizeof(cwd));
	printf("cwd = '%s'\n", cwd);
}

#undef ORIGIN_TESTDIR
#undef ORIGIN_BINARY

#endif /* ORIGIN_TEST_MAIN */
