<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{
	global $aConfig;

	$j = 0;
	echo "<h2>".$trad[$lang]['chaninfo']['chanhist'].": ".$_REQUEST['channel']."</h2><br />";
	if($_REQUEST['channel'] && w2c_isvalidchan($_REQUEST['channel']))
	{
		$access_r =  w2c_extractaccess($_SESSION['w2c_myaccess']);

		if($_SESSION['w2c_myinfo']['level'] > 1) $j = 500;
		else for($i = 1;$access_r[$i];$i++) /* find access level */
				if($access_r[$i][0] == $_REQUEST['channel'])
				{
					$j = $access_r[$i][1]; /* level */
					break;
				}
	}

	if ($j < 450) echo $trad[$lang]['chanhist']['noaccess'];
	else {

		$fd = mysql_connect($aConfig['th_hostname'], $aConfig['th_user'], $aConfig['th_pass']);
		if(!$fd) {
			echo "SQL Error<br />";
			return;
		}

		$lvl = isset($_REQUEST['count']) && w2c_checkadmin() ? $_REQUEST['count'] : 15;

		mysql_select_db($aConfig['th_db'], $fd);
		$res = mysql_query("SELECT TS,user,cmd,log from chancmdslog where chan='"
			.mysql_escape_string($_REQUEST['channel'])."' order by TS desc LIMIT 0,".$lvl.";", $fd);

		 if(!mysql_num_rows($res)) echo $trad[$lang]['chanhist']['nodata'];
		 else {
			$p = 1;
			echo '<Table class="tble" ><tr>
			 <td class="titre">'.$trad[$lang]['chanhist']['TS'].'<br /><br /></td>
			 <td class="titre">'.$trad[$lang]['chanhist']['username'].'<br /><br /></td>
			 <td class="titre">'.$trad[$lang]['chanhist']['cmd'].'<br /><br /></td>
			 <td class="titre">'.$trad[$lang]['chanhist']['logs'].'<br /><br /></td></tr>';

			while(($row=mysql_fetch_array($res,MYSQL_ASSOC)))
				if(time() - $row['TS'] <= 86400)
					echo "<tr><td>*".date("j-m-y\&\\n\b\s\p\;G:i:s", $row['TS'])."</td>
					<td>".$row['user']."</td><td>".$row['cmd']."</td>
					<td>".$row['log']."</td></tr>";
				else
					echo "<tr><td>".date("j-m-y\&\\n\b\s\p\;G:i:s", $row['TS'])."</td>
					<td>".$row['user']."</td><td>".$row['cmd']."</td>
					<td>".$row['log']."</td></tr>";

			mysql_free_result($res);
			mysql_close ($fd);
			echo "</table>";
		}
	}
}
else w2c_printerror(1);

do_footer();
php?>
