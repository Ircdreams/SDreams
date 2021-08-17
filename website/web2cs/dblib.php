<?php 

function db_connect()
{
	global $hostname, $mysqluser, $mysqlpassword, $db;
	global $db_link;
	
	$db_link = mysql_connect ($hostname, $mysqluser, $mysqlpassword);
	mysql_select_db ($db, $db_link);
	return $db_link;
}


function db_close($fd = NULL)
{
	global $db_link;
	$a = $fd ? $fd : $db_link;
	@mysql_close ($fd);
}


function db_query($query, $fd = NULL)
{    
	global $db_link;
	$a = $fd ? $fd : $db_link;
	//echo "Query : $query <br />\n";
	return mysql_query ($query, $a);
}

 function db_table_exists($table, $fd = NULL)
 {
	global $db_link;
	$a = $fd ? $fd : $db_link;
	$exists = db_query("SHOW TABLES LIKE '$table'",$a);
	return mysql_num_rows($exists) == 1;
}

?>
