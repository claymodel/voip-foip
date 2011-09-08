#
# $Id: CallWeaver.org.pm,v 1.12 2006/09/25 08:42:54 james Exp $
#
package CallWeaver;

require 5.004;

$VERSION = '0.09';

sub version { $VERSION; }

sub new {
	my ($class, %args) = @_;
	my $self = {};
	$self->{configfile} = undef;
	$self->{config} = {};
	bless $self, ref $class || $class;
	return $self;
}

sub DESTROY { }

1;
