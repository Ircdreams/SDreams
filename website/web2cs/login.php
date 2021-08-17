<?php
include("libw2c.inc.php");
include("config.inc.php");

if(w2c_checkvars() && !$fromupdate)
	w2c_printerror(0);
else {
//	if($_REQUEST['login'] && $_REQUEST['password'] && ($gn = w2c_db_query($_REQUEST['login'], $_REQUEST['password'], 'user ' . $_REQUEST['login'] . ' info')) && !$gn['erreur'])
	if($_SESSION['w2c_myinfo'] && !$_SESSION['w2c_myinfo']['erreur'])
		header('Location: ./recapmoncompte.php');
	else {
		$_SESSION['w2c_login'] = '';
		do_head();
?>
<h2>Login</h2>
<br />
<br />
<div style="text-align : center;">
<form action="updatesession.php" method="POST">
<table width="250" border="0" align="center">
<?php
	$i = 0;
	if($_SESSION['w2c_myinfo']['erreur'] == 'Déjà logué') $i = 1;
	elseif($_SESSION['w2c_myinfo']['erreur'] == "Username inexistant") $i = 2;
	elseif($_SESSION['w2c_myinfo']['erreur'] == "Username suspendu") $i = 3;
	elseif($_SESSION['w2c_myinfo']['erreur'] == "Mauvais pass") $i = 4;
	elseif($_SESSION['w2c_myinfo']['erreur']) $i = 5;

	if($i != 0) echo w2c_checkloginerror($i);

	session_unregister('w2c_login');
	session_unregister('w2c_password');
	session_unregister('w2c_myaccess');
	session_unregister('w2c_myinfo');
	session_unregister('w2c_mymemo');
	session_unregister('w2c_tmp');

?>
<tr><td>Username :</td><td><input type="text" name="login"></td></tr>
<tr><td>Password :</td><td><input type="password" name="password"></td></tr>
<tr><td colspan="2" align="center"><input type="submit" value="Login !"></td></tr>
</table>
</form>

<?php
echo $trad[$lang]['login']['doc']."</div>";


do_footer();
} }

function w2c_checkloginerror($i)
{
	global $trad,$lang;
	return (isset($trad[$lang]['login']['error'][$i])) ? $trad[$lang]['login']['error'][$i] : $trad[$lang]['login']['error'][0];
}
php?>
