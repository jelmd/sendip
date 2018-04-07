#include <errno.h>
#include <search.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "fargs.h"

/* a file cache entry */
typedef struct _fc {
	const char *path;
	unsigned int len;
	unsigned int idx;
	char *lines[];
} fc;

/* the root of the file cache tree used by this instance */
static void *root = NULL;

/* file cache entry comparator */
static int
fcmp(const void *n1, const void *n2) {
	const fc *fa = (const fc *) n1;
	const fc *fb = (const fc *) n2;
	if (fa == NULL && fb == NULL)
		return 0;
	if (fa == NULL)
		return -1;
	else if (fb == NULL)
		return 1;
	if (fa->path == NULL && fb->path == NULL)
		return 0;
	if (fa->path == NULL)
		return -1;
	else if (fb->path == NULL)
		return 1;
	return strcmp(fa->path, fb->path);
}

static void
fc_free(fc *f) {
	int i;

	for (i = f->len - 1; i >= 0; i--)
		free(f->lines[i]);
	free((void *)(unsigned long)f->path);
	free(f);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int noop(const void *keyA, const void *keyB) { return 0; }
#pragma GCC diagnostic pop


/* Delete all file cache entries. */
void
fargs_destroy(void)
{
	fc *f;

	while (root != NULL) {
		f = *(fc **) root;
		tdelete((void *) f, &root, noop);
		fc_free(f);
	}
}

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

/* Create and populate a new file cache item */
static fc *
fc_create(const char *path)
{
	FILE *fp;
	fc *f, *t;
	int capacity, i, len;
	struct stat statbuf;
	char line[LINE_MAX];
	char *s;

	if ((fp = fopen(path, "r")) == NULL)
		return NULL;

	/* guess a line count based on size */
	if (stat(path, &statbuf) < 0)
		return NULL;

	capacity = statbuf.st_size / 5;
	if (capacity < 4)
		capacity = 4;
	f = (fc *) malloc(sizeof(fc) + capacity * sizeof(char *));
	if (f == NULL) {
		perror("Unable to create file contents table");
		return NULL;
	}
	if ((f->path = strdup(path)) == NULL) {
		perror("Unable to populate file contents table");
		fc_free(f);
		return NULL;
	}
	/* read the lines into memory */
	errno = 0;
	for (i = 0; (s = fgets(line, LINE_MAX, fp)) != NULL; i++) {
		if (i >= capacity) {
			capacity *= 2;
			t = (fc *) realloc(f, sizeof(fc) + capacity * sizeof(char *));
			if (t == NULL) {
				snprintf(line, LINE_MAX, "Unable to extent file contents table "
					"to %d lines for '%s'", capacity, path);
				i--;
				break;
			}
			f = t;
		}
		len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[--len] = '\0';
		if (len == 0)
			fprintf(stderr, "Warning: empty line (%s: %d)\n", path, i);
		f->lines[i] = strdup(line);
		line[0] = '\0';
	}
	f->len = i;
	i = errno;
	fclose(fp);
	if (i != 0) {
		if (line[0] == '\0') {
			snprintf(line, LINE_MAX, "Stopped reading file '%s' on line %d",
				path, f->len);
		}
		fprintf(stderr, "%s: %s\n", line, strerror(i));
		fc_free(f);
		return NULL;
	}
	f->idx=0;
	return f;
}

/* Find the entry for a given file. If there isn't one, create it. */
static fc *
fc_find(const char *path)
{
	fc t;
	fc *f;
	void *p;

	t.path = path;
	if ((p = tfind(&t, &root, fcmp)) == NULL) {
		if ((f = fc_create(path)) == NULL) {
			perror(path);
			return NULL;
		}
		if ((p = tsearch((void *) f, &root, fcmp)) == NULL) {
			perror(path);
			return NULL;
		}
	}
	return *(fc **) p;
}

#ifdef TEST
static int lino;
#endif

/* Get the next line (or first line if EOF has been reached) of the file with
 * the given name. Once a file has been read and therefore cached, all lookups
 * get satisfied via the cache unless the cache gets completely cleared using
 * the fargs_destroy() function.
 *
 * @fname  the path of the file. This string is used AS IS to lookup cached
 *		file entries!
 * @return NULL if the file is not readable, the line without a trailing '\n'
 *	otherwise.
 */
char *
fileargument(const char *fname)
{
	fc *f;
	char *line;

	if ((f = fc_find(fname)) == NULL)
		return NULL;
#ifdef TEST
	lino = f->idx;
#endif
	line = f->lines[f->idx++];
	if (f->idx >= f->len)
		f->idx = 0;
	return line;
}

#ifdef TEST
int
main(int argc, char **argv)
{
	char arg[LINE_MAX];
	char *line;
	size_t l;

	while (1) {
		printf("file: ");
		if ((fgets(arg, LINE_MAX, stdin)) == NULL)
			break;

		l = strlen(arg);
		if (arg[l - 1] == '\n')
			arg[--l] = '\0';
		if (l == 0)
			break;
		line = fileargument(arg);
		if (line)
			printf("%s %d: '%s'\n", arg, lino, line);
		else
			printf("not found\n");
	}
	fargs_destroy();
}
#endif
