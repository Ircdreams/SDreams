<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{

	$axx = user_access($_REQUEST['channel'] ,$_SESSION['w2c_myaccess']);

	if (!$axx[0]) {
		echo "<h2>Chaninfo : ".$_REQUEST['channel']."</h2><br />\n";
		echo "Vous n'avez pas d'access sur ce salon<br />\n";
	}
	else
	{

	?>

		<?
		echo "<h2>Access : ".$_REQUEST['channel']."</h2><br />";
		echo "Votre niveau sur ce salon est ".$axx[1]."<br />\n";	
?>
		<form action="chanopt.php" method="post">
			<input type="hidden" name="channel" value="<? echo $_REQUEST['channel']; ?>">
			<input type="submit" name="Modifopt" value="Retour aux options">
		</form>

<?		

		$access = w2c_db_getaccess($_SESSION['w2c_login'], $_SESSION['w2c_password'], 
										$_REQUEST['channel'], "*");
									
?>
<?								
		if($access['erreur'])
			echo "Le salon ".$_REQUEST['channel']." n'est pas enregistré !<br />";
		else
		{
		$total = $access['accesscount'];	
?>
<?
	if ($_REQUEST['Modifier']) {
		echo "Modifications :<br />\n";
		

		$i = 1;
		while ($i <= $total) {
			$flag = $_REQUEST['autoop'.$i] + $_REQUEST['autovo'.$i] + $_REQUEST['protect'.$i] + $_REQUEST['suspend'.$i];
			$query = was_modif($_REQUEST['nick'.$i], $access, $flag,  $_REQUEST['info'.$i] ,  $_REQUEST['level'.$i]);
			if ($query)
				echo $_REQUEST['nick'.$i] . " => " . $query."<br />\n";
			else 
				echo "Pas de modif pour " . $_REQUEST['nick'.$i]."<br />\n";
			$i++;
		}
		
	}
?>


		<form action="modifaccess.php" method="post">
		
		<Table width="100%" border="1" >
		<tr><td width="13%">Username</td><td width="7%">Level</td><td width="18%">Autoop</td><td width="18%">Autovoice</td><td width="18%">Protege</td><td width="18%">Suspendu</td>
		</tr><br>

<?
			$i = 1;
			while($i <= $total) {

			   //On separe le message du reste des infos
		        list($ac, $info) = explode(':', $access['access'.$i], 2);

				$ac = explode(' ', $ac);
				if (!($ac[3] & A_WAITACCESS )) {
?>
				<tr>

					<td>
						<input type="hidden" name="nick<? echo $i; ?>" value="<? echo $ac[0]; ?>">
						<strong><? echo $ac[0]; ?></strong>
					</td>
					<td>
						<input name="level<? echo $i; ?>" type="text" size="3" value="<? echo $ac[1]; ?>">
					</td>
					<td width="18%">
						<input name="autoop<? echo $i; ?>" type="radio" value="<? echo A_OP; ?>" <? if ($ac[3] & A_OP ) echo 'checked';?>>ON
               	  <input name="autoop<? echo $i; ?>" type="radio" value="0" <? if (!($ac[3] & A_OP )) echo 'checked';?>>OFF				  </td>
					<td width="18%">
						<input name="autovo<? echo $i; ?>" type="radio" value="<? echo A_VOICE; ?>" <? if ($ac[3] & A_VOICE ) echo 'checked';?>> ON
                  <input name="autovo<? echo $i; ?>" type="radio" value="0" <? if (!($ac[3] & A_VOICE )) echo 'checked';?>>OFF				  </td>
					<td width="18%">
						<input name="protect<? echo $i; ?>" type="radio" value="<? echo A_PROTECT; ?>" <? if ($ac[3] & A_PROTECT) echo 'checked';?>>ON
                  <input name="protect<? echo $i; ?>" type="radio" value="0" <? if (!($ac[3] & A_PROTECT)) echo 'checked';?>>OFF				  </td>
					<td width="18%">
						<input name="suspend<? echo $i; ?>" type="radio" value="<? echo A_SUSPEND; ?>" <? if ($ac[3] & A_SUSPEND) echo 'checked';?>>ON
                  <input name="suspend<? echo $i; ?>" type="radio" value="0" <? if (!($ac[3] & A_SUSPEND)) echo 'checked';?>>OFF				  </td>					
				</tr>
				<tr>
					<td colspan=6>
						Infoline :<input name="info<? echo $i; ?>" type="text" value="<? echo $info; ?>" size="70">
						<br><br>
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
		?>
		<input type="hidden" name="channel" value="<? echo $_REQUEST['channel']; ?>">
		<input type="submit" name="Modifier" value="Modifier">
	</form>
	<form action="modifaccess.php" method="post">
		<input type="hidden" name="channel" value="<? echo $_REQUEST['channel']; ?>">
		<input name="Reset" type="submit" value="Reinitialiser les valeurs">
	</form>
	<?
	}
}
else
	w2c_printerror(1);

do_footer();
?>


<?

function was_modif($nick, $axx, $flag, $info, $lvl)
{	
	// on cherche le nick s'il n'y est pas conflit => on ne fait rien
	$total = $axx['accesscount'];	
	$i = 1;
	while($i <= $total) {	
		list($ac, $oldinfo) = explode(':', $axx['access'.$i], 2);
		$ac = explode(' ', $ac); 	
		if ($ac[0] == $nick)
			break;
		$i++;
	}
	
	if ($i > $total)
		return $res;
	
	if ($flag != $ac[3])
		$res .= " flag ".$flag;
	if ($info != $oldinfo) {
		if (!(($oldinfo == '') && ($info == 'Aucune'))) {
			if  (($info == 'Aucune') || ($info == ''))
				$res .= " info none";
			else
				$res .= " info ".$info;
		}
	}
	if ($lvl != $ac[1])
		$res .= " level ".$lvl	;
	return $res;
}

?>