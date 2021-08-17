<?php
include_once('libw2c.php');
include_once('config.php');
include_once('config.inc.php');

if(w2c_checkvars() && w2c_checkadmin())
{    /* Let's ride ! */
	$a = db_connect();
	foreach($aConfig as $k => $v)
		if($_REQUEST[$k]) update_option($k,$a,$_REQUEST[$k]);
	db_close($a);
	header('Location: admin.php');


} else 
	w2c_printerror(3); /* Not an admin */

?>