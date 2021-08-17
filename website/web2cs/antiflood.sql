

DROP TABLE IF EXISTS antiflood;
CREATE TABLE antiflood (
  ip tinytext NOT NULL,
  time int(10) NOT NULL default '0',
  flood int(4)  NOT NULL default '0'
) TYPE=MyISAM;
