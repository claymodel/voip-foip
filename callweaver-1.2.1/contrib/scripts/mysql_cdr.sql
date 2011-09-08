--
-- mysql_cdr.sql
--
-- By Roy Sigurd Karlsbakk <roy@karlsbakk.net>
--
-- Plain GP
--

CREATE TABLE cdr (
	id int(11) NOT NULL auto_increment,
	calldate datetime,
	clid varchar(80),
	src varchar(80),
	dst varchar(80),
	dcontext varchar(80),
	channel varchar(80),
	dstchannel varchar(80),
	lastapp varchar(80),
	lastdata varchar(80),
	duration int(11),
	billsec int(11),
	disposition varchar(45),
	amaflags int(11),
	accountcode varchar(20),
	uniqueid varchar(32),
	userfield varchar(255),
	PRIMARY KEY  (id),
	KEY cdr_src_idx (src),
	KEY cdr_dst_idx (dst)
) TYPE=MyISAM;

