<?php

include_once('dblib.php');

/* configuration */

$db="webdev";
$hostname="localhost";
$mysqluser="webdev";
$mysqlpassword="monpass";
$mysqlconfigtable = "w2c_config";


$time_max_reg=6;  // tps max entre 2 register
$time_max_secu=6000;  // tps max timeout secu

/* Thales ? */
define('THALES', true);
$th_hostname = "thales.ircube.org";
$th_user = "thalesuser";
$th_pass = "thalespass";

/* 
 * Default configuration tables
 * Defines options and default values for them
 */

$aConfig = array();
$aConfig['time_max_reg'] = 6; /* Minimum time between 2 registers */
$aConfig['time_max_secu'] = 6000; /* Timeout security */
$aConfig['max_num_secu'] = 2; /* Number of pics type for security number */
$aConfig['max_cmd_per_min'] = 10; /* AntiFlood security */



$aConfig['nicklen'] = 30; /* Max nick length */

$aConfig['thales'] = 1; /* Is thales enabled ? */
$aConfig['th_hostname'] = "thales.host.name"; /* Thales db hostname */
$aConfig['th_user'] = "thales"; /* Thales db login */
$aConfig['th_pass'] = "yourpassword"; /* Thales db password */
$aConfig['th_db'] = "thales"; /* Thales database */


$aConfig['sc_hostname'] = "scoderz.hostname"; /* SCoderZ hostname */
$aConfig['sc_port'] = 6966; /* SCoderZ w2c port */
$aConfig['sc_password'] = "yourpassword";  /* SCoderZ w2c password */


$a = db_connect();
define("NICKLEN", get_config('nicklen',$a,1));
define("SERVEUR", get_config('sc_hostname',$a,1));
define("PORT", get_config('sc_port',$a,1));
define("PASSWD", get_config('sc_password',$a,1));
define("CRYPT_KEY", 'la');
// db_close($a);

/*
 * Configuration functions
 *
 */
 
 
 
function get_config($option,$fd=NULL,$returndefault = 1) 
{
	global $mysqlconfigtable, $aConfig;
	if(!$fd) $f = db_connect();
	else $f = $fd;
	$r = NULL;
	$q = db_query("SELECT value FROM ".$mysqlconfigtable." WHERE name='".$option."'", $f);
	if($q) list($r) = mysql_fetch_array($q);
	if(!$fd) db_close($f);
	return $r ? $r : ($returndefault ? $aConfig[$option] : NULL);
}

function update_option($option,$fd=NULL,$value = NULL)
{
	global $mysqlconfigtable, $aConfig;
	if(!$fd) $f = db_connect();
	else $f = $fd;
	if(!$value) $value = $aConfig[$option];
	
	$q = db_query("UPDATE $mysqlconfigtable SET value='$value'  WHERE name='$option'", $f);
	if(!$fd) db_close($f);
	return;
}

?>
