#!/usr/local/bin/perl
use NTCP;

$ntcp = init NTCP::Client ({  
			    -server => "jabber.mydomain.com",
			    -username => "res_jabber",
			    -password => "mypass",
			    -resource => "controller",
			    -connect => "yes",
			    -talkto => "res_jabber\@jabber.mydomain.com",
			    -talkto_resource => "master_thread"
			   });


$ntcp->testclient("res_jabber\@jabber.mydomain.com", "master_thread");

