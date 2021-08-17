<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{

	?>
	<h2><? echo $trad[$lang]['memos']['liste']; ?></h2>
	<?

	if ($_REQUEST['suppr']) {
		$query = "MEMO DEL ";
		$i = 1;
		$nb = 0;
		while ($i <= $_SESSION['w2c_mymemo']['memocount'] ) {

			if ( $_REQUEST['id'.$i] ) {
				if ($nb) $query .= ",";
			 	$query .= "$i";
				$nb++;
			 }
			$i++;
		}
		if ($nb) {
			$res = w2c_db_query($_SESSION['w2c_login'], $_SESSION['w2c_password'],$query);
			if (!$res['erreur'])
				$_SESSION['w2c_mymemo'] = $res;
		}
	}
	else
	{
		$res = w2c_db_query($_SESSION['w2c_login'], $_SESSION['w2c_password'],"MEMO READ");
		$_SESSION['w2c_mymemo'] = $res;
	}
	if($_SESSION['w2c_mymemo']['memocount'] == 0) echo $trad[$lang]['memos']['nomemos'];
	else
	{
	?>
	<form action="mesmemos.php" method="POST">
	<table class="tble">
	<tr>
		<td class="titre"><? echo $trad[$lang]['memos']['de']; ?>
		</td>
		<td class="titre"><? echo $trad[$lang]['memos']['date']; ?>
		</td>
		<td class="titre"><? echo $trad[$lang]['memos']['del']; ?>
		</td>
	</tr>
	<?


		for($i=1;$_SESSION['w2c_mymemo']['memo'.$i];$i++)
		{
		//Recup de la ligne du memo
			$memo = $_SESSION['w2c_mymemo']['memo'.$i];
		//On separe le message du reste des infos
	        list($memo, $text) = explode(':', $memo, 2);
		//On sépare les infos
			list($from, $when, $new)  = explode(" ", $memo);
			$when = date("D j M G:i:s", $when);
	
			$text = ($new == 0) ? "<b> ".$text."</b> " : $text ;
			echo "<tr><td>".$from."</td><td>".$when."</td><td align=\"center\" rowspan=\"2\"><input type=\"checkbox\" name=\"id$i\" ></td></tr>\n";
			echo "<tr><td colspan=\"2\" class=\"memo\">".w2c_stripircchars($text)."</td></tr>\n";
		}

		?>
		<tr>
			<td colspan="2"><? echo $trad[$lang]['memos']['total']['0']; ?> <? echo ($i - 1); ?> <? echo $trad[$lang]['memos']['total']['1']; ?>
			</td>
			<td align="center"><input type="submit" name="suppr" value="suppr">
			</td>
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