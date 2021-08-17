<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{
	if(!$_REQUEST['channel'] || !($i = w2c_isvalidchan($_REQUEST['channel'])))
	{
?>
<h2><? echo $trad[$lang]['access']['titre']; ?></h2>
<br />
<br />
<div style="text-align : center;">
<form action="access.php" method="POST">
<table width="350" border="0" align="center">
<?php
		if($i)
			echo "<tr><td align=\"center\" colspan=\"2\">Veuillez spécifier un nom de salon valide</td></tr>";
?>
<tr><td align="left"><? echo $trad[$lang]['access']['chan']; ?></td><td><input type="text" name="channel"></td></tr>
<tr><td align="left"><? echo $trad[$lang]['access']['all']; ?></td><td><input type="checkbox" name="all" checked></td></tr>
<tr><td align="left"><? echo $trad[$lang]['access']['autoop']; ?></td><td><input type="checkbox" name="aop" ></td></tr>
<tr><td align="left"><? echo $trad[$lang]['access']['autovoice']; ?></td><td><input type="checkbox" name="avoice" ></td></tr>
<tr><td align="left"><? echo $trad[$lang]['access']['protect']; ?></td><td><input type="checkbox" name="protect" ></td></tr>
<tr><td align="left"><? echo $trad[$lang]['access']['suspent']; ?></td><td><input type="checkbox" name="suspend" ></td></tr>
<tr><td colspan="2" align="center"><input type="submit"></td></tr>
</table>
</form>
</div>
<?
	}
	else
	{
		echo "<h2>".$trad[$lang]['access']['titre']." ".$_REQUEST['channel']."</h2><br />";
		$access = w2c_db_getaccess($_SESSION['w2c_login'], $_SESSION['w2c_password'], 
									$_REQUEST['channel']);
		
		/* $access_r =  w2c_extractaccess($_SESSION['w2c_myaccess']);
		$j = 0;
		if($_SESSION['w2c_myinfo']['level'] > 1) $j++;
		for($i = 1;$access_r[$i] && !$j;$i++)
			if($access_r[$i][0] == $_REQUEST['channel']) $j = 1; */
		
		if($access['erreur'])
			echo $trad[$lang]['chaninfo']['notreg']['1'].$_REQUEST['channel']
					.$trad[$lang]['chaninfo']['notreg']['2'];
		/* elseif(!$j)
			echo $trad[$lang]['access']['noaccess']; */
		else
		{
		$total = $access['accesscount'];	
		?>
		<Table class="tble" >
		<tr><td class="titre"><? echo $trad[$lang]['access']['username']; ?><br /><br /></td>
		<td class="titre"><? echo $trad[$lang]['access']['level']; ?><br /><br /></td>
		<td class="titre"><? echo $trad[$lang]['access']['flags']; ?><br /><br /></td>
		</tr>

<?
			$i = 1;
			while($i <= $total) {
			
			   //On separe le message du reste des infos
		        list($ac, $info) = explode(':', $access['access'.$i], 2);

				$ac = explode(' ', $ac);
				
				if (!($ac[3] & A_WAITACCESS ) &&
					(  $_REQUEST['all']
					||	(($ac[3] & A_SUSPEND ) && $_REQUEST['suspend'])				
					|| (($ac[3] & A_PROTECT ) && $_REQUEST['protect'])
					|| (($ac[3] & A_VOICE ) && $_REQUEST['avoice'])
					|| (($ac[3] & A_OP ) && $_REQUEST['aop']))
					) {
?>
				<tr>

					<td>
						<? echo $ac[0]; ?>
					</td>
					<td>
						<? echo $ac[1]; ?>
					</td>
					<td>
						<? echo strlen(($a = w2c_accessflag2str($ac[3],1))) ? $a : $trad[$lang]['access']['noflag']; ?>
					</td>					
				</tr>
				<tr>
					<td colspan="3" class="memo">
						<span id="infoline"><? echo $trad[$lang]['access']['infoline']; ?></span> <? echo strlen($info) ? w2c_stripircchars($info) : $trad[$lang]['access']['noflag']; ?><br /><br />
					</td>
				</tr>
			<?
				}
				$i++;
			}
			?>
			</table>
			<?

		}
	}
do_footer();
}
else
	w2c_printerror(1);
?>

