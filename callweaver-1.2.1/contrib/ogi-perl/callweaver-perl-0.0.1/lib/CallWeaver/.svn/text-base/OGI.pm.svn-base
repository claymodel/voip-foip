package CallWeaver::OGI;

require 5.004;

use strict;
use warnings;
use CallWeaver;

use vars qw(@ISA);
@ISA = ( 'CallWeaver' );

=head1 NAME

CallWeaver::OGI - Simple CallWeaver.org Gateway Interface Class

=head1 SYNOPSIS

use CallWeaver::OGI;

$OGI = new CallWeaver::OGI;

# pull OGI variables into %input

	%input = $OGI->ReadParse();   

# say the number 1984

	$OGI->say_number(1984);

=head1 DESCRIPTION

This module should make it easier to write scripts that interact with the
callweaver.org open source pbx via OGI (callweaver.org gateway interface)

=head1 MODULE COMMANDS

=over 4

=cut

sub new {
	my ($class, %args) = @_;
	my $self = {};
	$self->{'callback'} = undef;
	$self->{'status'} = undef;
	$self->{'lastresponse'} = undef;
	$self->{'lastresult'} = undef;
	$self->{'hungup'} = 0;
	$self->{'debug'} = 0;
	$self->{'env'} = undef;
	bless $self, ref $class || $class;
	return $self;
}

sub ReadParse {
	my ($self, $fh) = @_;

	if (!$self->_env) {
		return $self->_ReallyReadParse($fh);
	}

	return %{$self->_env};
}

sub _ReallyReadParse {
	my ($self, $fh) = @_;

	my %input = ();

	$fh = \*STDIN if (!$fh);

	while (<$fh>) {
		chomp;
		last unless length($_);
		if (/^ogi_(\w+)\:\s+(.*)$/) {
			$input{$1} = $2;
		}
	}
	

	if ($self->_debug > 0) {
		print STDERR "OGI Environment Dump:\n";
		foreach my $i (sort keys %input) {
			print STDERR " -- $i = $input{$i}\n";
		}
	}

	$self->_env(%input);
	return %input;
}

=item $OGI->setcallback($funcref)

Set function to execute when call is hungup or function returns error.

Example: $OGI->setcallback(\&examplecallback);

=cut 

sub setcallback {
	my ($self, $function) = @_;

	if (defined($function) && ref($function) eq 'CODE') {
		$self->{'callback'} = $function;
	} 
}

sub callback {
	my ($self, $result) = @_;

	if (defined($self->{'callback'}) && ref($self->{'callback'}) eq 'CODE') {
		&{$self->{'callback'}}($result);
	}
}

sub execute {
	my ($self, $command) = @_;

	# Check if ReadParse has been run already.  If not do it now
	# to save end user confusion
	if (!$self->_env) {
		$self->ReadParse();
	}
	$self->_execcommand($command);
	my $res = $self->_readresponse();
	my $ret = $self->_checkresult($res);

	if (defined($ret) && $ret eq '-1' && !$self->_hungup()) {
		$self->_hungup(1);
		$self->callback($ret);
	}

	return $ret;
}

sub _execcommand {
	my ($self, $command, $fh) = @_;

	$fh = \*STDOUT if (!$fh);

	select ((select ($fh), $| = 1)[0]);

	return -1 if (!defined($command));

	print STDERR "_execcommand: '$command'\n" if ($self->_debug>3);

	return print $fh "$command\n";
}

sub _readresponse {
	my ($self, $fh) = @_;

	my $response = undef;
	$fh = \*STDIN if (!$fh);
	$response = <$fh> || return '200 result=-1 (noresponse)';
	chomp($response);
	return $response;
}

