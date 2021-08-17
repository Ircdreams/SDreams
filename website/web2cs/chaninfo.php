<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{
	if(!$_REQUEST['channel'] || !($i = w2c_isvalidchan($_REQUEST['channel'])))
	{
?>

<h2><? echo $trad[$lang]['chaninfo']['chaninfo']; ?></h2>
<br />
<br />

<div style="text-align : center;">
<form action="chaninfo.php" method="post">
<table width="250" border="0" align="center">
<?php
		if((!$i) && $_REQUEST['channel'])
			echo "<tr><td align=\"center\" colspan=\"2\">".$trad[$lang]['chaninfo']['badchanname']."</td></tr>";
?>
<tr>
	<td>
		<? echo $trad[$lang]['chaninfo']['titre']; ?>
	</td>
	<td>
		<input type="text" name="channel" value="<? echo $_REQUEST['channel']; ?>">
	</td>
</tr>
<tr>
	<td colspan="2" align="center">
		<input type="submit">
	</td>
</tr>
</table>
</form>
</div>
<?
	}
	else
	{
		echo "<h2>".$trad[$lang]['chaninfo']['chaninfo']." ".$_REQUEST['channel']."</h2><br />";
		$chaninfo = w2c_db_getchaninfo($_SESSION['w2c_login'], $_SESSION['w2c_password'], $_REQUEST['channel'], 'all');

		$access_r =  w2c_extractaccess($_SESSION['w2c_myaccess']);
		$j = 0;
		if($_SESSION['w2c_myinfo']['level'] > 1) $j = 500;
		else for($i = 1;$access_r[$i];$i++) /* find access level */
				if($access_r[$i][0] == $_REQUEST['channel'])
				{
					$j = $access_r[$i][1]; /* level */
					break;
				}


		if($chaninfo['erreur'])
			echo $trad[$lang]['chaninfo']['notreg']['1'].$_REQUEST['channel']
					.$trad[$lang]['chaninfo']['notreg']['2'];
		/* elseif(!$j && $chaninfo['modes'] & (C_MPRIVATE|C_MSECRET))
			echo $trad[$lang]['chaninfo']['noaccess']; */
		else
		{

		?>

		<table width="798" border="0" cellspacing="2">

          <tr>
            <td width="305"><strong><? echo $trad[$lang]['chaninfo']['owner']; ?></strong></td>
            <td width="532"><? echo $chaninfo['owner']; ?></td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['desc']; ?></strong></td>
            <td><? echo w2c_stripircchars($chaninfo['description']); ?></td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['topic']; ?></strong></td>
            <td><? echo w2c_stripircchars($chaninfo['topic']); ?></td>
          </tr>
          <tr>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['modes']; ?></strong></td>
            <td>
				<?
				if ($chaninfo['modes'] & C_MMSG) echo 'n';
				if ($chaninfo['modes'] & C_MTOPIC) echo 't';
				if ($chaninfo['modes'] & C_MNOCTRL) echo 'c';
				if ($chaninfo['modes'] & C_MMODERATE) echo 'm';
				if ($chaninfo['modes'] & C_MINV) echo 'i';
				if ($chaninfo['modes'] & C_MPRIVATE) echo 'p';
				if ($chaninfo['modes'] & C_MSECRET) echo 's';
				if ($chaninfo['modes'] & C_MNOCTCP) echo 'C';
				if ($chaninfo['modes'] & C_MUSERONLY) echo 'r';
				if ($chaninfo['modes'] & C_MKEY) echo 'k';
				if ($chaninfo['modes'] & C_MLIMIT) echo 'l';
				if ($chaninfo['modes'] & C_MKEY) echo " ".$chaninfo['key'];
	  			if ($chaninfo['modes'] & C_MLIMIT) echo " ".$chaninfo['limit'];
				?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['defmodes']; ?></strong></td>
            <td>
				<?
				if ($chaninfo['defmodes'] & C_MMSG) echo 'n';
				if ($chaninfo['defmodes'] & C_MTOPIC) echo 't';
				if ($chaninfo['defmodes'] & C_MNOCTRL) echo 'c';
				if ($chaninfo['defmodes'] & C_MMODERATE) echo 'm';
				if ($chaninfo['defmodes'] & C_MINV) echo 'i';
				if ($chaninfo['defmodes'] & C_MPRIVATE) echo 'p';
				if ($chaninfo['defmodes'] & C_MSECRET) echo 's';
				if ($chaninfo['defmodes'] & C_MNOCTCP) echo 'C';
				if ($chaninfo['defmodes'] & C_MUSERONLY) echo 'r';
				if ($chaninfo['defmodes'] & C_MKEY) echo 'k';
				if ($chaninfo['defmodes'] & C_MLIMIT) echo 'l';
				if ($chaninfo['defmodes'] & C_MKEY) echo " ".$chaninfo['defkey'];
				if ($chaninfo['defmodes'] & C_MLIMIT) echo " ".$chaninfo['deflimit'];
				?>
            </td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['bantype']; ?></strong></td>
            <td>
			<?
			if ($chaninfo['bantype'] == 1) echo '*!*ident@*.host';
			elseif ($chaninfo['bantype'] == 2) echo '*!*ident@*';
			elseif ($chaninfo['bantype'] == 3) echo '*nick*!*@*';
			elseif ($chaninfo['bantype'] == 4) echo '*!*@host';
			else echo '*!*ident@host';
			 ?>
	    </td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['bantime']; ?></strong></td>
            <td><? echo $chaninfo['bantime']; ?> sec</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['banlevel']; ?></strong></td>
            <td><? echo $chaninfo['banlevel']; ?></td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['chmodlvl']; ?></strong></td>
            <td><? echo $chaninfo['cml']; ?></td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['noops']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_NOOPS)
						echo 'ON';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['strictop']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_STRICTOP)
						echo 'ON';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['locktopic']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_LOCKTOPIC)
						echo 'ON';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['nobans']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_NOBANS)
						echo 'ON';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['autoinv']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_AUTOINVITE)
						echo 'ON';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['autolim']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_FLIMIT)
						echo 'ON '.'inc : '.$chaninfo['liminc'].' , grace : '.$chaninfo['limgrace'];
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['welcome']; ?></strong></td>
            <td><? if ($chaninfo['options'] & C_SETWELCOME)
						echo 'ON ';
					else
						echo 'OFF'; ?>
			</td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['wecomemsg']; ?></strong></td>
            <td><? echo w2c_stripircchars($chaninfo['welcome']); ?></td>
          </tr>
          <tr>
            <td><strong><? echo $trad[$lang]['chaninfo']['options']['motd']; ?></strong></td>
            <td><? echo w2c_stripircchars($chaninfo['motd']); ?></td>
          </tr>
</table>

	<?
		}
	}
	if ($j >= 450)
		echo '<br><form action="chanhistorique.php" method="post">
		<input type="hidden" name="channel" value="'.$_REQUEST['channel'].'">
		<input type="submit" value="'.$trad[$lang]['chaninfo']['chanhist'].'"></form>';

}
else
	w2c_printerror(1);

do_footer();
?>

