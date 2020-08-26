/*
 * Nasty preload hack to allow message catalogs to be read from the build tree.
 *
 * export LD_PRELOAD=/usr/lib/help2man/bindtextdomain.so
 * export TEXTDOMAIN=program
 * export LOCALEDIR=${DESTDIR}/usr/share/locale
 *
 * Copyright (C) 2012 Free Software Foundation, Inc.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.  This file is offered as-is,
 * without any warranty.
 *
 * Written by Brendan O'Dea <bod@debian.org>
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#define PRELOAD "bindtextdomain.so"

static void die(char const *msg)
{
    fprintf(stderr, PRELOAD ": %s\n", msg);
    exit(1);
}

static char *e_textdomain = 0;
static char *e_localedir = 0;
static char *(*r_textdomain)(char const *) = 0;
static char *(*r_bindtextdomain)(char const *, char const *) = 0;
static char *(*r_bind_textdomain_codeset)(char const *, char const *) = 0;

void setup()
{
    static int done = 0;
    if (done++)
        return;

    if (!(e_textdomain = getenv("TEXTDOMAIN")))
	die("TEXTDOMAIN not set");

    if (!(e_localedir = getenv("LOCALEDIR")))
	die("LOCALEDIR not set");

    if (!(r_textdomain = dlsym(RTLD_NEXT, "textdomain")))
	die("can't find symbol \"textdomain\"");

    if (!(r_bindtextdomain = dlsym(RTLD_NEXT, "bindtextdomain")))
	die("can't find symbol \"bindtextdomain\"");

    if (!(r_bind_textdomain_codeset = dlsym(RTLD_NEXT,
    					    "bind_textdomain_codeset")))
	die("can't find symbol \"bind_textdomain_codeset\"");
}

char *textdomain(char const *domainname)
{
    char *r;
    setup();
    r = r_textdomain(domainname);
    if (domainname && !strcmp(domainname, e_textdomain))
        r_bindtextdomain(domainname, e_localedir);

    return r;
}

char *bindtextdomain(char const *domainname, char const *dirname)
{
    char const *dir = dirname;
    setup();
    if (domainname && !strcmp(domainname, e_textdomain))
        dir = e_localedir;

    return r_bindtextdomain(domainname, dir);
}

char *bind_textdomain_codeset(char const *domainname, char const *codeset)
{
    char *r;
    setup();
    r = r_bind_textdomain_codeset(domainname, codeset);
    if (domainname && !strcmp(domainname, e_textdomain))
        r_bindtextdomain(domainname, e_localedir);

    return r;
}
