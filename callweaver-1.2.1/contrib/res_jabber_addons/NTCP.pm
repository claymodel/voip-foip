################################################################################
# NTCP (Network Telephony Control Protocol) *Jabber Transport Edition*
#
# (c) 2005 Anthony Minessale II
#  Anthony Minessale <anthmct@yahoo.com>
#
#  This program is free software, distributed under the terms of
#  the GNU General Public License
################################################################################

# This is a message class to represent NTCP Messages (hence the name)
package NTCP::Message;

# Constructor
sub new(;) {
  my $proto = shift;
  my $params = shift;
  my $class = ref($proto) || $proto;
  my $self = {PARAMS => $params};

  
  
  return bless($self, $class);
}

# Method to get the inner hash table so we dont cheat and touch 
# the guts via the public interface.
sub ashash($) {
  my $self = shift;
  $self->{PARAMS};
}

# Method to render a string suitable for dilevery.
sub astext($) {
  my $self = shift;

  my $buf = $self->{PARAMS}->{-command};

  if($self->{PARAMS}->{-command_args}) {
    $buf .= " " . $self->{PARAMS}->{-command_args};
  }

  $buf .= "\n";

  foreach(keys %{$self->{PARAMS}->{-headers}}) {
    $buf .= "${_}: $self->{PARAMS}->{-headers}->{$_}\n";
  }
  
  return $buf . "\n";
}

# NTCP::Client Class... Main client functionality.
package NTCP::Client;
use strict;
use Data::Dumper;
use IO::Select;
use Net::Jabber;
our $POINTERS = {};
our $PID = 1;


# Internal Net::Jabber Callback to NTCP Callback Gateway.
sub NTCPCallbacks {
  my $sid = shift;
  my $message = shift;
  my $to = $message->GetTo();
  my $obj = $POINTERS->{$to};

  if($obj) {
    $obj->process_ntcp_callbacks($message);
  }
}

# Helper function to convert a raw message into a NTCP::Message.
sub ParseMessage {
  my $message = shift;
  my $body = $message->GetBody();
  
  my($top, $rest) = split("\n", $body, 2);

  my ($command, $command_args) = split(" ", $top, 2);

  if($ENV{DEBUGNTCP}) {
    print STDERR "\n{$top}\n=\n$rest\n=\n";
  }

  my %heads = $rest =~ /(\S+)\:\s*([^\n]+)\n/sgm;
  my %lcheads;
  foreach (keys %heads) {
    $lcheads{lc $_} = $heads{$_};
  }

  my $msg = new NTCP::Message ({
				-command => $command,
				-command_args => $command_args,
				-headers => {%lcheads},
			       });
  
  return $msg;
}

# Helper used to dump messages to the terminal
sub PrintMessage {
  my $message = shift;
  my $type = $message->GetType();
  my $fromJID = $message->GetFrom("jid");
  my $to = $message->GetTo();
  my $from = $fromJID->GetUserID();
  my $resource = $fromJID->GetResource();
  my $subject = $message->GetSubject();
  my $body = $message->GetBody();

  print "\n";
  print "#################################\n";
  print "Message ($type)\n";
  print "From: $from ($resource)\n";
  print "Subject: $subject\n\n";
  print "$body\n\n";
  print "#################################\n\n";
}


# Internal Net::Jabber callback to just print any received messages.
sub MessagePrint {
  my $sid = shift;
  my $message = shift;
  PrintMessage $message;
}

# Internal Net::Jabber callback to queue message for delivery to an object.
sub MessageQueue {
  my $sid = shift;
  my $message = shift;
  my $to = $message->GetTo();
  my $obj = $POINTERS->{$to};

  if($obj) {
    my $m = ParseMessage $message;
    $obj->push_message($m);
  } elsif($ENV{DEBUGNTCP}) {
    print STDERR "no id $to\n";
  }
}

