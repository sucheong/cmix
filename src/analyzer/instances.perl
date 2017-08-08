# Authors:  Jens Peter Secher (jpsecher@diku.dk)
#           Arne Glenstrup (panic@diku.dk)
#           Henning Makholm (makholm@diku.dk)
# Content:  C-Mix system: Generator file for instances.c
#
# Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
# Redistribution and modification are allowed under certain
# terms; see the file COPYING.cmix for details.

while($org = shift) {
	print "#line 1 \"$org\"\n" ;
	open(ORG,$org) or die "Cannot find $org" ;
	$template = "" ;
	while(<ORG>) {
		if ( /^====+ *(.*)\n?$/ ) {
			$template = "$template $1" ;
			print "\n";
		} elsif ( /^[ \t\n]*$/ ) {
			$template = "" ;
			print $_ ;
		} elsif ( $template eq "" ) {
			print $_ ;
		} else {
			s/\n$// ;
			print "class $_;" if /^[ \t]*[^-][a-zA-Z0-9_]+[ \t]*$/ ;
			s/-//g ;
			$arg = $_ ;			
			$_ = $template ;
			s/@/ $arg /g ;
			s/ +/ /g ;
			print "$_\n" ;
		}
	}
}

