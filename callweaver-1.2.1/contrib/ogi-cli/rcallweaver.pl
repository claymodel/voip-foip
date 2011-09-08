#!/usr/bin/perl 
#
# @2007 by Antonio Gallo (AGX)
#          http://www.badpenguin.org/
#
# TODO:
# 	add option "-r" for compatibility
# 	add option "-x" for compatibility
# 	add other command line options for comatibility
#	add options to specify server, username and password
#	add option to parse a config file
#	add option to parse manager.conf and grab user/pass from it
#
# KNOW BUGS:
# 	clean for now


use CallWeaver::Manager;
use strict;
use warnings;
use diagnostics;

#
# Parameter checking
#
my ($mycommand) = @ARGV;
if ( (!defined($mycommand)) && ($mycommand ne '') ) {
	print STDERR "usage: rcallweaver.pl 'sip show peers'\n";
	exit 1;
}

#
# Config vars
#
my $CW_hostname = 'localhost';
my $CW_username = 'webmin';
my $CW_password = 'webmin';

#
# Prepare
#
my $manager = new CallWeaver::Manager;
$manager->user($CW_username);
$manager->secret($CW_password);
$manager->host($CW_hostname);

#
# Connect
#
my $res = $manager->connect;
if (!$manager->connected) {
	$res = 'server is down' if (!defined($res));
	print STDERR "cannot connect to CallWeaver, reason: $res\n";
	exit(2);
}

#
# Send
#
my $EOL = "\r\n";
my $command = 'Action: command'. $EOL. 'Command: '. $mycommand. $EOL. $EOL;
$manager->connfd->send($command);

#
# Parse output and print
#
my $tfd = $manager->connfd;
my $result = 0;
while (my $line = <$tfd>) {
	last if ($line eq $EOL);
	chomp $line;
	# Do not print the response but set the error flag if it happens
	next if ( $line=~ /^Response: Follows/ );
	next if ( $line=~ /^Response: Success/ );
	$result=1 if ( $line=~ /^Response: / );
	# Do not print other stuffs
	next if ( $line=~ /^Privilege: / );
	next if ( $line=~ /^--END COMMAND--/ );
	# print it
	print "$line\n";
}

#
# Disconnect and leave
#
$manager->disconnect if (!$manager->connected);
exit $result;
