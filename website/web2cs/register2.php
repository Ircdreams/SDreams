<?php
include('config.inc.php');
include("libw2c.inc.php");
//include("config.php");

do {

$login = stripslashes($_REQUEST['login']);
$email = stripslashes($_REQUEST['email']);
$secu= $_REQUEST['secu'];
$seed= $_REQUEST['seed'];
include ("get_secu.php");
if ($erreur)  { $err=7; break; }
elseif($secu != $code) { $err=8; break; }


if ($ok != 0) {// si y'en a erreur 
	$err=6;
}
else
{

	if(!w2c_isvalidusername($login)) { $err=1; break; }
	if(!w2c_isvalidemail($email)) { $err=2; break; }
	
	$pass = w2c_generatepass();

	$res = w2c_register($login, $pass, $email);
	if(!isset($res['erreur']) || $res['erreur'] == '') {
		$err = -1; 
		// sinon on la rajoute
		$link = db_connect();
		$sql = 'INSERT INTO register VALUES("'.$ip. '","'.$tps.'")';
		$req = mysql_query($sql);
		db_close();
	}
	elseif($res['erreur'] == "USER_INUSE") { $err = 3;db_close(); }
	elseif($res['erreur'] == "USER_FORBIDDEN") { $err=4;db_close(); }
	elseif($res['erreur'] == "USER_MAILINUSE") {$err=5;db_close();}
}

}while(0);
if ($err == -1)
	mail($email,$trad['english']['register']['mail']['title'],$trad['english']['register']['mail']['0']." $login .\n".$trad['english']['register']['mail']['1']." $pass .\n");

header('Location: ./register.php?erreur='.$err.'&login='.$login.'&email='.$email);
?>
