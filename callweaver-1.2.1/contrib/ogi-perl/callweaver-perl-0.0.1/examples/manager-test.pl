#!/usr/bin/perl
#
# Example script to show how to use CallWeaver::Manager
#
# Written by: James Golovich <james@gnuinter.net>
#
#

use lib './lib', '../lib';
use CallWeaver::Manager;

$|++;

my $opbman = new CallWeaver::Manager;

$opbman->user('test');
$opbman->secret('test');
$opbman->host('duff');

$opbman->connect || die $opbman->error . "\n";

$opbman->setcallback('Hangup', \&hangup_callback);
$opbman->setcallback('DEFAULT', \&default_callback);


#print STDERR $opbman->command('zap show channels');

print STDERR $opbman->sendcommand( Action => 'IAXPeers');

#print STDERR $opbman->sendcommand( Action => 'Originate',
#					Channel => 'Zap/7',
#					Exten => '500',
#					Context => 'default',
#					Priority => '1' );


$opbman->eventloop;

$opbman->disconnect;

sub hangup_callback {
	print STDERR "hangup callback\n";
}

sub default_callback {
	my (%stuff) = @_;
	foreach (keys %stuff) {
		print STDERR "$_: ". $stuff{$_} . "\n";
	}
	print STDERR "\n";
}
