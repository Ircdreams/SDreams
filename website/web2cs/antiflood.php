<?

function flood()
{

	
	include_once("config.php");
	include_once("dblib.php");

	$max_cmd_per_min = get_config('max_cmd_per_min');

	$db_link = db_connect();

	$ip = $_SERVER["REMOTE_ADDR"] ;
	// recup heure actuelle	
	$tps = time();

	$heure_max = $tps - 60;

	$sql = "DELETE FROM antiflood where ip='".$ip."' AND time < '".$heure_max."'";
	$req = mysql_query($sql);


	$sql = "SELECT flood FROM antiflood where ip='".$ip."';";
	$req = mysql_query($sql);

	$flood = 1;		
	if ( mysql_num_rows($req) ) { // a deja flooder
	   $tab = mysql_fetch_array($req);

	   $flood = $tab['flood'] + 1;
	   $sql = "UPDATE antiflood SET flood='".$flood."' , time='".$tps."' WHERE ip='".$ip."';";
	   
	}
	else {
		$sql = "INSERT INTO antiflood  SET ip='$ip' , flood='$flood' , time='".$tps."' ;";
	}

	$req = mysql_query($sql);
	db_close();
	
	if ($flood > $max_cmd_per_min)
	   return 1;
	else
	   return 0;
}

?>