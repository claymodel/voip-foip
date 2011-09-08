// 


Verbose(0,"DBPUT : ");
if ( Chan.DBPut("happyfamily", "key2", "value")) {
	Verbose(0, "ok\n");
} else { 
        Verbose(0, "Error : "+Chan.DBPut("happyfamily", "key2", "value")+"\n");
}

Verbose(0, "DBGET : ");
if ( Chan.DBGet("happyfamily", "key2") == "value" ) {
	Verbose(0, "ok\n");
} else {
        Verbose(0, "Error\n");
}

Verbose(0, "DBDEL : " );

if (Chan.DBDel("happyfamily", "key2")) {
	Verbose(0, "ok\n");
} else {
        Verbose(0, "Error\n");
}

// Try to get a variable that doesn't exist, verify failure
Verbose (0, "DBGET : " );
if (Chan.DBGet("asdfasdf", "asdfa") == false) {
	Verbose(0, "ok\n");
} else { 
	Verbose(0, "Error : " + Chan.DBGet("asdfasdf", "asdfa") + "\n");
}

