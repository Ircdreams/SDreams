<?
$seed = $_REQUEST['seed'];
$num = $_REQUEST['num'];
require("config.php");
$max_num = 2;

$db_link = mysql_connect ($hostname, $mysqluser, $mysqlpassword);
mysql_select_db ($db, $db_link);

$sql = 'SELECT ip , _'.$num.' FROM coderegister where seed="'.$seed.'"';
$res = mysql_query($sql);

$tab = mysql_fetch_array($res);
mysql_close ($db_link);
mysql_free_result($res);

$im = imagecreatefromgif("img/".$tab['_'.$num].".".rand(1,$max_num).".GIF");

header("Cache-Control: no-store, no-cache, must-revalidate"); // HTTP/1.1
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");
header("Content-type: image/jpeg");

imagejpeg($im);
?>