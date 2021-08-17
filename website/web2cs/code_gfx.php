<?
$seed = $_REQUEST['seed'];
$num = $_REQUEST['num'];
include_once("config.php");
$max_num = get_config('max_num_secu');

$db_link = db_connect();


$sql = 'SELECT ip , _'.$num.' FROM coderegister where seed="'.$seed.'"';
$res = mysql_query($sql);

$tab = mysql_fetch_array($res);
mysql_free_result($res);

db_close($db_link);

$im = imagecreatefromgif("img/".$tab['_'.$num].".".rand(1,$max_num).".GIF");

header("Cache-Control: no-store, no-cache, must-revalidate"); // HTTP/1.1
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");

if (function_exists("imagegif")) {
header("Content-type: image/gif");
imagegif($im);
}
elseif (function_exists("imagejpeg")) {
header("Content-type: image/jpeg");
imagejpeg($im);
}
elseif (function_exists("imagepng")) {
header("Content-type: image/png");
imagepng($im);
} elseif (function_exists("imagewbmp")) {
header("Content-type: image/vnd.wap.wbmp");
imagewbmp($im);
} else {
die("Pas de support graphique avec PHP sur ce serveur");
}

?>
