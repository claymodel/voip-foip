package alias_ldap;

use OpenSER::LDAPUtils::LDAPConf;
use OpenSER::LDAPUtils::LDAPConnection;

use OpenSER::Constants;

sub init {}

sub query {
	my $self = shift;
	my $alias_username = shift;
	my $alias_domain = shift;

	my $uri = "$alias_username\@$alias_domain";
	my $ldap = new OpenSER::LDAPUtils::LDAPConnection();

	OpenSER::log(L_INFO, "Trying LDAP request with $uri\n");
	my @ldaprows = $ldap->search("(&(ObjectClass=inetOrgPerson)(mail=$uri))", "ou=people,dc=example,dc=com", "uid");

	if (@ldaprows[0]) {
		OpenSER::log(L_INFO, "Got a row: ".@ldaprows[0]."\n");
		my $ret;
		$ret->{username} = @ldaprows[0];
		$ret->{domain} = "voip";
		return $ret;
	}
	return;
}


1;
