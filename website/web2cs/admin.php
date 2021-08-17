<?php
include_once('config.inc.php');
include_once("libw2c.inc.php");
include_once("config.php");
include_once("dblib.php");
do_head();
if(w2c_checkvars() && w2c_checkadmin())
{    /* Let's ride ! */
?>
	<h2><? echo $trad[$lang]['admin']['titre']; ?></h2>
<?
$a = db_connect();
if(!db_table_exists($mysqlconfigtable,$a)) echo $trad[$lang]['admin']['tabledoesnotexist'];
else {
?>
<form action="updateconfig.php" method="POST">
<table>
<?
	foreach($aConfig as $k => $v) {
		$v = get_config($k,$a,0);
		if(!$v) { $missing=1; } /* Missing one in db ? */
		echo "<tr><td><acronym title=\"".$trad[$lang]['config'][$k]." (".$trad[$lang]['admin']['defaultvalue']." : ".$aConfig[$k].")\">$k</acronym></td><td>"
			."<input type=\"text\" name=\"$k\" value=\"$v\" /></td></tr>\n";
	}
?>
<tr><td colspan="2"><input type="submit" /></td></tr>
</table>
</form>


<?
	if($missing)
		echo $trad[$lang]['admin']['missingoptions'];
}
db_close($a);
}
else  /* Go fuck. */
	w2c_printerror(3); /* Not an admin */

do_footer();
?>