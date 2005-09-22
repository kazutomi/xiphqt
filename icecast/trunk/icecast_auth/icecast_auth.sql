CREATE TABLE icecastauth_audit (
  datestamp datetime default NULL,
  username varchar(50) NOT NULL default '',
  server varchar(255) NOT NULL default '',
  mountpoint varchar(100) NOT NULL default '',
  action varchar(255) NOT NULL default ''
) TYPE=MyISAM;

--
-- Table structure for table `listen_detail`
--

CREATE TABLE icecastauth_listen_detail (
  username varchar(50) NOT NULL default '',
  datestamp datetime NOT NULL default '0000-00-00 00:00:00',
  server varchar(255) NOT NULL default '',
  mountpoint varchar(100) NOT NULL default '',
  time_spent_listening bigint(20) NOT NULL default '0',
  PRIMARY KEY  (username,datestamp,mountpoint)
) TYPE=MyISAM;

--
-- Table structure for table `mountpoints`
--

CREATE TABLE icecastauth_mountpoints (
  mountpoint_name varchar(100) NOT NULL default '',
  PRIMARY KEY  (mountpoint_name)
) TYPE=MyISAM;

--
-- Table structure for table `users`
--

CREATE TABLE icecastauth_users (
  username varchar(50) NOT NULL default '',
  password varchar(255) NOT NULL default '',
  mountpoint varchar(100) NOT NULL default '',
  email varchar(255) default NULL,
  timeleft bigint(20) default NULL,
  PRIMARY KEY  (username,mountpoint)
) TYPE=MyISAM;

