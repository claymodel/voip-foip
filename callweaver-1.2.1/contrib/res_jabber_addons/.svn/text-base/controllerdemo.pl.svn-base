#!/usr/local/bin/perl
################################################################################
# controllerdemo.pl -- simple IVR controller demo
#
# (c) 2005 Anthony Minessale II
#  Anthony Minessale <anthmct@yahoo.com>
#
#  This program is free software, distributed under the terms of
#  the GNU General Public License
################################################################################

use NTCP;
use Data::Dumper;
use strict;

#Helper Code
#######################################################################################
# Get the terminal size
our ($ROWS,$COLS)  = split(" ", `stty size`);

# Lazy helper to print the pid so we can tell which process is spewing info.
sub doprint(@) {
  foreach(@_) {
    my $line = $_;
    $line =~ s/\n/\n\[$$\] /g;
    print $line =~ $$ ? $line : "[$$] $line";
  }
}

# Try to render the header info into a concise printable format.
sub hash_headers($;$) {
  my $hash = shift;
  my $maxlen = shift || 80;

  my $ret;
  my $len = 0;

  foreach(sort keys %{$hash}) {
    my $add = "${_}($hash->{$_})  ";
    my $l = length($add) + 1;

    if(($len + $l) > $maxlen) {
      $len = 0;
      $ret .= "\n";
    } else {
      $len += $l;
    }

    $ret .= $add;

  }
  return $ret;
}


#######################################################################################
# Simple IVR callbacks for a demo ivr
# Each of these functions should accept 2 arguements:
# 1) An NTCP connection object
# 2) A string of the digits dialed to trigger the function.
#######################################################################################

# This one plays a sound file
sub play_congrats {
  my $ntcp = shift;
  my $dialed = shift;
  
  $ntcp->stream("demo-congrats");
}

# This one mutes/unmutes a call (if you are talking to someone).
sub toggle_mute {
  my $ntcp = shift;
  my $dialed = shift;
  my $state = $ntcp->state_obj();

  return if(!$state->{incall});

  if($state->{mute}) {
    $state->{mute} = 0;
    doprint "UNMUTED\n";
    $ntcp->resumeforwardmedia();
  } else {
    $ntcp->pauseforwardmedia();
    doprint "MUTED\n";
    $state->{mute} = 1;
  }
}

# This one makes any file stop playing.
sub stop_stream {
  my $ntcp = shift;
  my $dialed = shift;

  $ntcp->stopstream();
}

# This one creates another channel to the 42 conf then
# calls for a media bridge. The BridgeTo tells it 
# to immediatly contact or other channel and negotiate
# a raw udp audio connection.

sub call_conf {
  my $ntcp = shift;
  my $dialed = shift;
  doprint "CALLING CONF\n";
  $ntcp->call({
	       Type => "IAX2",
	       Data => "guest\@66.250.68.194/4242",
	       CallingPartyName => "NextGen Demo",
	       CallingPartyNumber => "4242424242",
	       CalledNumber => "4242",
	       BridgeTo => $ntcp->talkto_full()
	       });
  my $state = $ntcp->state_obj();
  $state->{incall}++;
}

# This one will cause the channel to hangup which in turn will
# Trigger an "END CALL" event (see EVENT CALLBACKS) and kill our process.
sub hangup_call {
  my $ntcp = shift;
  my $dialed = shift;
  
  $ntcp->hangup();
}

#######################################################################################
# EVENT CALLBACKS
# These functions are called when events are received
# The format is:
# 1) An NTCP::Client connection object
# 2) An NTCP::Message message object of the Event
#######################################################################################


# This function is mapped to every event on the special ALLEVENTS event so it can print debug info
# ALLEVENTS indicates it will be called no matter what on ALL EVENTS regardless of mapping.
sub on_all {
  my $ntcp = shift;
  my $msg = shift;
  my $hash = $msg->ashash();

  doprint "EVENT [" . $hash->{-command_args} . "]\n" . hash_headers($hash->{-headers}, $COLS) . "\n\n";

}

# This is so we can close down our forked process.
sub on_hangup {
  my $ntcp = shift;
  $ntcp->disconnect;
  exit;
};


# This is the one used by the main connection that is mapped to inbound calls.
# The goal here is:
# 1) To fork the application into a new child process.
# 2) Create another NTCP connection in the child
# 3) Instruct the other end to transfer communication to the child.
# 4) Handle the rest of the call's needs in the child.
# 5) Send the parent back to waiting for more events.
#
sub on_inbound {
  my $ntcp = shift;
  my $msg = shift;
  my $hash = $msg->ashash();
  my $pid;
  my $child_ntcp;

  # Here we will fork a new process with it's own id/connection
  # The new id will take control of the call so we can go back
  # to waiting for more calls.

  return if(!($child_ntcp = $ntcp->fork($hash->{-headers}->{from})));

  # If we made it to this point we are a child process controlling a call.
  # This is a new connection so if we care about any events we need to map
  # a handler for them.

  # Event handler to print notification of any and all events.
  $child_ntcp->set_ntcp_callback("allevents", \&on_all);

  # Event handler for when a call ends.
  $child_ntcp->set_ntcp_callback("END CALL", \&on_hangup);

  # We can do whatever we want here so let's try a simple IVR.
  # What you need to do is to map digit strings to various 
  # functions to execute whenever that paticular string is dialed.
  # Once you create the mapping, register the ivr object and the 
  # engine will do the rest.
  #
  # 001 = play a congrats file
  # 002 = stop playing any files
  # 003 = call 42 conf
  # 004 = toggle mute
  # 005 = hangup

  # Make a map of digitstring to callback function pointers.
  my $ivr_demo1 = {
		   "001" => \&play_congrats,
		   "002" => \&stop_stream,
		   "003" => \&call_conf,
		   "004" => \&toggle_mute,
		   "005" => \&hangup_call
		  };


  # Install the IVR 
  $child_ntcp->add_ivr("demo1", $ivr_demo1);
  
  # Let our buddies see us
  $child_ntcp->rosterize();

  # OK, answer the call
  $child_ntcp->answer();

  # Now, play a sound so you know the demo is up.
  $child_ntcp->stream("beep");

  # Fire up the engine.  We leave it in the hands of the callbacks from here.
  $child_ntcp->run;
}


#######################################################################################
# Main Program
# Like all good main programs, we aim to be a small wrapper at the ground 
# floor of the interface. 
#######################################################################################

# Create the Master Connection. (This should be the jabber id your endpoints are pointed at.)
my $NTCP = init NTCP::Client ({
			       -server => "jabber.mydomain.com",
			       -username => "res_jabber",
			       -password => "mypass",
			       -resource => "controller",
			       -connect => "yes",
			       -talkto => "res_jabber\@jabber.mydomain.com",
			       -talkto_resource => "master_thread"
			      });

# Send presence info.
$NTCP->rosterize();

# Add a callback for when a new call is detected so we can fork.
$NTCP->set_ntcp_callback("INCOMING CALL", \&on_inbound);

# Add a callback to print notification of any events for debugging.
$NTCP->set_ntcp_callback("allevents", \&on_all);

# Here we go!
$NTCP->run;

__END__