# Constructor, takes a hash of config options.
sub init(;) {
  my $proto = shift;
  my $params = shift;
  my $class = ref($proto) || $proto;
  my $self = {PARAMS => $params};
  $self->{PARAMS}->{-port} ||= "5222";
  $self->{PARAMS}->{-server} ||= "localhost";
  $self->{SELECT} = IO::Select->new();
  $self->{SELECT}->add(\*STDIN);
  $self->{JABBER_CALLBACKS} = {};
  $self->{QUEUE} = [];
  $self->{STATEOBJ} = {};
  my $obj = bless ($self, $class);
  if($obj->{PARAMS}->{-connect}) {
    $obj->connect();
  }
  $obj;
}

# Takes a NTCP::Message and sends it over XMPP
sub send_ntcp_message($$$) {
  my $self = shift;
  my $to = shift;
  my $msg_obj = shift;
  $self->send_jabber_message($to, $msg_obj->astext());
  
}

# Add a NTCP::Message to an object's queue.
sub push_message($$) {
  my $self = shift;
  my $message = shift;
  push @{$self->{QUEUE}}, $message;  
  
}

# Take the next NTCP::Message in line on the queue.
sub shift_message($$) {
  my $self = shift;
  my $message = shift @{$self->{QUEUE}};
  return $message;
}

# Produce a full Jabber ID from various pieces.
sub build_id($$) {
  my $self = shift;
  my $resource = shift || $self->{PARAMS}->{-resource};
  my $jid = $self->{PARAMS}->{-username} . '@' . $self->{PARAMS}->{-server} . '/' . $resource;
  return $jid;
}

# Connect to Jabber. (Called by constructor if (connect => 1))
sub connect($;$) {
  my $self = shift;
  if($ENV{DEBUGNTCP}) {
    print Dumper $self;
  }
  my $resource = shift || $self->{PARAMS}->{-resource};
  $self->{JABBER} = new Net::Jabber::Client();
  
  my $status = $self->{JABBER}->Connect( hostname => $self->{PARAMS}->{-server},
					 port => $self->{PARAMS}->{-port},
				       );
  if (!(defined($status))) {
    print STDERR "Connection Failed!\n";
    return 0;
  }
  
  my @result = $self->{JABBER}->AuthSend(username=> $self->{PARAMS}->{-username},
					 password=> $self->{PARAMS}->{-password},
					 resource=> $resource);
  if ($result[0] ne "ok") {
    print STDERR "ERROR: Authorization failed: $result[0] - $result[1]\n";
    return 0;
  }

  $self->{JID} = $self->build_id($resource);
  $self->set_jabber_callback("message", \&MessageQueue);

  $POINTERS->{$self->{JID}} = $self;
  $self->talkto($self->{PARAMS}->{-talkto});
  $self->talkto_resource($self->{PARAMS}->{-talkto_resource});
}

# Fetch roster and send presence to each of them.
sub rosterize() {
  my $self = shift;

  $self->{JABBER}->RosterGet();
  $self->{JABBER}->PresenceSend();
}

# Disconnect from Jabber
sub disconnect($) {
  my $self = shift;
  $self->{JABBER}->Disconnect;
  $POINTERS->{$self->{JID}} = undef;
}

# Install an NTCP callback function on NTCP Events.
sub set_ntcp_callback($$$) {
  my $self = shift;
  my $event = shift;
  my $ref = shift;

  $self->{NTCP_CALLBACKS}->{lc $event} = $ref;
  #$self->set_jabber_callback("message", \&NTCPCallbacks);  
}

# Install an Net::Jabber callback function on XMPP Events.
sub set_jabber_callback($$$) {
  my $self = shift;
  my $event = shift;
  my $ref = shift;
  $self->{JABBER_CALLBACKS}->{$event} = $ref;
  $self->{JABBER}->SetCallBacks(%{$self->{JABBER_CALLBACKS}});
}

# The FULL id of who i am talking to me@jabber.domain.com/resource
sub talkto_full($$) {
  my $self = shift;
  my $r = shift;
  return $self->{VARS}->{talkto} . "/" . ($r || $self->{VARS}->{resource});
}

