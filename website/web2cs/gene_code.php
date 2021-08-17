<?

include_once("config.php");
include_once("dblib.php");

$ip = $_SERVER["REMOTE_ADDR"] ;
// recup heure actuelle	
$tps = time();

$heure_max = $tps - get_config('time_max_secu');

$db_link = db_connect();
$sql = "DELETE FROM coderegister where ip=\"$ip\" OR time < '$heure_max' ;";
$req = mysql_query($sql);

$seed = rand(0,999999);
$sql = "INSERT INTO coderegister SET ip='$ip', seed='$seed', time='$tps', ".
		"_1='".rand(1,9)."', _2='".rand(0,9)."', _3='".rand(0,9).
		"', _4='".rand(0,9)."', _5='".rand(0,9)."' ;";

$req = mysql_query($sql);
db_close();

?>
