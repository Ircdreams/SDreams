<?php
include_once('config.inc.php');
include_once("config.php");
do_head();
if(w2c_checkvars() && w2c_checkadmin())
{
	if(!$_REQUEST['type'] || $_REQUEST['type'] != "lastr")
	{
php?>

	<h2>Admin Historique</h2>
	<br /><h3>Historique d'un salon</h3>
	<br /><form action="chanhistorique.php" method="post">
	<input type="text" name="channel">
	<input type="hidden" name="count" value=30>
	<input type="submit" value="Find it!">
	</form>
	<br /><h3>Historique des registers</h3>
	<br /><form action="adminstats.php" method="post">
	<input type="hidden" name="type" value="lastr">
	<input type="submit" value="List!">
	</form>
<?php
	}
	else {
		$fd = mysql_connect($aConfig['th_hostname'], $aConfig['th_user'], $aConfig['th_pass']);
		if(!$fd) {
			echo "SQL Error<br />";
			return;
		}

		$lvl = isset($_REQUEST['count']) && w2c_checkadmin() ? $_REQUEST['count'] : 15;

		mysql_select_db($aConfig['th_db'], $fd);
		$res = mysql_query("SELECT TS,user,log from usercmdslog where cmd='register' order by TS desc LIMIT 0,30;", $fd);

		 if(!mysql_num_rows($res)) echo $trad[$lang]['chanhist']['nodata'];
		 else {

			 echo '<table class="tble" ><tr>
				 <td class="titre">'.$trad[$lang]['chanhist']['TS'].'<br /><br /></td>
				 <td class="titre">'.$trad[$lang]['chanhist']['username'].'<br /><br /></td>
				 <td class="titre">'.$trad[$lang]['chanhist']['cmd'].'<br /><br /></td>
				 <td class="titre">'.$trad[$lang]['chanhist']['logs'].'<br /><br /></td></tr>';

			while(($row=mysql_fetch_array($res,MYSQL_ASSOC)))
				if(time() - $row['TS'] <= 86400)
					echo "<tr><td>*".date("j-m-y\&\\n\b\s\p\;G:i:s", $row['TS'])."</td>
					<td>".$row['user']."</td><td>register</td><td>".$row['log']."</td></tr>";
				else
					echo "<tr><td>".date("j-m-y\&\\n\b\s\p\;G:i:s", $row['TS'])."</td>
					<td>".$row['user']."</td><td>register</td><td>".$row['log']."</td></tr>";

			mysql_free_result($res);
			mysql_close ($fd);
			echo "</table>";
		}
	}
}
else  /* Go fuck. */
	w2c_printerror(3); /* Not an admin */

do_footer();
php?>