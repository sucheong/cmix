# Authors:  Jens Peter Secher (jpsecher@diku.dk)
#           Arne Glenstrup (panic@diku.dk)
#           Henning Makholm (makholm@diku.dk)
# Content:  C-Mix system: Generator file for taboos.c
#
# Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
# Redistribution and modification are allowed under certain
# terms; see the file COPYING.cmix for details.

while($org = shift) {
	print "#line 1 \"$org\"\n" ;
	open(ORG,$org) or die "Cannot find $org" ;
	$active = 0 ;
	while(<ORG>) {
		if ( /^%/ ) {
			next ;
		} elsif ( /-+ *(\w+) *-+/ ) {
			$next = $1 ;
			finishoff() if $active ;
			$active = $next ;
			%thesewords = () ;
		} else {
			foreach $word (split) {
				$active or die "Got words without place" ;
				$thesewords{$word} = "" ;
			}	
		}
	}
	finishoff() if $active ;
}

sub finishoff() {
	print "char const * ${active}[] = {\n  " ;
	$rest = 76 ;
	foreach $word ( %thesewords ) {
		# we get index-value pairs; want only indices
		next unless $word ;
		if ( $rest < length($word) + 3 ) {
			print "\n  " ;
			$rest = 76 ;
		}
		$rest -= length($word)+3 ;
		print "\"$word\"," ;
	}
	print "\n  " if ( $rest < 10 ) ;
	print "(char*)0};\n" ;
}

