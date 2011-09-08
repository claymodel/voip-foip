#!/usr/bin/perl 

use CallWeaver::Manager;
use strict;
use warnings;
use diagnostics;

$SIG{INT} = \&safe_exit;
$SIG{QUIT} = \&safe_exit;
$SIG{TERM} = \&safe_exit;

# Config vars
#
my $CW_hostname = 'localhost';
my $CW_username = 'webmin';
my $CW_password = 'webmin';

my $manager = new CallWeaver::Manager;
$manager->user($CW_username);
$manager->secret($CW_password);
$manager->host($CW_hostname);

sub printlog($$) {
	my ($a,$b) = @_;
	print "[$a] $b\n";
}


# Enter Loop
my $CW_connected = 1;
while( 1 ) {
	printlog('info', 'Connecting to CallWeaver...');
	if( $manager->connect ) {
		$CW_connected = 1;
		printlog('info', 'Connected to CallWeaver!');
		$manager->setcallback('DEFAULT', \&status_callback);
		eval { $manager->eventloop; };
		exit;
	} else {
		printlog('err', 'Could not connect to CallWeaver!') if $CW_connected;
		$CW_connected = 0;
	}
	sleep 1;
}

#
# Globals
#
my %channels;



# Install signall handler
#

sub Status2String ($) {
	my ($status) = @_;
	#print "STATUS=". $status. ".\n";
	return 'UNDEFINED' 		if (!defined($status));
	return 'Removed'		if ($status eq '-2');
	return 'Deactivated'		if ($status eq '-1');
	return 'Idle'	if ($status eq '0');
	return 'InUse'	 		if ($status eq '1');
	return 'Busy' 			if ($status eq '2');
	return 'Unavail/Unregistered'	if ($status eq '4');
	return 'Ringing' 		if ($status eq '8');
	return 'Call Waiting???'	if ($status eq '9');
	return 'On Hold???'		if ($status eq '16');
	return 'UNKNOWN:'. $status.".";
}

sub DeviceState2String ($) {
	my ($status) = @_;
	return 'UNDEFINED' 	if (!defined($status));
	return 'Unknown'	if ($status eq '0');
	return 'Idle'		if ($status eq '1');
	return 'In Use'		if ($status eq '2');
	return 'Busy' 		if ($status eq '3');
	return 'Invalid' 	if ($status eq '4');
	return 'Unavailable' 	if ($status eq '5');
	return 'Ringing' 	if ($status eq '6');
	return 'DeviState2String('. $status.")";
}

sub print_full_event($) {
	my ($ev) = @_;
	return if (!defined($ev));
	print "> ";
	foreach (keys %{$ev}) {
		next if ($_ eq 'Privilege');
		print "$_: ". $ev->{$_}. ",  ";
	}
	print "\n";
}

sub status_callback {
	my (%E) = @_;

	#print_full_event(\%E);

	#
	# Disply Events
	#
	for ($E{Event}) {

		/PeerStatus/ && do {
			print "\t". $E{Peer}. " has ". $E{PeerStatus}. "\n";
			return;
		};
		/ExtensionStatus/ && do {
			print "\t". $E{Exten}. " @ ". $E{Context}. " is ". Status2String($E{Status}). "\n";
			return;
		};
		/QueueMemberStatus/ && do {
			print "\t". $E{Location}. " is ". DeviceState2String($E{Status}). " in queue ". $E{Queue}. "\n";
			return;
		};
		/Registry/ && do {
			print "\t". $E{Channel}. "/". $E{Domain}. " has ". $E{Status}. "\n";
			return;
		};
		/Newchannel/ && do {
			$E{Status} = 'UNDEFINED' if (!defined($E{Status}));
			print "\t\tChan: ".  $E{CallerID}. ' <'.  $E{CallerIDName}.  ' on '.  $E{Channel}.
			' is '.  $E{Status}.  "\tState: ".  $E{State}.  "\n";
			return;
		};
		/Newcallerid/ && do {
			$E{Status} = 'UNDEFINED' if (!defined($E{Status}));
			print "\t\tCLID: ".  $E{CallerID}. " <".  $E{CallerIDName}. '> on '.  $E{Channel}. 
				"\tStatus: ". $E{Status}.  "\n";
			return;
		};
		/Dial/ && do {
			print "\t\t". "Dial\t". $E{CallerIDName}. ' <'. $E{CallerID}. '>'. 
				"\t". $E{Source}. "=>". $E{Destination}. "\t".
				$E{SrcUniqueID}. "=>". $E{DestUniqueID}. "\n";
print_full_event(\%E);
			return;
		};
		/Hangup/ && do {
			print "\t\tHangup ". $E{Channel}. "\t". $E{Cause}. " ". $E{'Cause-txt'}. "\n";
			return;
		};
		/Newexten/ && do {
			print "exten => ". $E{Context}. ", ". $E{Extension}. ", ". $E{Priority}. ", ".
				$E{Application}. " ( ". $E{AppData}. " )\n";
			return;
		};
		/Newstate/ && do {
			print "CLID: ". $E{CallerID}. " <". $E{CallerIDName}. "> on ". $E{Channel}. " is ". $E{State}. "\n";
			return;
		};
		/^UserEvent/ && do {
			my $event_name = $E{Event};
			$event_name =~ s/^UserEvent//;
			$E{Source} = 'UNDEFINED' if (!defined($E{Source}));
			print "[". $event_name. " ". $E{Channel} . "]\t". $E{Source}. "\n";
			return;
		};
		/Cdr/ && do {
			print "CDR: ". $E{Source}. ' => '. $E{Destination}. "\t\t".
				$E{Disposition}. '/'. $E{UserField}. "\tsec:". $E{Duration}. "\n";
			return;
		};
		/Shutdown/ && do {
			print "Shutdown: ". $E{Shutdown}. "\tRestart: ". $E{Restart}. "\n";
			return;
		};
	}


	#
	# ERROR, Unhandled event
	#
	print "[UNKNOW] Event: ". $E{Event}. " ==> ";
	print_full_event(\%E);
	return;
}


# Signal Handlers
#
sub safe_exit {
	my($sig) = @_;
	printlog('crit', "Caught a SIG$sig - shutting down");
	$manager->disconnect if $CW_connected;     
	exit;
}

