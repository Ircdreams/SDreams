<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{
?>

<h2><? echo $trad[$lang]['moncompte']['titre']; ?></h2>
<?php
if(0) {
	$memos = w2c_extractmemos($_SESSION['w2c_mymemo']);
    /*$i commence a 1 car dans w2c_extractmemos($a)  $tab commence a 1*/
	for($j=0,$i=1;$memos[$i];$i++) if($memos[$i][2] == 0) $j++;

	if ( $j > 0 ) echo $trad[$lang]['moncompte']['newmemos']['1'].$j
						.$trad[$lang]['moncompte']['newmemos']['2'].
						 (($j > 1) ? $trad[$lang]['moncompte']['newmemos']['3'] : $trad[$lang]['moncompte']['newmemos']['4']) .
						  (($j > 1) ? $trad[$lang]['moncompte']['newmemos']['5'] : $trad[$lang]['moncompte']['newmemos']['6']);
	else echo $trad[$lang]['moncompte']['nonewmemos'];
}
?>
<?


if ($_REQUEST['modif']) {

	// on recupere le flags des otpions
	$opts = $_SESSION['w2c_myinfo']['options'];
	
	$query = " set ";
	
	$res = error_username($_SESSION['w2c_login'], stripslashes($_REQUEST['username']));
	if ((!$res[0]) && $res[1])
		$query .=  " username ".stripslashes($_REQUEST['username']); 
	$text .= $res[1];
	$erreur += $res[0];

	$res = error_pass($_SESSION['w2c_password'], stripslashes($_REQUEST['pass']), stripslashes($_REQUEST['pass2']));		
	if ((!$res[0]) && $res[1])
		$query .= " pass " . stripslashes($_REQUEST['pass']) ;
	$text .= $res[1];		
	$erreur += $res[0];
	
	$res = error_mail($_SESSION['w2c_myinfo']['mail'], stripslashes($_REQUEST['mail']));
	if ((!$res[0]) && $res[1])
			$query .= " mail " . stripslashes($_REQUEST['mail']) ;
	$text .= $res[1];
	$erreur += $res[0];		
	

	if (($_REQUEST['lang']) 			
		&& ($_SESSION['w2c_myinfo']['lang']  != $_REQUEST['lang'])) {
			 			
			$query .= " lang " . $_REQUEST['lang'] ;
			$text .= $trad[$lang]['moncompte']['language'].": OK.<br/>\n";
	}
		
	// Reconstruction du flag utilisateur	
	$flag = $_REQUEST['vhost'] + $_REQUEST['axx'] + $_REQUEST['prot'] + $_REQUEST['memos'];
	$flag += ($opts & U_NOPURGE) ? U_NOPURGE : 0;
	$flag += ($opts & U_WANTDROP) ? U_WANTDROP : 0;
	$flag += ($opts & U_FIRST) ? U_FIRST : 0;
	$flag += ($opts & U_ALREADYCHANGE) ? U_ALREADYCHANGE : 0;
	$flag += ($opts & U_ADMBUSY) ? U_ADMBUSY : 0;		
	

	$query .= " flag ".$flag;

	// on affiche les modif s'il y en a eu
	if ($text) {
	echo "<br/>\n<h3>".$trad[$lang]['moncompte']['changes']."</h3><br/>\n";
	echo $text;
    }
	
	if (!$erreur && ($text || ($opts != $flag) ) ) {
		// on ferme la session pour mettre tout à jour

		// on envoit la requete
		$res = w2c_db_query($_SESSION['w2c_login'], $_SESSION['w2c_password'], 'user ' . $_SESSION['w2c_login'] . $query);

		if (!$res['erreur']) {

			session_unregister('w2c_myinfo');
			session_register('w2c_myinfo');
			$_SESSION['w2c_myinfo'] = $res;

			$res = error_pass($_SESSION['w2c_password'], stripslashes($_REQUEST['pass']), stripslashes($_REQUEST['pass2']));		
			if ((!$res[0]) && $res[1]) $_SESSION['w2c_password'] =  stripslashes($_REQUEST['pass']);
		
			$res = error_username($_SESSION['w2c_login'], stripslashes($_REQUEST['username']));
			if ((!$res[0]) && $res[1]) $_SESSION['w2c_login'] =  stripslashes($_REQUEST['username']); 		

		}
		else
			echo $res['erreur'];
 	}
}
$opts = $_SESSION['w2c_myinfo']['options'];
?>

 <form action="moncompte.php" method="post">
 <br> 
 <table width="828" border="0">
   <tr>
     <td width="212"><strong><? echo $trad[$lang]['moncompte']['username']; ?></strong></td>
     <td width="606"><input type="text" name="username" value="<? echo $_SESSION['w2c_login']; ?>"
	   	 <? if ($opts & U_ALREADYCHANGE) echo 'disabled';?> >
       <? echo $trad[$lang]['moncompte']['1modif']; ?></td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['chgpass']; ?></strong></td>
     <td><input type="password" name="pass" value="">
       <? echo $trad[$lang]['moncompte']['chgpassmini']; ?></td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['chgpassverif']; ?></strong></td>
     <td><input type="password"  name="pass2" value="">       </td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['email']; ?></strong></td>
     <td><input type="text" name="mail" value="<? echo $_SESSION['w2c_myinfo']['mail'] ;?>"></td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['language']; ?></strong></td>
     <td><select name="lang">
       <option value="francais" <? if ($_SESSION['w2c_myinfo']['lang'] == "francais") echo "selected" ;?> >Francais</option>
       <option value="english" <? if ($_SESSION['w2c_myinfo']['lang'] == "english") echo "selected" ;?>>English</option>
     </select></td>
    </tr>
 </table>
 <br>
<table width="734" border="0">
  <tr>
    <td width="240"><strong><? echo $trad[$lang]['moncompte']['propsaccess']['0'] ;?></strong></td>
    <td width="240"><strong><? echo $trad[$lang]['moncompte']['protection']['0'];?></strong></td>
    <td width="117"><strong><? echo $trad[$lang]['moncompte']['memos'] ;?></strong></td>
    <td width="119"><strong><? echo $trad[$lang]['moncompte']['vhost'] ;?></strong></td>
    </tr>
  <tr>
    <td><input name="axx" type="radio" 
					value="<? echo U_PREJECT; ?>" <? if ($opts & U_PREJECT) echo 'checked';?> ><? echo $trad[$lang]['moncompte']['propsaccess']['1'] ;?></td>
    <td width="240"><input name="prot" type="radio"
					value="0" <? if (!($opts & U_PNICK) && !($opts & U_PKILL) ) echo 'checked';?>><? echo $trad[$lang]['moncompte']['protection']['1'] ;?></td>
    <td width="117"><input name="memos" type="radio" 
					value="0" <? if (!($opts & U_NOMEMO)) echo 'checked';?>>ON </td>
    <td width="119"><input name="vhost" type="radio" 
					value="<? echo U_WANTX; ?>" <? if ($opts & U_WANTX) echo 'checked';?>>ON </td>
  </tr>
  <tr>
    <td><input name="axx" type="radio" 
					value="<? echo U_POKACCESS; ?>" <? if ($opts & U_POKACCESS) echo 'checked';?> ><? echo $trad[$lang]['moncompte']['propsaccess']['2'] ;?></td>
    <td width="240"><input name="prot" type="radio" 
					value="<? echo U_PNICK; ?>" <? if ($opts & U_PNICK) echo 'checked';?>><? echo $trad[$lang]['moncompte']['protection']['2'] ;?></td>
    <td width="117"><input name="memos" type="radio" 
					value="<? echo U_NOMEMO; ?>" <? if ($opts & U_NOMEMO) echo 'checked';?>>OFF </td>
    <td width="119"><input name="vhost" type="radio" 
					value="0" <? if (!($opts & U_WANTX)) echo 'checked';?>>OFF </td>
    </tr>
  <tr>
    <td><input name="axx" type="radio" 
					value="<? echo U_PACCEPT; ?>" <? if ($opts & U_PACCEPT) echo 'checked';?> ><? echo $trad[$lang]['moncompte']['propsaccess']['3'] ;?></td>
    <td width="240"><input name="prot" type="radio" 
					value="<? echo U_PKILL; ?>" <? if ($opts & U_PKILL) echo 'checked';?>><? echo $trad[$lang]['moncompte']['protection']['3'] ;?></td>
    <td width="117">&nbsp;</td>
    <td width="119">&nbsp;</td>
    </tr>
</table>
<br>
     <input type="submit" name="modif" value="<? echo $trad[$lang]['moncompte']['modif'] ;?>">
	 <input type="submit" name="Reset" value="<? echo $trad[$lang]['moncompte']['raz'] ;?>">
 </form>
 
 <form action="moncompte.php" method="post">

 </form>	 
<?

}
else
	w2c_printerror(1); /* Vous devez être loggué ! */
