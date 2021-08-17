<?
include('config.inc.php');
include("libw2c.inc.php");

$lang = 'francais';
if($_REQUEST['a'] == 2) $lang = 'english';

$_SESSION['w2c_lang'] = $lang;

$ref = $_SERVER['HTTP_REFERER'] ?  $_SERVER['HTTP_REFERER'] : './login.php';
header('Location: '.$ref);
?>