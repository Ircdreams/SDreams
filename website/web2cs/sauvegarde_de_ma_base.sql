-- MySQL dump 9.10
--
-- Host: localhost    Database: web2cs
-- ------------------------------------------------------
-- Server version	4.0.18

--
-- Table structure for table `coderegister`
--

DROP TABLE IF EXISTS coderegister;
CREATE TABLE coderegister (
  ip tinytext NOT NULL,
  seed int(10) NOT NULL default '0',
  time int(10) NOT NULL default '0',
  _1 char(1) NOT NULL default '',
  _2 char(1) NOT NULL default '',
  _3 char(1) NOT NULL default '',
  _4 char(1) NOT NULL default '',
  _5 char(1) NOT NULL default ''
) TYPE=MyISAM;

--
-- Dumping data for table `coderegister`
--


/*!40000 ALTER TABLE coderegister DISABLE KEYS */;
LOCK TABLES coderegister WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE coderegister ENABLE KEYS */;

--
-- Table structure for table `register`
--

DROP TABLE IF EXISTS register;
CREATE TABLE register (
  ip tinytext NOT NULL,
  time int(10) default NULL
) TYPE=MyISAM;

--
-- Dumping data for table `register`
--



