use OpenSER qw ( log );
use OpenSER::Constants;

# Demonstrating the three ways of internal logging.
# Additionally, you have the xlog module...

sub logdemo {
	my $m = shift; 

	log(L_INFO, "Logging without OpenSER:: - import the symbol manually! See use statement...\n");
	OpenSER::log(L_INFO, "This is the other preferred way: Include the package name");
	$m->log(L_INFO, "The Message object has its own logging function. Rather don't use it ;)");

	return 1;
}
