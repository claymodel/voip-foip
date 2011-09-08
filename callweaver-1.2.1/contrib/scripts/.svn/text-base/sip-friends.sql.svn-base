--
-- Table structure for table `sipfriends`
--

CREATE TABLE sipfriends (
  name varchar(40) primary key,
  username varchar(40),
  secret varchar(40) NOT NULL,
  context varchar(40) NOT NULL default 'default',
  ipaddr varchar(40) default NULL,
  port int(6) default '5060',
  regseconds int(11) default NULL,
  dtmfmode varchar(50) NOT NULL default 'inband',
  restrictcid varchar(50) default NULL,
  qualify varchar(10) default 'no',
  insecure varchar(50) default NULL,
  callerid varchar(10) default NULL,
  disallow varchar(50) default NULL,
  allow varchar(50) default NULL,
  nat varchar(10) default 'yes',
  host varchar(20) default 'dynamic',
  fullcontact varchar(80) default NULL,
  accountcode varchar(50) default NULL,
  useragent varchar(255) default NULL,
  transfer varchar(10) default 'no',
  canreinvite varchar(10) NOT NULL default 'no',
  pickupgroup varchar(64) NOT NULL,
  callgroup varchar(64) NOT NULL,
  regserver varchar(64) default NULL,
  call_limit int(11) default '1'
); 

create unique index sipfriends_name_idx on sipfriends(name);
create unique index sipfriends_username_idx on sipfriends(username);

