/* Getopt.c ripped straight out of GNU libc with some very minor modifications
	Use this getopt not a system dependant one, as sendip is rather sensitive
	to precise getopt behaviour.
	JEL: removed unused bloat to make sure, everybody gets the same. Behaves
	     per default POSIXLY_CORRECT, i.e. stop option parsing at the first
		 non-option argument.
*/

/*
   Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 2000
   	Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "gnugetopt.h"

/* sendip does not support i10n yet. So stay consistent and do the same here */
#ifndef _
#define _(x)	x
#endif

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

char *gnuoptarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `gnuoptind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
int gnuoptind = 1;

/* Formerly, initialization of getopt depended on gnuoptind==0, which
   causes problems with re-calling getopt as programs generally don't
   know that. */

int __getopt_initialized;

/* The next char to be scanned in the option-element
   in which the last option character we returned was found.
   This allows us to pick up the scan where we left off.

   If this is zero, or a null string, it means resume the scan
   by advancing to the next ARGV-element.  */

char *nextchar = NULL;

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */

int gnuopterr = 1;

/* Set to an option character which was unrecognized.
   This must be initialized on some systems to avoid linking in the
   system's own getopt implementation.  */

int gnuoptopt = '?';

/* Describe how to deal with options that follow non-option ARGV-elements.

   If the caller did not specify anything, the default is REQUIRE_ORDER.

   REQUIRE_ORDER means don't recognize them as options;
   stop option processing when the first non-option is seen.
   This is what Unix does.
   This mode of operation is selected by using `+' as the first character
   of the list of option characters.

   PERMUTE means, we permute the contents of ARGV as we scan,
   so that eventually all the non-options are at the end.  This allows options
   to be given in any order, even with programs that were not written to
   expect this.

   RETURN_IN_ORDER is an option available to programs that were written
   to expect options and other ARGV-elements in any order and that care about
   the ordering of the two.  We describe each non-option ARGV-element
   as if it were the argument of an option with character code 1.
   Using `-' as the first character of the list of option characters
   selects this mode of operation.

   The special argument `--' forces an end of option-scanning regardless
   of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
   `--' can cause `getopt' to return -1 with `gnuoptind' != ARGC.  */

