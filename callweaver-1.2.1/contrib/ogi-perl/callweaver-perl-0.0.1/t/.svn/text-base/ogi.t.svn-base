#Just do some simple tests to make sure the basic stuff works
use strict;
use Test;

use lib '../lib';
use lib 'lib';

BEGIN { plan tests => 2 }

use CallWeaver::OGI;

my $OGI = new CallWeaver::OGI;

my $fh;
open($fh, "<t/ogi.head") || die;
my %vars = $OGI->ReadParse($fh);
close($fh);
ok(%vars);
open($fh, "<t/ogi.goodresponse") || die;
my $response = $OGI->_readresponse($fh);
ok($response);
