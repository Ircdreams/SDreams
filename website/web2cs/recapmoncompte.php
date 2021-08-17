<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{
$opts = $_SESSION['w2c_myinfo']['options'];
?>

<h2><? echo $trad[$lang]['recapmoncompte']['titre']; ?></h2>
<?php
	$memos = w2c_extractmemos($_SESSION['w2c_mymemo']);
    /*$i commence a 1 car dans w2c_extractmemos($a)  $tab commence a 1*/
	for($j=0,$i=1;$memos[$i];$i++) if($memos[$i][2] == 0) $j++;

	if ( $j > 0 ) echo $trad[$lang]['recapmoncompte']['newmemos']['1'].$j
						.$trad[$lang]['recapmoncompte']['newmemos']['2'].
						 (($j > 1) ? $trad[$lang]['recapmoncompte']['newmemos']['3'] : $trad[$lang]['recapmoncompte']['newmemos']['4']) .
						  (($j > 1) ? $trad[$lang]['recapmoncompte']['newmemos']['5'] : $trad[$lang]['recapmoncompte']['newmemos']['6']);
	else echo $trad[$lang]['recapmoncompte']['nonewmemos'];
	
?>
 <br> 
 <table width="828" border="0">
   <tr>
     <td width="212"><strong><? echo $trad[$lang]['moncompte']['username']; ?> :</strong></td>
     <td width="606"><? echo $_SESSION['w2c_login']; ?></td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['email']; ?> :</strong></td>
     <td><? echo $_SESSION['w2c_myinfo']['mail'] ;?></td>
    </tr>
   <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['language']; ?> :</strong></td>
     <td><? echo $_SESSION['w2c_myinfo']['lang']; ?></td>
    </tr>
       <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['propsaccess']['0'] ;?> :</strong></td>
     <td><? if ($opts & U_PREJECT) echo $trad[$lang]['moncompte']['propsaccess']['1']; 
     		   elseif ($opts & U_POKACCESS) echo $trad[$lang]['moncompte']['propsaccess']['2'];
		   else echo $trad[$lang]['moncompte']['propsaccess']['3'];
     ?></td>
    </tr>
     <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['protection']['0'];?> :</strong></td>
     <td><? if (!($opts & U_PNICK) && !($opts & U_PKILL) )  echo $trad[$lang]['moncompte']['protection']['1']; 
     		   elseif ($opts & U_PNICK) echo $trad[$lang]['moncompte']['protection']['2'];
		   else echo $trad[$lang]['moncompte']['protection']['3'];
     ?></td>
    </tr>
     <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['memos']; ?> :</strong></td>
     <td><? if($opts & U_NOMEMO)   echo $trad[$lang]['recapmoncompte']['memos']['refuse']; 
		   else echo $trad[$lang]['recapmoncompte']['memos']['accept'];
     ?></td>
    </tr>
      <tr>
     <td><strong><? echo $trad[$lang]['moncompte']['vhost']; ?> :</strong></td>
     <td><? if($opts & U_WANTX)   echo $trad[$lang]['recapmoncompte']['vhost']['active']; 
		   else echo $trad[$lang]['recapmoncompte']['vhost']['unactive'];
     ?></td>
    </tr>
      <tr>
     <td valign="top"><strong><? echo $trad[$lang]['recapmoncompte']['access']; ?> :</div></strong></td>
     <td>
<?
$access_r =  w2c_extractaccess($_SESSION['w2c_myaccess']);
if(!($access_r[1])) /* No access */
	echo $trad[$lang]['recapmoncompte']['noaccess'];
else /*One or more access */
{
?>
<table class="tble">
<tr><td class="titre"><? echo $trad[$lang]['recapmoncompte']['channel']; ?></td><td class="titre"><? echo $trad[$lang]['recapmoncompte']['level']; ?></td><td class="titre"><? echo $trad[$lang]['recapmoncompte']['flags']; ?></td></tr>
<?
	for($i=1;$access_r[$i];$i++)
		echo "<tr><td>"
			 .$access_r[$i][0] /* Chan */
			 ."</td><td>"
			 .$access_r[$i][1] /* Level */
			 ."</td><td>"
			 .(strlen(($a = w2c_accessflag2str($access_r[$i][2], 1))) ? $a : $trad[$lang]['recapmoncompte']['noflag'])
			 ."</td></tr>\n";
}
?>
</table>
    </td>
    </tr>
 </table>
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


?>