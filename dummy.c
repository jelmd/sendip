/** dummy.c - example sendip module
 * Created by Mike Ricketts <mike@earth.li>
 */

/* To write a new sendip module:
 * * Check https://github.com/jelmd/sendip/issues to find out, whether someone
 *   is already working on something like this. If not, please file a new issue
 *   (to notify people)!
 * * copy dummy.c and dummy.h
 * * replace dummy with the name of your module throughout
 * * In <your_module>.h:
 *    - fill in the struct foo_header with all the header fields in your
 *      module's packet.  If the packet is a variable length, only put the
 *      common bits here and use additional structs for other bits, if needed.
 *      Be very careful about introducing byteorder dependencies.  See ntp.h
 *      for a simple case of how to do it.  In general, things smaller than
 *      16 bits are problematic, use the __BYTE_ORDER macro and test against
 *      __LITTLE_ENDIAN, __BIG_ENDIAN.  Always have a #else catchall with a
 *      #error in, just in case.
 *      Every field should be a u_int*_t or an int*_t to avoid things being
 *      differnt lengths from you expect.  Use these rather than equivalent
 *      ones as these will exist everywhere that sendip compiles.
 *    - create a list of #defines FOO_MOD_*, one for each header field that
 *      may be modified.  The first should have value 1, the rest should be
 *      1 << x for increasing values of x.
 *    - fill in the foo_opts array for all the options your module supports.
 *      Each entry has the format:
 *      { opt_string, arg, description, default }
 *      opt_string is the option that is used to set it, EXCLUDING the -x that
 *      tells sendip which module.  arg is 0 (for no value) or 1 if the option
 *      takes a value (almost always).  description should be a short
 *      explanation of what the option does, and default should be its default
 *      value (as a string, can be NULL if there is no default).
 *    - remove the #error line at the top
 * * In <your_module>.c:
 *    - remove this essay (you can still read it in dummy.c!)
 *    - change the top comment in the obvious way
 *    - find an option character not used elsewhere and replace opt_char with
 *      that.  You can see what is used by doing
 *      grep '^const char opt_char' *.c
 *      in the sendip source directory.
 *    - adjust the initialize() function as needed. This function gets called
 *      each time a new packet gets created by sendip. Therefore do not use any
 *      global or static variables. Instead put them into a separate struct and
 *      initialize and set sendip_data->private to the pointer to this struct.
 *      Thus, when other relevant functions gets called, you'll always get the
 *      right data for the related package, because sendip_data gets passed to
 *      such functions. Also note, that you need to deallocate alias free all
 *      dynamically allocated memory (via malloc, alloc, calloc, strdup, etc.)
 *      optimal as soon as you do not need it anymore (got copied to the data
 *      area of the packet) and set related pointer vars to NULL (optional but
 *      good practice to avoid double frees), but no later than in the
 *      finalize() function to avoid memory leaks. However, sendip_data->data
 *      gets freed by sendip itself (so do not free this one).
 *    - in the do_opt function, fill in code for all the options you defined in
 *      the header file.  Typically, the code will look a lot like:
 *      case 'option':
 *        header->thing = htons((u_int16_t) strtoul(arg, NULL, 0));	// OR
 *        header->thing = opt2intn(arg, 2);         // 2 for 16-bit OR
 *        header->thing = hostargument(arg, 2);     // if not byte-swapped
 *        pack->modified |= FOO_MOD_THING;
 *        break;
 *      If some of your options change the length of the packet, you might want
 *      to take a look in ipv4.c or tcp.c - specifically where they add IPV4 or
 *      TCP options.
 *      Make sure you use htons and htonl everywhere you need to avoid
 *      byteorder problems. For input arguments the functions in parseargs.c
 *      usually take care of it.
 *      -opt contains the option string, including the starting opt_char
 *      -arg contains any argument given
 *      -pack contains our headers
 *    - in the finalize function, fill in anything that needs to be computed
 *      after all the options are processed and free all dynamically allocated
 *      memory (private data) to avoid memory leaks.  This function MUST NOT
 *      change the length or location of the headers in memory, else bad things
 *      will happen.  Typical things that go in here are filling in the length
 *      field of the header if it hasn't been overriden, computing checksums,
 *      etc.  You may also whish to check that your packet is enclosed in a
 *      sensible carrier.  tcp.c does all of the things.
 *      -hdrs is build by taking the opt_char for each packet in turn from the
 *       outside in, up to this packet inclusive.
 *      -headers is an array of all the enclosing headers in the same order
 *      -data contains the data inside this set of headers.  This may include
 *       headers of underlying protocols, that will already have been
 *       finalized.  DO NOT MODIFY IT _unless_ you really need to! (ESP)
 *      -pack contains our headers.
 *    - You might, possibly, find the following functions useful.  They are
 *      automatically available to all modules:
 *      -int str2val(char *in, const char *out, size_t free);
 *       For strings starting 0x or 0X, converts each pair of bytes thereafter
 *       to a single byte of that hex value.  For other strings starting 0,
 *       converts sets of 3 bytes to a single byte of that octal value.  For
 *       all other strings, does nothing.  Returns the length of the final
 *       string.  This is recomended when parsing arbitrary data (like the -d
 *       option of sendip, -tonum for arbitrary TCP options)
 *      -int opt2val(char *output, const char *input, size_t outlen);
 *       The "standard" string argument routine - does either str2val
 *       above, or for strings of the form rN and zN, generates N random or
 *       nul (zero) bytes respectively, or fPATH, which gets replaced by the
 *       value read from a line in PATH (see fargs.c for more info).
 *      -u_int16_t csum(u_int16_t *data, int len)
 *       returns the standard internet checksum of the packet
 *    - If something doesn't work as expected, or you can't figure out how to
 *      do something, open an issue via https://github.com/jelmd/sendip/issues
 * * In the Makefile add <your_module>.so to on *PROTOS line
 * * Test it
 * * Use https://github.com/jelmd/sendip/pulls to create a new pull request
 *   or open an issue via https://github.com/jelmd/sendip/issues and attach your
 *   patch or new files you created to this issue.
 */

#include <stdlib.h>
#include <sys/types.h>
#include "sendip_module.h"

#include "common.h"
#include "dummy.h"

/* Character that identifies our options */
const char opt_char = 'dummy';

sendip_data
*initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	dummy_header *dummy = malloc(sizeof(dummy_header));
	// priv_data *priv = malloc(sizeof(priv_data));
	memset(dummy, 0, sizeof(dummy_header));
	ret->alloc_len = sizeof(dummy_header);
	ret->data = dummy;
	// ret->private = priv;
	ret->modified = 0;
	return ret;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	dummy_header *dummy = (dummy_header *) pack->data;

	switch (opt[1]) {
		//...
	}
	return TRUE;
}

bool
finalize(char *hdrs, sendip_data *headers[], int index, sendip_data *data,
	sendip_data *pack)
{
	// free(data->private);
	//...
	return TRUE;
}

int
num_opts() {
	return sizeof(dummy_opts) / sizeof(sendip_option);
}

sendip_option
*get_opts() {
	return dummy_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