do_footer();
?>
      
<?php
function w2c_extractmemos($a) {
	$tab = array();
	if($a['memocount'] == 0) return $tab;
	for($i=1;$a['memo'.$i];$i++) {

		list($infos, $text) = explode(':', $a['memo'.$i], 2);
		$infos = trim($infos);
		$text = trim($text);
		list($tab[$i][0], $tab[$i][1], $tab[$i][2]) = explode(' ', $infos, 3);
		$tab[$i][3] = $text;
	}
	return $tab;
}


function error_username($prev, $new)
{
	global $lang;
	global $trad;
	$res = array();
	$res[0] = 0;
	if ( ($new) && ($prev != $new) ) {
		if (w2c_isvalidusername($new) ) {
				$res[1] = $trad[$lang]['moncompte']['error_username']['1'];
				return $res;				
			}
		$res[0] = 1;			
		$res[1] = $trad[$lang]['moncompte']['error_username']['2'];	
		return $res;	
	}
	$res[0] = 0;
	return $res;
}

function error_pass($prev, $new1, $new2)
{
	global $lang;
	global $trad;
	$res = array();
	$res[0] = 0;
	if ($new1 && $new2) {
	   if ($new1 == $new2) {
			if ($prev != $new1) {
		       if (w2c_isvalidpass($new1)) {
			   		$res[1] = $trad[$lang]['moncompte']['error_pass']['1'];
			   		return $res;
				}
			else {
					$res[0] = 1;
					$res[1] = $trad[$lang]['moncompte']['error_pass']['2'];
			   		return $res;
				}
			}
			$res[0] = 0;
			return $res;
		}
		$res[0] = 1;
		$res[1] = $trad[$lang]['moncompte']['error_pass']['3'];
		return $res;
	}
	elseif ((!$new1 && $new2) || ($new1 && !$new2)) {
			$res[0] = 1;
			$res[1] = $trad[$lang]['moncompte']['error_pass']['3'];
			return $res;
		}
	$res[0] = 0;
	return $res;
}

function error_mail($prev, $new)
{
	global $lang;
	global $trad;
	$res = array();
	$res[0] = 0;
	if ( ($new) && ($prev != $new) ) {
		if (w2c_isvalidemail($new) ) {
				$res[1] = $trad[$lang]['moncompte']['error_mail']['1'];
				return $res;				
			}
		$res[0] = 1;			
		$res[1] = $trad[$lang]['moncompte']['error_mail']['2'];	
		return $res;	
	}
	$res[0] = 0;
	return $res;
}
?>