sub _checkresult {
	my ($self, $response) = @_;

	return undef if (!defined($response));
	my $result = undef;

	$self->_lastresponse($response);
	if ($response =~ /^200/) {
		if ($response =~ /result=(-?[\d*#]+)/) {
			$result = $self->{'lastresult'} = $1;
		}
	} elsif ($response =~ /\(noresponse\)/) {
		$self->_status('noresponse');
	} else {
		print STDERR "Unexpected result '" . defined($response) ? $response : '' . "'\n" if ($self->_debug>0);
	}
	print STDERR "_checkresult(" . defined($response) ? $response : '' . ") = " . defined($result) ? $result : '' . "\n" if ($self->_debug>3);

	return $result;				
}

sub _status {
	my ($self, $status) = @_;

	if (defined($status)) {
		$self->{'status'} = $status;
	} else {
		return $self->{'status'};
	}
}

sub _lastresponse {
	my ($self, $response) = @_;

	if (defined($response)) {
		$self->{'lastresponse'} = $response;
	} else {
		return $self->{'lastresponse'};
	}
}

sub _lastresult {
	my ($self, $result) = @_;

	if (defined($result)) {
		$self->{'lastresult'} = $result;
	} else {
		return $self->{'lastresult'};
	}
}

sub _hungup {
	my ($self, $value) = @_;

	if (defined($value)) {
		$self->{'hungup'} = $value;
	} else {
		return $self->{'hungup'};
	}
}

sub _debug {
	my ($self, $value) = @_;

	if (defined($value)) {
		$self->{'debug'} = $value;
	} else {
		return $self->{'debug'};
	}
}

sub _env {
	my ($self, %env) = @_;

	if (%env) {
		$self->{'env'} = \%env;
	} else {
		return $self->{'env'};
	}
}

=head1 OGI COMMANDS

=item $OGI->answer()

Executes OGI Command "ANSWER"

Answers channel if not already in answer state

Example: $OGI->answer();

Returns: -1 on channel failure, or
0 if successful

=cut

sub answer {
	my ($self) = @_;

	return $self->execute('ANSWER');
}

=item $OGI->channel_status([$channel])

Executes OGI Command "CHANNEL STATUS $channel"

Returns the status of the specified channel.  If no channel name is given the
returns the status of the current channel.

Example: $OGI->channel_status();

Returns: -1 Channel hungup or error
         0 Channel is down and available
         1 Channel is down, but reserved
         2 Channel is off hook
         3 Digits (or equivalent) have been dialed
         4 Line is ringing
         5 Remote end is ringing
         6 Line is up
         7 Line is busy

=cut

sub channel_status {
	my ($self, $channel) = @_;

	return $self->execute("CHANNEL STATUS $channel");
}

=item $OGI->control_stream_file($filename, $digits [, $skipms [, $ffchar [, $rewchar [, $pausechar]]]])

Executes OGI Command "CONTROL STREAM FILE $filename $digits [$skipms [$ffchar [$rewchar [$pausechar]]]]"

Send the given file, allowing playback to be controled by the given digits, if
any. Use double quotes for the digits if you wish none to be permitted.
Remember, the file extension must not be included in the filename.

Note: ffchar and rewchar default to * and # respectively.

Example: $OGI->control_stream_file('status', 'authorized');

Returns: -1 on error or hangup;
         0 if playback completes without a digit being pressed;
         the ASCII numerical value of the digit of one was pressed.

=cut

sub control_stream_file {
	my ($self, $filename, $digits, $skipms, $ffchar, $rewchar, $pausechar) = @_;

	return -1 if (!defined($filename));
	$digits = '""' if (!defined($digits));
	return $self->execute("CONTROL STREAM FILE $filename $digits $skipms $ffchar $rewchar $pausechar");
}

=item $OGI->database_del($family, $key)

Executes OGI Command "DATABASE DEL $family $key"

Removes database entry <family>/<key>

Example: $OGI->database_del('test', 'status');

Returns: 1 on success, 0 otherwise

=cut

sub database_del {
	my ($self, $family, $key) = @_;

	return $self->execute("DATABASE DEL $family $key");
}

=item $OGI->database_deltree($family, $key)

Executes OGI Command "DATABASE DELTREE $family $key"

Deletes a family or specific keytree within a family in the CallWeaver.org database

Example: $OGI->database_deltree('test', 'status'); 
Example: $OGI->database_deltree('test');

Returns: 1 on success, 0 otherwise

=cut

sub database_deltree {
	my ($self, $family, $key) = @_;

	return $self->execute("DATABASE DELTREE $family $key");
}

=item $OGI->database_get($family, $key)

Executes OGI Command "DATABASE GET $family $key"

Example: $var = $OGI->database_get('test', 'status');

Returns: The value of the variable, or undef if variable does not exist

=cut

sub database_get {
	my ($self, $family, $key) = @_;

	my $result = undef;

	if ($self->execute("DATABASE GET $family $key")) {
		my $tempresult = $self->_lastresponse();
		if ($tempresult =~ /\((.*)\)/) {
			$result = $1;
		}
	}
	return $result;
}

=item $OGI->database_put($family, $key, $value)

Executes OGI Command "DATABASE PUT $family $key $value"

Set/modifes database entry <family>/<key> to <value>

Example: $OGI->database_put('test', 'status', 'authorized');

Returns: 1 on success, 0 otherwise

=cut

sub database_put {
	my ($self, $family, $key, $value) = @_;

	return $self->execute("DATABASE PUT $family $key $value");
}

=item $OGI->exec($app, $options)

Executes OGI Command "EXEC $app $options"

The most powerful OGI command.  Executes the given application passing the given options.

Example: $OGI->exec('Dial', 'Zap/g2/8005551212');

Returns: -2 on failure to find application, or
whatever the given application returns

=cut

sub exec {
	my ($self, $app, $options) = @_;
	return -1 if (!defined($app));
	$options = '""' if (!defined($options));
	return $self->execute("EXEC $app $options");
}

=item $OGI->get_data($filename, $timeout, $maxdigits)

Executes OGI Command "GET DATA $filename $timeout $maxdigits"

Streams $filename and returns when $maxdigits have been received or
when $timeout has been reached.  Timeout is specified in ms

Example: $OGI->get_data('demo-welcome', 15000, 5);

=cut

sub get_data {
	my ($self, $filename, $timeout, $maxdigits) = @_;

	return -1 if (!defined($filename));
	return $self->execute("GET DATA $filename $timeout $maxdigits");
}

=item $OGI->get_full_variable($variable [, $channel])

Executes OGI Command "GET FULL VARIABLE $variablename $channel"

Similar to get_variable, but additionally understands
complex variable names and builtin variables.  If $channel is not set, uses the
current channel.

Example: $OGI->get_full_variable('status', 'SIP/4382');

Returns: The value of the variable, or undef if variable does not exist

=cut

sub get_full_variable {
	my ($self, $variable, $channel) = @_;

	$channel = '' if (!defined($channel));

	my $result = undef;

	if ($self->execute("GET FULL VARIABLE $variable $channel")) {
		my $tempresult = $self->_lastresponse();
		if ($tempresult =~ /\((.*)\)/) {
			$result = $1;
		}
	}
	return $result;
}

=item $OGI->get_option($filename, $digits [, $timeout])

Executes OGI Command "GET OPTION $filename $digits $timeout"

Behaves similar to STREAM FILE but used with a timeout option.

Streams $filename and returns when $digits is pressed or when $timeout has been
reached.  Timeout is specified in ms.  If $timeout is not specified, the command
will only terminate on the $digits set.

Example: $OGI->get_option('demo-welcome', '#', 15000);

=cut

sub get_option {
	my ($self, $filename, $digits, $timeout) = @_;

	$timeout = 0 if (!defined($timeout)); 

	return -1 if (!defined($filename));
	return $self->execute("GET OPTION $filename $digits $timeout");
}

=item $OGI->get_variable($variable)

Executes OGI Command "GET VARIABLE $variablename"

Gets the channel variable <variablename>

Example: $OGI->get_variable('status');

Returns: The value of the variable, or undef if variable does not exist

=cut

sub get_variable {
	my ($self, $variable) = @_;

	my $result = undef;

	if ($self->execute("GET VARIABLE $variable")) {
		my $tempresult = $self->_lastresponse();
		if ($tempresult =~ /\((.*)\)/) {
			$result = $1;
		}
	}
	return $result;
}

=item $OGI->hangup($channel)

Executes OGI Command "HANGUP $channel"

Hangs up the passed $channel, or the current channel if $channel is not passed.
It is left to the OGI script to exit properly, otherwise you could end up with zombies.

Example: $OGI->hangup();

Returns: Always returns 1

=cut

sub hangup {
	my ($self, $channel) = @_;

	if ($channel) {
		return $self->execute("HANGUP $channel");
	} else {
		return $self->execute("HANGUP");
	}
}

=item $OGI->noop()

Executes OGI Command "NOOP"

Does absolutely nothing except pop up a log message.  
 Useful for outputting debugging information to the CallWeaver.org console.

Example: $OGI->noop("Test Message");

Returns: -1 on hangup or error, 0 otherwise

=cut

sub noop {
	my ($self) = @_;

	return $self->execute("NOOP");
}

=item $OGI->receive_char($timeout)

Executes OGI Command "RECEIVE CHAR $timeout"

Receives a character of text on a channel. Specify timeout to be the maximum
time to wait for input in milliseconds, or 0 for infinite. Most channels do not
support the reception of text. 

Example: $OGI->receive_char(3000);

Returns: Returns the decimal value of the character if one
is received, or 0 if the channel does not support text reception.  Returns -1
only on error/hangup.

=cut

sub receive_char {
	my ($self, $timeout) = @_;

#wait forever if timeout is not set. is this the prefered default?
	$timeout = 0 if (!defined($timeout));
	return $self->execute("RECEIVE CHAR $timeout");
}

=item $OGI->receive_text($timeout)

Executes OGI Command "RECEIVE TEXT $timeout"

Receives a string of text on a channel. Specify timeout to be the maximum time
to wait for input in milliseconds, or 0 for infinite. Most channels do not
support the reception of text. 

Example: $OGI->receive_text(3000);

Returns: Returns the string of text if received, or -1 for failure, error or hangup.

=cut

sub receive_text {
	my ($self, $timeout) = @_;

#wait forever if timeout is not set. is this the prefered default?
	$timeout = 0 if (!defined($timeout));
	return $self->execute("RECEIVE TEXT $timeout");
}

=item $OGI->record_file($filename, $format, $digits, $timeout, $beep, $offset, $beep, $silence)

Executes OGI Command "RECORD FILE $filename $format $digits $timeout [$offset [$beep [s=$silence]]]"

Record to a file until $digits are received as dtmf.
The $format will specify what kind of file will be recorded.
The $timeout is the maximum record time in milliseconds, or -1 for no timeout.

$offset samples is optional, and if provided will seek to the offset without
exceeding the end of the file.

$silence is the number of seconds of silence allowed before the function
returns despite the lack of dtmf digits or reaching timeout.

Example: $OGI->record_file('foo', 'wav', '#', '5000', '0', 1, '2');

Returns: 1 on success, -1 on hangup or error.

=cut

sub record_file {
	my ($self, $filename, $format, $digits, $timeout, $offset, $beep, $silence) = @_;

	my $extra = '';

	return -1 if (!defined($filename));
	$digits = '""' if (!defined($digits));
	$extra .= $offset if (defined($offset));
	$extra .= ' ' . $beep if (defined($beep));
	$extra .= ' s=' . $silence if (defined($silence));

	return $self->execute("RECORD FILE $filename $format $digits $timeout $extra");
}

=item $OGI->say_alpha($string, $digits)

Executes OGI Command "SAY ALPHA $string $digits"

Say a given character string, returning early if any of the given DTMF $digits
are received on the channel. 

Returns 
Example: $OGI->say_alpha('Joe Smith', '#');

Returns: 0 if playback completes without a digit being pressed; 
         the ASCII numerical value of the digit if one was pressed;
         -1 on error/hangup.

=cut

sub say_alpha {
	my ($self, $string, $digits) = @_;

	$digits = '""' if (!defined($digits));

	return -1 if (!defined($string));
	return $self->execute("SAY ALPHA $string $digits");
}

=item $OGI->say_date($time [, $digits])

=cut

=item $OGI->say_time($time [, $digits])

=cut

=item $OGI->say_datetime($time [, $digits [, $format [, $timezone]]])

Executes OGI Command "SAY DATE $number $digits"
Executes OGI Command "SAY TIME $number $digits"
Executes OGI Command "SAY DATETIME $number $digits $format $timezone"

Say a given date or time, returning early if any of the optional DTMF $digits are
received on the channel.  $time is number of seconds elapsed since 00:00:00 on
January 1, 1970, Coordinated Universal Time (UTC), commonly known as
"unixtime." 

For say_datetime, $format is the format the time should be said in; see
voicemail.conf (defaults to "ABdY 'digits/at' IMp").  Acceptable values for
$timezone can be found in /usr/share/zoneinfo.  Defaults to machine default.

Example: $OGI->say_date('100000000');
         $OGI->say_time('100000000', '#'); 
         $OGI->say_datetime('100000000', '#', 'ABdY IMp', 'EDT');

Returns: -1 on error or hangup;
         0 if playback completes without a digit being pressed;
         the ASCII numerical value of the digit of one was pressed.

=cut

sub say_datetime_all {
	my ($self, $type, $time, $digits, $format, $timezone) = @_;

	my $ret = 0;
	$digits = '""' if (!defined($digits));
	return -1 if (!defined($time));

	if ($type == 'date') {
		$ret = $self->execute("SAY DATE $time $digits");
	} elsif ($type == 'time') {
		$ret = $self->execute("SAY TIME $time $digits");
	} elsif ($type == 'datetime') {
		$ret = $self->execute("SAY DATETIME $time $digits $format $timezone");
	} else {
		$ret = -1;
	}

	return $ret;
}

sub say_date {
	my ($self, $time, $digits) = @_;

	return $self->say_datetime_all('date', $time, $digits);
}

sub say_time {
	my ($self, $time, $digits) = @_;

	return $self->say_datetime_all('time', $time, $digits);
}

sub say_datetime {
	my ($self, $time, $digits, $format, $timezone) = @_;

	return $self->say_datetime_all('datetime', $time, $digits, $format, $timezone);
}

=item $OGI->say_digits($number, $digits)

Executes OGI Command "SAY DIGITS $number $digits"

Says the given digit string $number, returning early if any of the $digits are received.

Example: $OGI->say_digits('8675309');

Returns: -1 on error or hangup,
0 if playback completes without a digit being pressed, 
or the ASCII numerical value of the digit of one was pressed.

=cut

sub say_digits {
        my ($self, $number, $digits) = @_;

	$digits = '""' if (!defined($digits));

	return -1 if (!defined($number));
	$number =~ s/\D//g;
	return $self->execute("SAY DIGITS $number $digits");
}

=item $OGI->say_number($number, $digits)

Executes OGI Command "SAY NUMBER $number $digits"

Says the given $number, returning early if any of the $digits are received.

Example: $OGI->say_number('98765');

Returns: -1 on error or hangup,
0 if playback completes without a digit being pressed, 
or the ASCII numerical value of the digit of one was pressed.

=cut

sub say_number {
	my ($self, $number, $digits) = @_;

	$digits = '""' if (!defined($digits));

	return -1 if (!defined($number));
	$number =~ s/\D//g;
	return $self->execute("SAY NUMBER $number $digits");
}

=item $OGI->say_phonetic($string, $digits)

Executes OGI Command "SAY PHONETIC $string $digits"

Say a given character string with phonetics, returning early if any of the
given DTMF digits are received on the channel.

Example: $OGI->say_phonetic('Joe Smith', '#');

Returns: 0 if playback completes without a digit being pressed; 
         the ASCII numerical value of the digit if one was pressed;
         -1 on error/hangup.

=cut

sub say_phonetic {
        my ($self, $string, $digits) = @_;

	$digits = '""' if (!defined($digits));

	return -1 if (!defined($string));
	return $self->execute("SAY PHONETIC $string $digits");
}

=item $OGI->send_image($image)

Executes OGI Command "SEND IMAGE $image

Sends the given image on a channel.  Most channels do not support the transmission of images.

Example: $OGI->send_image('image.png');

Returns: -1 on error or hangup,
0 if the image was sent or if the channel does not support image transmission.

=cut

sub send_image {
	my ($self, $image) = @_;

	return -1 if (!defined($image));

	return $self->execute("SEND IMAGE $image");
}

=item $OGI->send_text($text)

Executes OGI Command "SEND TEXT "$text"

Sends the given text on a channel.  Most channels do not support the transmission of text.

Example: $OGI->send_text('You've got mail!');

Returns: -1 on error or hangup,
0 if the text was sent or if the channel does not support text transmission.

=cut

sub send_text {
	my ($self, $text) = @_;

	return 0 if (!defined($text));
	return $self->execute("SEND TEXT \"$text\"");
}

=item $OGI->set_autohangup($time)

Executes OGI Command "SET AUTOHANGUP $time"

Cause the channel to automatically hangup at <time> seconds in the future.
Of course it can be hungup before then as well.
Setting to 0 will cause the autohangup feature to be disabled on this channel.

Example: $OGI->set_autohangup(60);

Returns: Always returns 1

=cut

sub set_autohangup {
	my ($self, $time) = @_;

	$time = 0 if (!defined($time));
	return $self->execute("SET AUTOHANGUP $time");
}

=item $OGI->set_callerid($number)

Executes OGI Command "SET CALLERID $number"

Changes the callerid of the current channel to <number>

Example: $OGI->set_callerid('9995551212');

Returns: Always returns 1

=cut

sub set_callerid {
	my ($self, $number) = @_;

	return if (!defined($number));
	return $self->execute("SET CALLERID $number");
}

=item $OGI->set_context($context)

Executes OGI Command "SET CONTEXT $context"

Changes the context for continuation upon exiting the ogi application

Example: $OGI->set_context('dialout');

Returns: Always returns 0

=cut

sub set_context {
	my ($self, $context) = @_;

	return -1 if (!defined($context));
	return $self->execute("SET CONTEXT $context");
}

=item $OGI->set_extension($extension)

Executes OGI Command "SET EXTENSION $extension"

Changes the extension for continuation upon exiting the ogi application

Example: $OGI->set_extension('7');

Returns: Always returns 0

=cut

sub set_extension {
	my ($self, $extension) = @_;

	return -1 if (!defined($extension));
	return $self->execute("SET EXTENSION $extension");
}

=item $OGI->set_music($mode [, $class])

Executes OGI Command "SET MUSIC $mode $class"

Enables/Disables the music on hold generator.  If $class is not specified, then
the default music on hold class will be used.  $mode must be "on" or "off".

Example: $OGI->set_music("on", "happy");
         $OGI->set_music("off");

Returns: -1 on hangup or error, 0 otherwise.

=cut

sub set_music {
	my ($self, $mode, $class) = @_;

	return $self->execute("SET MUSIC $mode $class");
}

=item $OGI->set_priority($priority)

Executes OGI Command "SET PRIORITY $priority"

Changes the priority for continuation upon exiting the ogi application

Example: $OGI->set_priority(1);

Returns: Always returns 0

=cut

sub set_priority {
	my ($self, $priority) = @_;

	return -1 if (!defined($priority));
	return $self->execute("SET PRIORITY $priority");
}

=item $OGI->set_variable($variable, $value)

Executes OGI Command "SET VARIABLE $variable $value"

Sets the channel variable <variablename> to <value>

Example: $OGI->set_variable('status', 'authorized');

Returns: Always returns 1

=cut

sub set_variable {
	my ($self, $variable, $value) = @_;

	return $self->execute("SET VARIABLE $variable \"$value\"");
}

=item $OGI->stream_file($filename, $digits)

Executes OGI Command "STREAM FILE $filename $digits"

This command instructs CallWeaver.org to play the given sound file and listen for the given dtmf digits. The
fileextension must not be used in the filename because CallWeaver.org will find the most appropriate file
type.

Example: $OGI->stream_file('demo-echotest', '0123');

Returns: -1 on error or hangup,
0 if playback completes without a digit being pressed,
or the ASCII numerical value of the digit if a digit was pressed

=cut

sub stream_file {
	my ($self, $filename, $digits) = @_;

	$digits = '""' if (!defined($digits));

	return -1 if (!defined($filename));
	return $self->execute("STREAM FILE $filename $digits");
}

=item $OGI->tdd_mode($mode)

Executes OGI Command "TDD MODE <on|off>"

Enable/Disable TDD transmission/reception on a channel. 

Example: $OGI->tdd_mode('on');

Returns: Returns 1 if successful, or 0 if channel is not TDD-capable.

=cut

sub tdd_mode {
	my ($self, $mode) = @_;

	return 0 if (!defined($mode));
	return $self->execute("TDD MODE $mode");
}

=item $OGI->verbose($message, $level)

Executes OGI Command "VERBOSE $message $level"

Logs $message with verboselevel $level

Example: $OGI->verbose("System Crashed\n", 1);

Returns: Always returns 1

=cut

sub verbose {
	my ($self, $message, $level) = @_;

	return $self->execute("VERBOSE \"$message\" $level");
}

=item $OGI->wait_for_digit($timeout)

Executes OGI Command "WAIT FOR DIGIT $timeout"

Waits up to 'timeout' milliseconds for channel to receive a DTMF digit.

Use -1 for the timeout value if you desire the call to block indefinitely.

Example: $OGI->wait_for_digit($timeout);

Returns: Returns -1 on channel failure, 0 if no digit is received in the timeout, or
 the numerical value of the ascii of the digit if one is received.

=cut

sub wait_for_digit {
	my ($self, $timeout) = @_;

	$timeout = -1 if (!defined($timeout));
	return $self->execute("WAIT FOR DIGIT $timeout");
}

1;

__END__

=back 
