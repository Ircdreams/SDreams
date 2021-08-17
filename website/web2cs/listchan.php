<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{

?>
<h2><? echo $trad[$lang]['listchan']['titre']; ?></h2>
<br />
	<?
			if ($_REQUEST['accept'])
				echo "MYACCESS ACCEPT ".$_REQUEST['channel']."<br />";
			if ($_REQUEST['refuse'])
				echo "MYACCESS REFUSE ".$_REQUEST['channel']."<br />";		

	?>
<?
	$total = $_SESSION['w2c_myaccess']['accesscount'];
	$i = 1;
	if ($total != 0) 
		while($i <= $total) {
			$info = strchr($_SESSION['w2c_myaccess']['access'.$i], ':');
			$ac = explode(' ', $_SESSION['w2c_myaccess']['access'.$i]);
			
			if (!($ac[2] & A_WAITACCESS)) {
				$i++;
				continue;
			}
		?>
			<form method="post" action="listchan.php">
		<?
			echo $trad[$lang]['chanopt']['propaccess']['0'].$ac[0].$trad[$lang]['chanopt']['propaccess']['1'].$ac[1]."<br />\n";
		?>
			<input type="hidden" name="channel" value="<? echo $ac[0]; ?>">	
			<input type="submit" name="accept" value="Accepter">			
			<input type="submit" name="refuse" value="Refuser">				
			</form>
		<?
			$i++;
			}
		?>
<br />
<?
	if ($_REQUEST['Regchan']) {
	
		if (w2c_isvalidchan($_REQUEST['channel'])) {
			if (($_REQUEST['desc']) && (w2c_isvaliddesc($_REQUEST['desc'])))
					echo "Regchan ".$_REQUEST['channel']." ".$_REQUEST['desc']."<br />\n"; 
				else	
				$erreur .= $trad[$lang]['listchan']['errdesc']; 		
		}
		else 
			$erreur .= $trad[$lang]['listchan']['errname']; 
	
	}

?>
<?
	$total = $_SESSION['w2c_myaccess']['accesscount'];
	$i = 1;
	if ($total != 0) 
	{
?>
<form action="chanopt.php" method="post">
<table width="339" border="0" align="center">

<tr><td width="171"><? echo $trad[$lang]['listchan']['chan']; ?>
  <div align="right"></div></td><td width="158">
    <div align="left">
      <select name="channel">
        <?
	while($i <= $total) {
			$info = strchr($_SESSION['w2c_myaccess']['access'.$i], ':');
			$ac = explode(' ', $_SESSION['w2c_myaccess']['access'.$i]);
			if (!($ac[2] & A_WAITACCESS)) 
				echo "<option value=\"".$ac[0]."\">".$ac[0]."</option>\n";
			$i++;
		}
?>
      </select>
    </div></td></tr>

<tr><td colspan="2" align="center"><input type="submit" value="<? echo $trad[$lang]['listchan']['manage']; ?>"></td></tr>
</table>
</form>
<?
	}
	else
	{
		echo $trad[$lang]['listchan']['noaccess'];
	}
?>
<?
	$chan = user_isowner($_SESSION['w2c_myaccess']);
	if ($chan) {
		echo $trad[$lang]['listchan']['own'].$chan."<br />\n";
	}
	else
	{ 
?>
<h2><? echo $trad[$lang]['listchan']['titre2']; ?></h2>
<?
	if ($_REQUEST['Regchan']) {
		if ($erreur) {
			echo "Erreur(s) lors de l'enregistrement de votre salon :<br />\n";
			echo $erreur;
		}
	}
?>
<form action="listchan.php" method="post">
  <table width="660" border="0" align="center">
    <tr>
      <td width="193"><div align="right"><? echo $trad[$lang]['listchan']['chan']; ?></div></td>
      <td width="457"><input name="channel" type="text" value="<? echo $_REQUEST['channel']; ?>" size="30"> 
      <? echo $trad[$lang]['listchan']['exchan']; ?></td>
    </tr>
    <tr>
      <td align="center"><div align="right"><? echo $trad[$lang]['listchan']['desc']; ?></div></td>
      <td><input name="desc" type="text" value="<? echo $_REQUEST['desc']; ?>" size="50%"></td>	  
    </tr>
    <tr>
      <td colspan="2" align="center"><input name="Regchan" type="submit" value="<? echo $trad[$lang]['listchan']['register']; ?>"></td>
    </tr>
  </table>


</form>
<?
	}
?>

<?
}
else
	w2c_printerror(1);
	
do_footer();
?>