<?php
include('config.inc.php');
include("libw2c.inc.php");
$login = stripslashes($_REQUEST['login']);
if(!$login) $login = $_SESSION['w2c_login'];

$xsum = md5(substr($_REQUEST['password'],0,2).$_REQUEST['password']);

$pass = sprintf("%x%x", base_convert(substr($xsum, 0,8),16,10)+ base_convert(substr($xsum, 16,8),16,10),
base_convert(substr($xsum, 8,8),16,10)+ base_convert(substr($xsum, 24,8),16,10));

if(!$pass) $pass = $_SESSION['w2c_password'];

$_SESSION['w2c_myinfo'] = w2c_db_query($login, $pass, 'user ' . $login . ' info');
$_SESSION['w2c_myaccess']['accesscount'] = $_SESSION['w2c_myinfo']['accesscount'];
$_SESSION['w2c_mymemo']['memocount'] = $_SESSION['w2c_myinfo']['memocount'];



$i = 1;
while ($i <= $_SESSION['w2c_myaccess']['accesscount']) {
	$_SESSION['w2c_myaccess']['access'.$i] = $_SESSION['w2c_myinfo']['access'.$i];
	$i++;
}
$i = 1;
while ($i <= $_SESSION['w2c_mymemo']['memocount']) {
	$_SESSION['w2c_mymemo']['memo'.$i] = $_SESSION['w2c_myinfo']['memo'.$i];
	$i++;
}

$_SESSION['w2c_login'] = $login;
$_SESSION['w2c_password'] = $pass;
$_SESSION['w2c_tmp'] = 1;
$_SESSION['w2c_lang'] = $lang;

$ref = $_SERVER['HTTP_REFERER'] ?  $_SERVER['HTTP_REFERER'] : './login.php';
header('Location: '.$ref);
?>
