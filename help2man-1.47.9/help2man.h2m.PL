#!/usr/bin/perl

# Self extracting help2man.h2m script.

#   -q, --quiet            Suppress extraction message
#   -s, --stdout           Extract to stdout
#   -o, --output=FILE      Extract to FILE
#   -l, --locale=STRING    Output file for locale STRING
#   -m, --message-dir=DIR  Directory for message catalogs

# Copyright (C) 2012, 2013, 2014 Free Software Foundation, Inc.

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

# Written by Brendan O'Dea <bod@debian.org>

use 5.008;
use Config;
use Getopt::Long;
use POSIX qw(setlocale LC_ALL);

my %opts;
die "Usage: $0 [--quiet] [--stdout|--output=FILE] [--locale=STRING] ",
    "[--message-dir=DIR]\n" unless GetOptions \%opts,
	qw(quiet stdout output=s locale=s message-dir=s) and !@ARGV;

my $DOMAIN = 'help2man';
my $target = $0;
my $tmp;
if ($opts{stdout})
{
    *OUT = *STDOUT;
    $opts{quiet} = 1;
}
else
{
    if ($opts{output})
    {
	$target = $opts{output};
    }
    else
    {
	$target =~ s!.*/!!;
	$target =~ s/\.PL$// or die "$0: can't determine target name\n";
	if ($opts{locale})
	{
	    $target =~ s/(\.[^.]*)$/.$opts{locale}$1/
	      or $target =~ s/$/.$opts{locale}/;
	}
    }

    $tmp = "$target.tmp$$";
    unlink $tmp       or die "$0: can't unlink $tmp ($!)\n" if -e $tmp;
    open OUT, ">$tmp" or die "$0: can't create $tmp ($!)\n";
}

my $msg = "Extracting $target";
$msg .= ' (with locale substitutions)' if $opts{locale};
print $msg, "\n" unless $opts{quiet};

sub _;
if ($opts{locale})
{
    delete @ENV{qw(LANGUAGE LC_MESSAGES LANG)};
    setlocale LC_ALL, $ENV{LC_ALL} = $opts{locale};

    require Locale::gettext;
    require I18N::Langinfo;
    require Encode;
    my $gettext = Locale::gettext->domain($DOMAIN);
    $gettext->dir($opts{'message-dir'}) if $opts{'message-dir'};
    my $encoding = I18N::Langinfo::langinfo(I18N::Langinfo::CODESET());
    *_ = sub { Encode::encode($encoding, $gettext->get($_[0])) }
}
else
{
    *_ = sub { $_[0] }
}

print OUT _('Include file for help2man man page'), "\n";
print OUT "\n";
print OUT '--locale=', $opts{locale} || 'C', "\n";
print OUT "\n";

print OUT '[', _('NAME'), "]\n";
print OUT _('help2man \- generate a simple manual page'), "\n";
print OUT "\n";

print OUT '[', _('INCLUDE FILES'), "]\n";
print OUT _(<<'EOT');
Additional material may be included in the generated output with the
.B \-\-include
and
.B \-\-opt\-include
options.  The format is simple:

    [section]
    text

    /pattern/
    text
EOT
print OUT "\n";

print OUT _(<<'EOT');
Blocks of verbatim *roff text are inserted into the output either at
the start of the given
.BI [ section ]
(case insensitive), or after a paragraph matching
.BI / pattern /\fR.
EOT
print OUT "\n";

print OUT _(<<'EOT');
Patterns use the Perl regular expression syntax and may be followed by
the
.IR i ,
.I s
or
.I m
modifiers (see
.BR perlre (1)).
EOT
print OUT "\n";

print OUT _(<<'EOT');
Lines before the first section or pattern which begin with `\-' are
processed as options.  Anything else is silently ignored and may be
used for comments, RCS keywords and the like.
EOT
print OUT "\n";

print OUT _('The section output order (for those included) is:'), "\n";
print OUT "\n";
print OUT '    ', _('NAME'), "\n";
print OUT '    ', _('SYNOPSIS'), "\n";
print OUT '    ', _('DESCRIPTION'), "\n";
print OUT '    ', _('OPTIONS'), "\n";
print OUT '    ', _('ENVIRONMENT'), "\n";
print OUT '    ', _('FILES'), "\n";
print OUT '    ', _('EXAMPLES'), "\n";
print OUT '    \fI', _('other'), "\\fR\n";
print OUT '    ', _('AUTHOR'), "\n";
print OUT '    ', _('REPORTING BUGS'), "\n";
print OUT '    ', _('COPYRIGHT'), "\n";
print OUT '    ', _('SEE ALSO'), "\n";
print OUT "\n";

print OUT _(<<'EOT');
Any
.B [NAME]
or
.B [SYNOPSIS]
sections appearing in the include file will replace what would have
automatically been produced (although you can still override the
former with
.B \-\-name
if required).
EOT
print OUT "\n";

print OUT _(<<'EOT');
Other sections are prepended to the automatically produced output for
the standard sections given above, or included at
.I other
(above) in the order they were encountered in the include file.
EOT
print OUT "\n";

print OUT _(<<'EOT');
Placement of the text within the section may be explicitly requested by using
the syntax
.RI [< section ],
.RI [= section ]
or
.RI [> section ]
to place the additional text before, in place of, or after the default
output respectively.
EOT
print OUT "\n";

print OUT '[', _('AVAILABILITY'), "]\n";
print OUT _('The latest version of this distribution is available on-line from:'), "\n";
print OUT "\n";
print OUT "    ftp://ftp.gnu.org/gnu/help2man/\n";

# Fix output file permissions
unless ($opts{stdout})
{
    close OUT;
    rename $tmp, $target or die "$0: can't rename $tmp to $target ($!)\n";
    chmod 0444, $target  or warn "$0: can't change mode of $target ($!)\n";
}

exit 0;