# The main id of who i am talking to me@jabber.domain.com
sub talkto($$) {
  my $self = shift;
  my $id = shift;

  if($id) {
    $self->{VARS}->{talkto} = $id;
  }
  return $self->{VARS}->{talkto};
}

# The resource of who i am talking to me@jabber.domain.com
sub talkto_resource($$$) {
  my $self = shift;
  my $res = shift;
  my $abs = shift;

  if($res || $abs) {
    $self->{VARS}->{resource} = $res;
  }

  return $self->{VARS}->{resource};
}

# Manually send a Jabber Message.
sub send_jabber_message($$$) {
  my $self = shift;

  my $to = shift;
  my $msg_txt = shift;
  
  my $message = Net::XMPP::Message->new();
  $message->SetMessage( to => $to,
			body => $msg_txt,
			type => "chat"
		      );

  $self->{JABBER}->Send($message);

}

# Check for Jabber events $how is how many seconds to timeout on 
# if there are no events. (undef means forever.)
sub process($) {
  my $self = shift;
  my $how = shift;
  return $self->{JABBER}->Process($how);
}

# Debug function to start a mini cheezy terminal-based Jabber client.
sub testclient($$) {
  my $self = shift;
  $self->talkto(shift);
  $self->talkto_resource(shift);
  my $lastin;
  

  sub print_bar {
    my $talkto = $self->talkto_full();
    print "TALKING TO $talkto\n================================================================================\n";
  };

  $self->set_jabber_callback("message", \&MessagePrint);

  print_bar();
  while(defined($self->process(1))) { 
    if (my @ready = $self->{SELECT}->can_read(1)) {
      my $in;
      while (1) {
	my $i = <STDIN>;
	$in .= $i;
	if ($i eq "\n") {
	  last;
	}
      }

      if ($in =~ /^!/i) {
	$in = $lastin;
      } elsif ($in =~ /^\.exit|^\.quit|.bye/i) {
	last;
      } elsif ($in =~ /^\.talk\s*(.*)/) {
	my $arg = $1;
	if($arg =~ /\@/) {
	  my($a,$b) = split("/", $arg, 2);
	  $self->talkto($a);
	  $self->talkto_resource($b,1);	  
	} else {
	  $self->talkto_resource($arg);
	}
	
	print_bar();
	next;
      } 

      if ($in eq "\n") {
	next;
      }

      $self->send_jabber_message($self->talkto_full(), $in);
      $lastin = $in;
      print_bar();

    }
    while(my $message = $self->shift_message()) {
      print Dumper $message;
      print_bar();
    }
  }
}

# Send a digit to an defined IVR
sub feed_ivr($$$) {
  my $self = shift;
  my $ivr = shift;
  my $digit = shift;
 
  my $buf = undef;

  if($ivr) {
    
    push @{$ivr->{digits}}, $digit;
    if(@{$ivr->{digits}} > $ivr->{max}) {
      shift @{$ivr->{digits}};
    }
    $buf = join("", @{$ivr->{digits}});
    my $ref = $ivr->{table}->{$buf};
    if($ref) {
      eval {
	$ref->($self, $buf);
      };
      if($@) {
	print STDERR $@;
      }
    }
  }

  return $buf;
}

# Delete an IVR.
sub del_ivr($$) {
  my $self = shift;
  my $name = shift;

  delete $self->{IVR}->{$name};
}

# Find an IVR.
sub get_ivr($$) {
  my $self = shift;
  my $name = shift;
  return $self->{IVR}->{$name};
}

# Install an IVR
sub add_ivr($$$) {
  my $self = shift;
  my $name = shift;
  my $table = shift;
  my $max = 0;

  foreach my $k (keys %{$table}) {
    if(length($k) > $max) {
      $max = $k;
    }
  }

  $self->{IVR}->{$name} = {
			   max => $max,
			   digits => [],
			   table => $table
			  };
  
}

