<?
include_once('config.inc.php');
include_once("libw2c.inc.php");
include_once("config.php");
include_once("dblib.php");

$a = db_connect();
if(db_table_exists($mysqlconfigtable,$a)) {
	db_close($a);
	echo "The table already exists.<br />\n";
} else {
	db_query("CREATE TABLE `$mysqlconfigtable` ( `name` VARCHAR( 255 ) , `value` TEXT );",$a);
	echo "Table created<br />\n";
}


foreach($aConfig as $k => $v) { /* Insert default values */
	if(!get_config($k,$a,0)) /* This option isn't registered in the database */
	{
		echo "Inserting option $k with value $v<br />\n";
		db_query("INSERT INTO $mysqlconfigtable (name, value) VALUES('$k', '$v');",$a);
	}
	else /* Option already registered => reset to default value */
	{
		if($_REQUEST['a'] != 1) echo "Option $k already exists. Skipping<br />\n";
		else {
			echo "Updating option $k to value $v<br />\n";
			update_option($k,$a);
		}
	}
}



echo "Default values inserted<br />\n";
db_close($a);
?>