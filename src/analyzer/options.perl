# Authors:  Jens Peter Secher (jpsecher@diku.dk)
#           Arne Glenstrup (panic@diku.dk)
#           Henning Makholm (makholm@diku.dk)
# Content:  C-Mix system: Generator file for options.cc
#
# Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
# Redistribution and modification are allowed under certain
# terms; see the file COPYING.cmix for details.

$option_string = ":" ;
# The colon makes some getopts complain in an identifiable manner
# when an argument is missing
$option_long = "" ;
$option_cases = "" ;
$option_help = "" ;
$debug_table = "" ;

while($org = shift) {
	print "#line 1 \"$org\"\n" ;
	$line = 0 ;
	open(ORG,$org) or die "Cannot find $org" ;
	$template = "" ;
	while(<ORG>) {
		$line = $line + 1 ;
		if ( /^ *# *e(ndif|lse)/ ) {
			print ;
			print "#line $line\n\n" ;
			next ;
		}
		print,next unless /^ *%(.*)/s ;
		$_ = $1 ;
		if ( /^OPTION \s*
		      -(.) \s*
		      ( --\S* ( \s+ --\S* )* | ) \s*
		      ( ARG | OPTIONAL | )? \s*
		      " ([^"]*) " \s*
		      { (.*) $ /sx ) {
			$option_string .= $1 ;
			if ( $4 ) {
				$helptemplate="xxx" ;
				$option_string .= ":" ;
				$option_string .= ":" if $3 eq "OPTIONAL" ;
				$long_mode = ($4 eq "OPTIONAL") ?
					"optional_argument" :
					"required_argument" ;
			} else {
				$helptemplate="   " ;
				$long_mode = no_argument ;
			}
			foreach $l ( split /\s+/,$2 ) {
			  $option_long .=
				"{(char*)\"$l\",$long_mode,NULL,'$1'},\n" ;
			}
			$option_help .= "\"  -$1$helptemplate - $5\\n\"\n" ;
			$option_case .= "#line $line\ncase '$1': {" ;
			$_ = $6 ;
			$level = 1 ;
			do {
				if ( m:^( ( [^"/{}']
					  | '[^\\]'
					  | '\\[^']+'
					  | "([^\\]|\\.)+"
					  | //.*\n
					  | /[^/] )+ ) (.*):sx ) {
					$option_case .= $1 ;
					$_ = $4 ;
					print "\n" if $1 =~ /(.*\n.*)/ ;
				} elsif ( /^{(.*)/s ) {
					$option_case .= "{" ;
					$_ = $1 ;
					$level = $level+1 ;
				} elsif ( /^}(.*)/s ) {
					$option_case .= "}" ;
					$_ = $1 ;
					$level = $level-1 ;
				} else {
					$_ = <ORG> ;
					die
					  "$org:$line: unexpected end of file"
					  if eof ;
					$line = $line + 1;
				}
			} while ( $level > 0 );
			$option_case .= "break;\n" ;
			print  ;
		} elsif ( /^OPTION_SHORT/ ) {
			print "static char short_list[]=\"$option_string\";\n";
		} elsif ( /^OPTION_LONG/ ) {
			$option_long =~ s/"--/"/g ;
			print $option_long ;
			print "#line $line\n\n" ;
		} elsif ( /^OPTION_CASES/ ) {
			$option_case =~ s/\$/optarg/g ;
			print $option_case ;
			print "#line $line\n\n" ;
		} elsif ( /^OPTION_HELP/ ) {
			print $option_help ;
			print "#line $line\n\n" ;
		} elsif ( /^DEBUGSWITCH\s+(\S+)\s+(\S+)/ ) {
			$debug_table .= "{\"$2\",&$1,NULL,0},\n" ;
			print "bool $1 = false;\n" ;
		} elsif ( /^DEBUGSTREAM\s+(\S+)\s+([0-9])\s*$/ ) {
			$debug_table .= "{\"$1\",NULL,&debugstream_$1,$2},\n" ;
			print "DebugStream debugstream_$1; \n" ;
		} elsif ( /^DEBUG_TABLE/ ) {
			print $debug_table ;
			print "#line $line\n\n" ;
		} else {
			print stderr "$org:$line: bad control line\n" ;
			print "#error bad control line: $_" ;
		}
	}
}
