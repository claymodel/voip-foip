-- for more and updated info please see http://www.callweaver.org/wiki/Realtime+SIP+Howto
-- or on #callweaver contact Tiav

GRANT ALL ON TABLE sip TO callweaver;

drop table sip;
CREATE TABLE sip (
id serial NOT NULL,
name character varying(80) DEFAULT '' NOT NULL,
host character varying(31) DEFAULT 'dynamic' NOT NULL,
nat character varying(5) DEFAULT 'no' NOT NULL,
"type" character varying DEFAULT 'friend' NOT NULL,
accountcode character varying(20),
amaflags character varying(7),
callgroup character varying(10),
callerid character varying(80),
cancallforward character varying(3) DEFAULT 'yes',
canreinvite character varying(3) DEFAULT 'no',
context character varying(80) DEFAULT NULL,
defaultip character varying(15) DEFAULT NULL,
dtmfmode character varying(7) DEFAULT 'rfc2833',
fromuser character varying(80) DEFAULT NULL,
fromdomain character varying(80) DEFAULT NULL,
insecure character varying(4) DEFAULT NULL,
"language" character varying(2) DEFAULT NULL,
mailbox character varying(50) DEFAULT NULL,
md5secret character varying(80) DEFAULT NULL,
permit character varying(95) DEFAULT NULL,
deny character varying(95) DEFAULT NULL,
mask character varying(95) DEFAULT NULL,
musiconhold character varying(100) DEFAULT NULL,
pickupgroup character varying(10) DEFAULT NULL,
qualify character varying(3) DEFAULT 'yes',
regexten character varying(80) DEFAULT '' NOT NULL,
restrictcid character varying(3) DEFAULT NULL,
rtptimeout character varying(3) DEFAULT NULL,
rtpholdtimeout character varying(3) DEFAULT NULL,
secret character varying(80) DEFAULT NULL,
setvar character varying(100) DEFAULT NULL,
disallow character varying(100) DEFAULT 'all',
allow character varying(100) DEFAULT 'g729;ilbc;gsm;ulaw;alaw',
fullcontact character varying(80) DEFAULT '' NOT NULL,
ipaddr character varying(15) DEFAULT '' NOT NULL,
port character varying(5) DEFAULT '' NOT NULL,
regseconds bigint DEFAULT 0::bigint NOT NULL,
username character varying(80) DEFAULT '' NOT NULL,
useragent character varying(255) DEFAULT NULL,
regserver character varying(80) DEFAULT NULL
);

ALTER TABLE ONLY sip ADD CONSTRAINT sip_name_key UNIQUE (name);
GRANT ALL ON TABLE sip TO callweaver;