static enum
{
  REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

/* Avoid depending on library functions or files
   whose names are inconsistent.  */

static const char *
my_index (const char *str, int chr)
{
  while (*str)
    {
      if (*str == chr)
	return str;
      str++;
    }
  return 0;
}

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

/* Initialize the internal data when the first call is made.  */

static const char *
_getopt_initialize (const char *optstring)
{
  /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  first_nonopt = last_nonopt = gnuoptind;

  nextchar = NULL;

  /* Determine how to handle the ordering of options and nonoptions.  */

  if (optstring[0] == '-')
    {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    }
  else if (optstring[0] == '+')
    {
      ordering = REQUIRE_ORDER;
      ++optstring;
    }
  else
    ordering = REQUIRE_ORDER;	/* order is important for sendip */

  return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `gnuoptind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns -1.
   Then `gnuoptind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return '?' after printing an error message.  If you set `gnuopterr' to
   zero, the error message is suppressed but we still return '?'.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `gnuoptarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `gnuoptarg', otherwise `gnuoptarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   The elements of ARGV aren't really const, because we permute them.
   But we pretend they're const in the prototype to be compatible
   with other systems.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.  */

int
_getopt_internal (int argc, char *const *argv, const char *optstring,
	const struct option *longopts, int *longind, int long_only)
{
  int print_errors = gnuopterr;
  if (optstring[0] == ':')
    print_errors = 0;

  if (argc < 1)
    return -1;

  gnuoptarg = NULL;

  if (gnuoptind == 0 || !__getopt_initialized)
    {
      if (gnuoptind == 0)
	gnuoptind = 1;	/* Don't scan ARGV[0], the program name.  */
      optstring = _getopt_initialize (optstring);
      __getopt_initialized = 1;
    }

  /* Test whether ARGV[gnuoptind] points to a non-option argument.
     Either it does not have option syntax, or there is an environment flag
     from the shell indicating it is not an option.  The later information
     is only used when the used in the GNU libc.  */
# define NONOPTION_P (argv[gnuoptind][0] != '-' || argv[gnuoptind][1] == '\0')

  if (nextchar == NULL || *nextchar == '\0')
    {
      /* Advance to the next ARGV-element.  */

      /* Give FIRST_NONOPT & LAST_NONOPT rational values if GNUOPTIND has been
	 moved back by the user (who may also have changed the arguments).  */
      if (last_nonopt > gnuoptind)
	last_nonopt = gnuoptind;
      if (first_nonopt > gnuoptind)
	first_nonopt = gnuoptind;

      /* The special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (gnuoptind != argc && !strcmp (argv[gnuoptind], "--"))
	{
	  gnuoptind++;

	  if (first_nonopt != last_nonopt && last_nonopt != gnuoptind) {
	    fprintf(stderr, "Internal error, please contact the maintainer!\n"
			"Sent him the full command line with all arguments used.\n");
		first_nonopt = gnuoptind;
	  } else if (first_nonopt == last_nonopt) {
	    first_nonopt = gnuoptind;
	  }
	  last_nonopt = argc;

	  gnuoptind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (gnuoptind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (first_nonopt != last_nonopt)
	    gnuoptind = first_nonopt;
	  return -1;
	}

      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if (NONOPTION_P)
	{
	  if (ordering == REQUIRE_ORDER)
	    return -1;
	  gnuoptarg = argv[gnuoptind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Skip the initial punctuation.  */

      nextchar = (argv[gnuoptind] + 1
		  + (longopts != NULL && argv[gnuoptind][1] == '-'));
    }

  /* Decode the current option-ARGV-element.  */

  /* Check whether the ARGV-element is a long option.

     If long_only and the ARGV-element has the form "-f", where f is
     a valid short option, don't consider it an abbreviated form of
     a long option that starts with f.  Otherwise there would be no
     way to give the -f short option.

     On the other hand, if there's a long option "fubar" and
     the ARGV-element is "-fu", do consider that an abbreviation of
     the long option, just like "--fu", and not "-f" with arg "u".

     This distinction seems to be the most useful approach.  */

  if (longopts != NULL && (argv[gnuoptind][1] == '-' || (long_only
		  && (argv[gnuoptind][2] || !my_index(optstring, argv[gnuoptind][1])))))
    {
      char *nameend;
      const struct option *p;
      const struct option *pfound = NULL;
      int exact = 0;
      int ambig = 0;
      int indfound = -1;
      int option_index;

      for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
	/* Do nothing.  */ ;

      /* Test all long options for either exact match
	 or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name; p++, option_index++)
	if (!strncmp (p->name, nextchar, nameend - nextchar))
	  {
	    if ((unsigned int) (nameend - nextchar)
		== (unsigned int) strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == NULL)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = option_index;
	      }
	    else if (long_only
		     || pfound->has_arg != p->has_arg
		     || pfound->flag != p->flag
		     || pfound->val != p->val)
	      /* Second or later nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  if (print_errors)
	    fprintf (stderr, _("%s: option `%s' is ambiguous\n"),
		     argv[0], argv[gnuoptind]);
	  nextchar += strlen (nextchar);
	  gnuoptind++;
	  gnuoptopt = 0;
	  return '?';
	}

      if (pfound != NULL)
	{
	  option_index = indfound;
	  gnuoptind++;
	  if (*nameend)
	    {
	      /* Don't test has_arg with >, because some C compilers don't
		 allow it to be used on enums.  */
	      if (pfound->has_arg)
		gnuoptarg = nameend + 1;
	      else
		{
		  if (print_errors)
		    {
		      if (argv[gnuoptind - 1][1] == '-')
			/* --option */
			fprintf (stderr,
				 _("%s: option `--%s' doesn't allow an argument\n"),
				 argv[0], pfound->name);
		      else
			/* +option or -option */
			fprintf (stderr,
				 _("%s: option `%c%s' doesn't allow an argument\n"),
				 argv[0], argv[gnuoptind - 1][0], pfound->name);
		    }

		  nextchar += strlen (nextchar);

		  gnuoptopt = pfound->val;
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (gnuoptind < argc)
		gnuoptarg = argv[gnuoptind++];
	      else
		{
		  if (print_errors)
		    fprintf (stderr,
			   _("%s: option `%s' requires an argument\n"),
			   argv[0], argv[gnuoptind - 1]);
		  nextchar += strlen (nextchar);
		  gnuoptopt = pfound->val;
		  return optstring[0] == ':' ? ':' : '?';
		}
	    }
	  nextchar += strlen (nextchar);
	  if (longind != NULL)
	    *longind = option_index;
	  if (pfound->flag)
	    {
	      *(pfound->flag) = pfound->val;
	      return 0;
	    }
	  return pfound->val;
	}

      /* Can't find it as a long option.  If this is not getopt_long_only,
	 or the option starts with '--' or is not a valid short
	 option, then it's an error.
	 Otherwise interpret it as a short option.  */
      if (!long_only || argv[gnuoptind][1] == '-'
	  || my_index (optstring, *nextchar) == NULL)
	{
	  if (print_errors)
	    {
	      if (argv[gnuoptind][1] == '-')
		/* --option */
		fprintf (stderr, _("%s: unrecognized option `--%s'\n"),
			 argv[0], nextchar);
	      else
		/* +option or -option */
		fprintf (stderr, _("%s: unrecognized option `%c%s'\n"),
			 argv[0], argv[gnuoptind][0], nextchar);
	    }
	  nextchar = (char *)(unsigned long)"";
	  gnuoptind++;
	  gnuoptopt = 0;
	  return '?';
	}
    }

  /* Look at and handle the next short option-character.  */

  {
    char c = *nextchar++;
    const char *temp = my_index (optstring, c);

    /* Increment `gnuoptind' when we start to process its last character.  */
    if (*nextchar == '\0')
      ++gnuoptind;

    if (temp == NULL || c == ':')
      {
	if (print_errors)
	  {
	      fprintf (stderr, _("%s: invalid option -- %c\n"),
		       argv[0], c);
	  }
	gnuoptopt = c;
	return '?';
      }
    /* Convenience. Treat POSIX -W foo same as long option --foo */
    if (temp[0] == 'W' && temp[1] == ';')
      {
	char *nameend;
	const struct option *p;
	const struct option *pfound = NULL;
	int exact = 0;
	int ambig = 0;
	int indfound = 0;
	int option_index;

	/* This is an option that requires an argument.  */
	if (*nextchar != '\0')
	  {
	    gnuoptarg = nextchar;
	    /* If we end this ARGV-element by taking the rest as an arg,
	       we must advance to the next element now.  */
	    gnuoptind++;
	  }
	else if (gnuoptind == argc)
	  {
	    if (print_errors)
	      {
		/* 1003.2 specifies the format of this message.  */
		fprintf (stderr, _("%s: option requires an argument -- %c\n"),
			 argv[0], c);
	      }
	    gnuoptopt = c;
	    if (optstring[0] == ':')
	      c = ':';
	    else
	      c = '?';
	    return c;
	  }
	else
	  /* We already incremented `gnuoptind' once;
	     increment it again when taking next ARGV-elt as argument.  */
	  gnuoptarg = argv[gnuoptind++];

	/* gnuoptarg is now the argument, see if it's in the
	   table of longopts.  */

	for (nextchar = nameend = gnuoptarg; *nameend && *nameend != '='; nameend++)
	  /* Do nothing.  */ ;

	/* Test all long options for either exact match
	   or abbreviated matches.  */
	for (p = longopts, option_index = 0; p->name; p++, option_index++)
	  if (!strncmp (p->name, nextchar, nameend - nextchar))
	    {
	      if ((unsigned int) (nameend - nextchar) == strlen (p->name))
		{
		  /* Exact match found.  */
		  pfound = p;
		  indfound = option_index;
		  exact = 1;
		  break;
		}
	      else if (pfound == NULL)
		{
		  /* First nonexact match found.  */
		  pfound = p;
		  indfound = option_index;
		}
	      else
		/* Second or later nonexact match found.  */
		ambig = 1;
	    }
	if (ambig && !exact)
	  {
	    if (print_errors)
	      fprintf (stderr, _("%s: option `-W %s' is ambiguous\n"),
		       argv[0], argv[gnuoptind]);
	    nextchar += strlen (nextchar);
	    gnuoptind++;
	    return '?';
	  }
	if (pfound != NULL)
	  {
	    option_index = indfound;
	    if (*nameend)
	      {
		/* Don't test has_arg with >, because some C compilers don't
		   allow it to be used on enums.  */
		if (pfound->has_arg)
		  gnuoptarg = nameend + 1;
		else
		  {
		    if (print_errors)
		      fprintf (stderr, _("\
%s: option `-W %s' doesn't allow an argument\n"),
			       argv[0], pfound->name);

		    nextchar += strlen (nextchar);
		    return '?';
		  }
	      }
	    else if (pfound->has_arg == 1)
	      {
		if (gnuoptind < argc)
		  gnuoptarg = argv[gnuoptind++];
		else
		  {
		    if (print_errors)
		      fprintf (stderr,
			       _("%s: option `%s' requires an argument\n"),
			       argv[0], argv[gnuoptind - 1]);
		    nextchar += strlen (nextchar);
		    return optstring[0] == ':' ? ':' : '?';
		  }
	      }
	    nextchar += strlen (nextchar);
	    if (longind != NULL)
	      *longind = option_index;
	    if (pfound->flag)
	      {
		*(pfound->flag) = pfound->val;
		return 0;
	      }
	    return pfound->val;
	  }
	  nextchar = NULL;
	  return 'W';	/* Let the application handle it.   */
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*nextchar != '\0')
	      {
		gnuoptarg = nextchar;
		gnuoptind++;
	      }
	    else
	      gnuoptarg = NULL;
	    nextchar = NULL;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*nextchar != '\0')
	      {
		gnuoptarg = nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		gnuoptind++;
	      }
	    else if (gnuoptind == argc)
	      {
		if (print_errors)
		  {
		    /* 1003.2 specifies the format of this message.  */
		    fprintf (stderr,
			     _("%s: option requires an argument -- %c\n"),
			     argv[0], c);
		  }
		gnuoptopt = c;
		if (optstring[0] == ':')
		  c = ':';
		else
		  c = '?';
	      }
	    else
	      /* We already incremented `gnuoptind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      gnuoptarg = argv[gnuoptind++];
	    nextchar = NULL;
	  }
      }
    return c;
  }
}

int
gnugetopt (int argc, char *const *argv, const char *optstring)
{
  return _getopt_internal (argc, argv, optstring,
			   (const struct option *) 0,
			   (int *) 0,
			   0);
}
