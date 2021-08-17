<?
include_once("config.php");
include_once("dblib.php");

$erreur = 0;
$db_link = db_connect();
$sql = "SELECT _1, _2, _3, _4, _5 FROM coderegister where seed='".$seed."'";

$req = mysql_query($sql);
$tab = mysql_fetch_array($req);
mysql_free_result($req);	

if (!$tab) $erreur = 1;
else
	$code = $tab['_1'].$tab['_2'].$tab['_3'].$tab['_4'].$tab['_5'] ;
	
$sql = "DELETE FROM coderegister where seed='".$seed."'";
$req = mysql_query($sql);
	
// recup heure actuelle	
$tps = time();

// recup l'ip
$ip = $_SERVER["REMOTE_ADDR"] ;

// on purge la base
$heure_max = $tps - get_config('time_max_reg');
$sql = 'DELETE FROM register where time < "'.$heure_max.'"';
mysql_query($sql);

// on cherche pour la meme ip dans la base
$query = "SELECT * FROM register WHERE ip='$ip' ";
$res = db_query($query);
$ok = mysql_num_rows($res);
mysql_free_result($res);	
	
db_close();
?>