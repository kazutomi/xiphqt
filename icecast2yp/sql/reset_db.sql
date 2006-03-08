use oddsock;
DROP TABLE if exists server_details;
DROP TABLE if exists servers_touch;
DROP TABLE if exists servers;
DROP TABLE if exists clusters;
DROP TABLE if exists playlists;
DROP TABLE if exists listens;

-- MySQL dump 9.08
--
-- Host: localhost    Database: oddsock
---------------------------------------------------------
-- Server version	4.0.13-log

--
-- Table structure for table 'listens'
--

CREATE TABLE if not exists listens (
  listen_ip varchar(25) default NULL,
  server_name varchar(100) default NULL,
  listen_time timestamp(14) NOT NULL
) TYPE=MyISAM;

--
-- Table structure for table 'server_details'
--

CREATE TABLE if not exists server_details (
  id mediumint(9) NOT NULL auto_increment,
  parent_id mediumint(9) default NULL,
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  description varchar(255) default NULL,
  genre varchar(100) default NULL,
  sid varchar(200) default NULL,
  cluster_password varchar(50) default NULL,
  url varchar(255) default NULL,
  current_song varchar(255) default NULL,
  listen_url varchar(200) default NULL,
  playlist_id mediumint(9) default NULL,
  server_type varchar(25) default NULL,
  server_subtype varchar(255) default NULL,
  bitrate varchar(25) default NULL,
  listeners int(11) default NULL,
  channels varchar(25) default NULL,
  samplerate varchar(25) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

create table if not exists playlists (
  id mediumint(9) NOT NULL,
  listen_url varchar(200) default NULL
) TYPE=MyISAM;

create table if not exists clusters (
  id mediumint(9) NOT NULL auto_increment,
  server_name varchar(255) default NULL,
  cluster_password varchar(50) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table 'servers'
--

CREATE TABLE if not exists servers (
  id mediumint(9) NOT NULL auto_increment,
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  listeners int(11) default NULL,
  rank int(11) default NULL,
  yp_status varchar(100) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table 'servers_touch'
--

CREATE TABLE if not exists servers_touch (
  id varchar(200) NOT NULL default '',
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  last_touch datetime default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

