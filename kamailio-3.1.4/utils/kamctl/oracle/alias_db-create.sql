INSERT INTO version (table_name, table_version) values ('dbaliases','1');
CREATE TABLE dbaliases (
    id NUMBER(10) PRIMARY KEY,
    alias_username VARCHAR2(64) DEFAULT '',
    alias_domain VARCHAR2(64) DEFAULT '',
    username VARCHAR2(64) DEFAULT '',
    domain VARCHAR2(64) DEFAULT '',
    CONSTRAINT dbaliases_alias_idx  UNIQUE (alias_username, alias_domain)
);

CREATE OR REPLACE TRIGGER dbaliases_tr
before insert on dbaliases FOR EACH ROW
BEGIN
  auto_id(:NEW.id);
END dbaliases_tr;
/
BEGIN map2users('dbaliases'); END;
/
CREATE INDEX dbaliases_target_idx  ON dbaliases (username, domain);