# Examine an NTCP message and react according to configuration.
sub process_ntcp_callbacks($) {
  my $self = shift;
  my $message = shift;

  my $hash = $message->ashash;
  my $event = lc $hash->{-command_args};
  my $ref;

  # send digits to IVRs
  if ($event eq "dtmf") {
    my $hash = $message->ashash();
    my $digit = $hash->{-headers}->{digit};

    foreach my $ivr_name (keys %{$self->{IVR}}) {
      $self->feed_ivr($self->{IVR}->{$ivr_name}, $digit);
    }
    
  }

  # see if there is a global handler

  $ref = $self->{NTCP_CALLBACKS}->{"allevents"};
  if($ref) {
    eval {
      $ref->($self, $message);
    };
    if($@) {
      print STDERR $@;
    }
  }

  # otherwise try the mapped callbacks

  $ref = $self->{NTCP_CALLBACKS}->{$event} || $self->{NTCP_CALLBACKS}->{"catchall"};

  if($ref) {
    eval {
      $ref->($self, $message);
    };
    if($@) {
      print STDERR $@;
    }
  }

}

# Fork a new process and return a new connection in the child.
sub fork($$) {
  my $self = shift;
  my $to = shift;
  my $args;
  my $pid;

  foreach my $k (keys %{$self->{PARAMS}}) {
    $args->{$k} = $self->{PARAMS}->{$k};
  }
  $args->{-resource} .= "_" . $PID++;
  my $jid = $args->{-username} . '@' . $args->{-server} . '/' . $args->{-resource};
  $args->{-talkto_resource} = $to;
  
  if($pid = fork) {
    $self->setmaster($jid, $to);  
    return undef;
  }

  $NTCP::POINTERS = {};
  return init NTCP::Client $args;

}

# Fetch a ref to the objects extra state hash
# you can stick whatever you want in here.
sub state_obj($$) {
  my $self = shift;
  my $obj = shift;
  if($obj) {
    $self->{STATEOBJ} = $obj;
  }
  return $self->{STATEOBJ};
}

# Loop 1 time through the regular processing engine.
sub run_once($;$) {
  my $self = shift;
  my $sec = shift;
  if(defined($self->process($sec))) {
    while(my $message = $self->shift_message()) {
      $self->process_ntcp_callbacks($message);
    }
    return 1;
  }
  return 0;
}

# Loop forever through the regular processing engine.
sub run($;$) {
  my $self = shift;
  my $sec = shift;
  while($self->run_once($sec)){}
}

# The rest of the methods are interfaces to regular NTCP commands.
################################################################################


# Request a new Channel that will report to you when it's ready or send you a 
# CALL END if it fails.
sub call($$) {
  my $self = shift;
  my $params = shift;
  
  $params->{resource} = $$;
  my $msg = new NTCP::Message ({
				-command => "CALL",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
}

# Tell the call to indicate answer. (go offhook)
sub answer($$) {
  my $self = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "ANSWER",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
}

# Tell the call to end
sub hangup($$) {
  my $self = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "HANGUP",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
  
}

# Tell the call to steam a local audio file.
sub stream($$$) {
  my $self = shift;
  my $file = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "STREAM",
				-command_args => $file,
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
  

}

# Tell the call to abort any streaming audio (will trigger a STOPSTREAM event.)
sub stopstream($$$) {
  my $self = shift;
  my $file = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "STOPSTREAM",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
  

}

# Tell the call to change it's master to a new controller entity (jabber id)
sub setmaster($$$) {
  my $self = shift;
  my $jid = shift;
  my $resource = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "SETMASTER",
				-command_args => $jid,
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full($resource), $msg);
}

# Tell the call to pause it's forwarding of media. (mute)
sub pauseforwardmedia($$$) {
  my $self = shift;
  my $file = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "pauseforwardmedia",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
}

# Tell the call to resume it's forwarding of media. (unmute)
sub resumeforwardmedia($$$) {
  my $self = shift;
  my $file = shift;
  my $params = shift;
  my $msg = new NTCP::Message ({
				-command => "resumeforwardmedia",
				-headers => $params,
			       });

  $self->send_ntcp_message($self->talkto_full(), $msg);
}


# More to follow......
# You can also formulate them yourself as you can see from these examples.


1;
