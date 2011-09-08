INSERT INTO version (table_name, table_version) values ('lcr_gw','1');
CREATE TABLE lcr_gw (
    id INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY NOT NULL,
    lcr_id SMALLINT UNSIGNED NOT NULL,
    gw_name VARCHAR(128),
    ip_addr VARCHAR(15),
    hostname VARCHAR(64),
    port SMALLINT UNSIGNED,
    params VARCHAR(64),
    uri_scheme TINYINT UNSIGNED,
    transport TINYINT UNSIGNED,
    strip TINYINT UNSIGNED,
    tag VARCHAR(16) DEFAULT NULL,
    flags INT UNSIGNED DEFAULT 0 NOT NULL,
    defunct INT UNSIGNED DEFAULT NULL,
    CONSTRAINT lcr_id_ip_addr_port_hostname_idx UNIQUE (lcr_id, ip_addr, port, hostname)
) ENGINE=MyISAM;

INSERT INTO version (table_name, table_version) values ('lcr_rule_target','1');
CREATE TABLE lcr_rule_target (
    id INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY NOT NULL,
    lcr_id SMALLINT UNSIGNED NOT NULL,
    rule_id INT UNSIGNED NOT NULL,
    gw_id INT UNSIGNED NOT NULL,
    priority TINYINT UNSIGNED NOT NULL,
    weight INT UNSIGNED DEFAULT 1 NOT NULL,
    CONSTRAINT rule_id_gw_id_idx UNIQUE (rule_id, gw_id)
) ENGINE=MyISAM;

CREATE INDEX lcr_id_idx ON lcr_rule_target (lcr_id);

INSERT INTO version (table_name, table_version) values ('lcr_rule','1');
CREATE TABLE lcr_rule (
    id INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY NOT NULL,
    lcr_id SMALLINT UNSIGNED NOT NULL,
    prefix VARCHAR(16) DEFAULT NULL,
    from_uri VARCHAR(64) DEFAULT NULL,
    stopper INT UNSIGNED DEFAULT 0 NOT NULL,
    enabled INT UNSIGNED DEFAULT 1 NOT NULL,
    CONSTRAINT lcr_id_prefix_from_uri_idx UNIQUE (lcr_id, prefix, from_uri)
) ENGINE=MyISAM;

