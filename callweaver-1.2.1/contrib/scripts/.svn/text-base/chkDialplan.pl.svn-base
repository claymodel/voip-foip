#!/usr/bin/perl -w
#	Perl extensions.conf validation script
#
#	chkDialplan.pl
#
#	Original Author: Santiago Baranda (sbaranda_at_gmail_dot_com)
#	Modifications by: Sam Bingner (sbingner_at_gmail_dot_com)
#
use strict;

my $def_applications = "applications.txt";
my $def_extensions = "extensions.conf";
my (@inc_list, %inc_list);
my $validation_option = 1;
## ---------------------------------------------------------------------------
## Sub do_usage
## ---------------------------------------------------------------------------
##
sub do_usage {
	print "Usage: $0 <modify|validate> [applications.txt [extensions.conf]]
Options:
	modify: Modifies your $def_extensions and all the included files
		Your original files are renamed to .bak
		WARNING: If you run this script it will overwrite your existing .bak files
	validate: Validates your $def_extensions and all the included files
		It will generate a logfile called validation.log
		WARNING: If you run this script it will overwrite your existing validation.log file
";
	exit 1;
}

## ---------------------------------------------------------------------------
## Sub parse_args
## ---------------------------------------------------------------------------
##
sub parse_args {
	if (defined $ARGV[0]) {
		if ($ARGV[0] eq 'modify') {
			$validation_option = 0;
		}
		elsif ($ARGV[0] eq 'validate') {
			unlink("validation.log");
		}
		else {
			do_usage();
		}
		$def_applications = $ARGV[1] if (defined $ARGV[1]);
		$def_extensions   = $ARGV[2] if (defined $ARGV[2]);
	}
	else {
		do_usage();
	}
}
## ---------------------------------------------------------------------------
## Sub build_applist
## ---------------------------------------------------------------------------
##
sub build_applist {
	my %app_list;
	open(APPS, $def_applications) || die "$def_applications is missing";
	while (my $app = <APPS>) {
		chomp $app;
		if ($app) {
			if ($app =~ /^([^=]+)=(.+)/) {
				$app_list{lc($1)} = $2;
			} 
			else {
				$app_list{lc($app)} = $app;
			}
		}
	}
	close(APPS);
	return \%app_list;
}

## ---------------------------------------------------------------------------
## Sub parse_ext_args
## ---------------------------------------------------------------------------
##
sub parse_ext_args {
	my ($exten, $prio, $newcmd, $origargs) = @_;

	$newcmd =~ s/\<EXTEN\>/$exten/g;
	$newcmd =~ s/\<PRIO\>/$prio/g;
	$newcmd =~ s/\<ARGS\>/$origargs/g;
	return $newcmd;
}

## ---------------------------------------------------------------------------
## Sub parse_extlist
## ---------------------------------------------------------------------------
##
sub parse_extlist {
	my ($filename, $ref_app_list, $validate) = @_;
	if ($validate) {
		open(OUTPUT, ">>validation.log") || die "Unable to open output file - do you have permissions?";
	}
	unless (open(EXTS, $filename)) {
		if ($filename eq $def_extensions) {
			die "Unable to open default file: $filename";
		} 
		else {
			if ($validate) {
				print OUTPUT "ERROR: Unable to open file $filename included at $inc_list{$filename}\n";
				close(OUTPUT);
			}
			print STDERR "ERROR: Unable to open file $filename included at $inc_list{$filename}\n";
			return 0;
		}
	}
	unless ($validate) {
		open(OUTPUT, ">tmp_$filename") || die "Unable to open output file - do you have permissions?";
	}
	my %invcommands;
	my $linenum = 0;
	while (my $ext_line = <EXTS>) {
		chomp($ext_line);
		my $orig_line = $ext_line;
		$linenum++;
                my $comment = '';
		my $new_line = '';
		my $linecmd;
		my $unknown = 0;
		if ($ext_line =~ s/(;.+)$//) {
			$comment = $1;
		}
		if ($ext_line =~ /^\s*exten\s*\=\>\s*([^,]+),([\dn]+),([^\(]+)\((.*)\)(\s*)$/i) {
			my ($exten, $prio, $command, $args, $space) = ($1, $2, $3, $4, $5);
			my $newcommand = '';
			my $cmd = lc($command);
			if (defined $ref_app_list->{$cmd}) {
				if ($cmd eq 'agi') {
					$args =~ s/^([^\)]+)\.agi/$1.ogi/;
				}
				if ($ref_app_list->{$cmd} =~ /\</) {
					$linecmd = parse_ext_args($exten, $prio, $ref_app_list->{$cmd}, $args);
					$new_line = sprintf("exten => %s,%s,%s", $exten, $prio, $linecmd);
				} 
				else {
					$cmd = $ref_app_list->{$cmd};
					$linecmd = $cmd if ($3 ne $cmd);
					$new_line = sprintf("exten => %s,%s,%s(%s)", $exten, $prio, $cmd, $args);
				}
			} 
			else {
				$unknown = $cmd = $3;
				print STDERR "Unknown command: $cmd\n" unless (defined $invcommands{$cmd});
				$invcommands{$cmd} = 1;
				$new_line = sprintf("exten => %s,%s,%s(%s)", $exten, $prio, $cmd, $args);
			}
			$new_line .= $space if (defined $space);
		} 
		elsif ($ext_line =~ /^\s*\[(\S+)\]/) {
			my $context = $ext_line;
			if ($context =~ s/^\[(proc|macro)-([^\]]+)\]/\[proc-$2\]/i) {
				$linecmd = "[proc-$2]" if ($1 ne 'proc');
			}
			$new_line = $context;
		} 
		elsif ($ext_line =~ /^\#include (.*)$/i) {
			unless (defined $inc_list{$1}) {
				$inc_list{$1} = "$filename:$linenum";
				push(@inc_list, $1);
				$new_line = $ext_line;
			}
		} 
		else {
			$new_line = $ext_line;
		}
		$new_line .= $comment;
		if ($validate) {
			if ($unknown) {
				print OUTPUT "$filename:$linenum: '$orig_line' has unknown command $unknown\n"
			} 
			elsif (defined $linecmd) {
				print OUTPUT "$filename:$linenum: '$orig_line' should be $linecmd\n"
			}
		} 
		else {
			print OUTPUT $new_line . "\n" ;
		}
	}
	close(EXTS);
	close(OUTPUT);
	return 1;
}

parse_args();
my $inc_off = 0;
my $applist = build_applist();
push @inc_list, $def_extensions;
while (defined $inc_list[$inc_off]) {
	print "parsing $inc_list[$inc_off]\n";
	if (parse_extlist($inc_list[$inc_off], $applist, $validation_option) && (!$validation_option)) {
		rename("$inc_list[$inc_off]", "$inc_list[$inc_off].bak");
		rename("tmp_$inc_list[$inc_off]", "$inc_list[$inc_off]");
		print "Saved $inc_list[$inc_off] is in $inc_list[$inc_off].bak\n";
	}
	$inc_off++;
}
if ($validation_option) {
	print "validation log is in validation.log\n";
}
