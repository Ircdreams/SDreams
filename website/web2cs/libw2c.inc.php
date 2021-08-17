<?php

include_once('config.php');

/* Flags Access */
#define A_OP             0x01
#define A_VOICE          0x02
#define A_PROTECT        0x04
#define A_SUSPEND        0x08
#define A_WAITACCESS     0x10

#define U_PKILL	 	0x0001
#define U_PNICK		 0x0002
#define U_WANTDROP	 0x0004
#define U_NOPURGE	 0x0008
#define U_WANTX		 0x0010
#define U_OUBLI		 0x0020
#define U_FIRST 	 0x0040
#define U_NOMEMO	 0x0080
#define U_PREJECT	 0x0100
#define U_POKACCESS	 0x0200
#define U_PACCEPT	 0x0400
#define U_ALREADYCHANGE	 0x0800
#define U_ADMBUSY 		0x1000

#define C_SETWELCOME    0x0001
#define C_JOINED		0x0002
#define C_STRICTOP		0x0004
#define C_LOCKTOPIC		0x0008
#define C_NOBANS		0x0010
#define C_WANTUNREG		0x0020
#define C_NOOPS			0x0040
#define C_AUTOINVITE 	0x0080
#define C_ALREADYRENAME 0x0100
#define C_FLIMIT 		0x0200

#define C_MMSG 		0x001
#define C_MTOPIC 	0x002
#define C_MINV 		0x004
#define C_MLIMIT 	0x008
#define C_MKEY 		0x010
#define C_MSECRET 	0x020
#define C_MPRIVATE 	0x040
#define C_MMODERATE 0x080
#define C_MNOCTRL 	0x100
#define C_MNOCTCP 	0x200
#define C_MOPERONLY 0x400
#define C_MUSERONLY 0x800

define('A_OP',			0x1);
define('A_VOICE',			0x2);
define('A_PROTECT', 		0x4);
define('A_SUSPEND', 	0x8);
define('A_WAITACCESS', 	0x10);

define('U_PKILL',	 			0x0001);
define('U_PNICK', 			0x0002);
define('U_WANTDROP', 		0x0004);
define('U_NOPURGE', 		0x0008);
define('U_WANTX', 			0x0010);
define('U_OUBLI', 			0x0020);
define('U_FIRST', 			0x0040);
define('U_NOMEMO', 			0x0080);
define('U_PREJECT', 			0x0100);
define('U_POKACCESS', 		0x0200);
define('U_PACCEPT', 		0x0400);
define('U_ALREADYCHANGE', 0x0800);
define('U_ADMBUSY',			0x1000);

define ('C_SETWELCOME',    	0x0001);
define ('C_JOINED',			0x0002);
define ('C_STRICTOP',		0x0004);
define ('C_LOCKTOPIC',		0x0008);
define ('C_NOBANS',			0x0010);
define ('C_WANTUNREG',		0x0020);
define ('C_NOOPS',			0x0040);
define ('C_AUTOINVITE', 		0x0080);
define ('C_ALREADYRENAME',0x0100);
define ('C_FLIMIT', 			0x0200);

define ('C_MMSG', 		0x001);
define ('C_MTOPIC', 		0x002);
define ('C_MINV', 			0x004);
define ('C_MLIMIT', 		0x008);
define ('C_MKEY', 		0x010);
define ('C_MSECRET', 	0x020);
define ('C_MPRIVATE', 	0x040);
define ('C_MMODERATE', 0x080);
define ('C_MNOCTRL', 	0x100);
define ('C_MNOCTCP', 	0x200);
define ('C_MOPERONLY', 0x400);
define ('C_MUSERONLY', 0x800);

function w2c_isaccesslevel($string)
{
	if(ereg("^(<|>|=){1}[0-5]?[0-9]{1,2}$", $string)) return 1;
	else return 0;
}

function w2c_isvalidchan($string)
{
	if($string[0] != "#") return 0;
	if(strlen($string) > 30) return 0;
	return ereg("^#[\"\'_a-zA-Z0-9-]{1,30}$", $string);
// Not yet implemented
//	$stop = "^#[A-Z0-9\ #]";
//	if(ereg($stop, $string)) return 0;
	return 1;

}

function w2c_isvaliddesc($string)
{
  $string = ereg_replace(" ", "", rtrim($string));
  if ($string != '') return 1;
  return 0;
}

function w2c_stripircchars($string)
{
  $regex = "\002|\037|\22|\003[0-9]{0,2}(,[0-9]{0,2})?";
  $string = ereg_replace($regex, "", rtrim($string));
  return $string;
}

function w2c_isvalidemail($email)
{
	return ereg("^[_a-zA-Z0-9-]+(\.[_a-zA-Z0-9-]+)*@[a-zA-Z0-9-]+(\.[a-zA-Z0-9-]+)*\.(([0-9]{1,3})|([a-zA-Z]{2,3})|(aero|coop|info|museum|name))$", $email);
}

function w2c_isvalidusername($username)
{
	return ereg("^([a-zA-Z]|_|`|\\{|\\[|]|}|\\||\\\\|\\^){1}([a-zA-Z0-9]|_|`|\\{|\\[|]|}|\\||\\\\|-|\\^){1,".(NICKLEN - 1)."}$",$username);
}

function w2c_isvalidpass($mdp)
{
	return ereg("^[^[:space:]]{7,15}$", $mdp);
}

function w2c_generatepass()
{
		for($j=0;$j<=10;$j++)
		{
			do $i = rand(1,9999) %57 + 65;
			while($i >= 91 && $i <= 96);
			$c .= chr($i);
		}
		return $c;
}

function w2c_char2html($string)
{
	return w2c_stripircchars($string);
	//return ircg_html_encode($string);
}

function w2c_checkerr($get)
{
	return ereg(ERR, $get);
}

function w2c_db_query($nick, $pass, $query)
{

        if($fd = fopen('/home/coderz/public_html/w2c/web2cs/w2c.log', 'a'))
        {
                $log = date("d/m/y - H:i:s") . ' - ' . $nick . ' - ' . 
			getenv('REMOTE_ADDR') . ' - ' .strtok($query, " "). "\n";
                fputs($fd, $log);
                fclose($fd);
        }


/*
 * array avec résultat ou élément 'erreur' set.
 */
	$fp = fsockopen(SERVEUR, PORT, $errno, $errstr, 30);

	if ($fp == NULL) /* connexion a échoué*/
	{

		return array('erreur' => 'Connexion impossible ...');
	}

	fputs($fp, "PASS ".PASSWD."\r\nLOGIN ".$nick." ".$pass." ".getenv('REMOTE_ADDR')."\r\n".$query."\r\n");

	/* $tab = array(); */
	stream_set_timeout($fp, 10);
        while($get = htmlentities(trim(fgets($fp, 1024))))
	{
		//echo "GET" .$get. '<br />';
		$res = strtok($get, ' ');
	    //    echo "RES" .$res. '<br />';
		if($res == "OK") break;
		$tab[$res] = substr(strchr($get, ' '), 1);

		if($res == "ERROR")
		{
			$tab['erreur'] = $tab[$res];
			 break;
		}
		if($get[0] == '=') { /* parse different items */
			$fl = explode(' ', substr($get, 1));

			foreach($fl as $st) {
				$tmp = explode('=', $st);
				//echo "ADD ".$tmp[0]." = ".$tmp[1] .'<br />';
				$tab[$tmp[0]] = $tmp[1];
			}
			unset($tab[$res]);
		}
	}
	fclose($fp);
	return $tab ? $tab : ( ($res == "OK") ?  $tab : array('erreur' => 'Connexion impossible ...') );
}

function w2c_db_chan_isreg($nick, $pass, $chan)
{
	$res = array();
	if(!$chan || $chan[0] != '#') $res['erreur'] = "Nom de salon invalide.";
	else $res = db_query($nick, $pass, "channel ".$chan);
	return $res;
}

/* db_getchaninfo(,, "#la", "all"|"topic modes owner");
 */
function w2c_db_getchaninfo($nick, $pass, $chan, $arg)
{
	$res = array();
	if(!$chan || $chan[0] != '#') $res['erreur'] = "Nom de salon invalide.";
	else $res = w2c_db_query($nick, $pass, "channel ".$chan." info ".$arg);
	return $res;
}

function w2c_db_getaccess($nick, $pass, $chan)
{
	$res = array();
	if(!$chan || $chan[0] != '#') $res['erreur'] = "Nom de salon invalide.";
	else {
		$tmp = "channel ".$chan." info ";
		
		$tmp = $tmp . " access ";
	    $res = w2c_db_query($nick, $pass, $tmp);
	}
	return $res;
}

function w2c_register($nick, $pass, $email)
{
	$fp = fsockopen(SERVEUR, PORT, $errno, $errstr, 30);

	if ($fp == NULL) /* connexion a échoué*/
	{
		/* echo "$errstr ($errno)"; */
		return "Connexion impossible ...";
	}

	fputs($fp, "PASS ".PASSWD."\r\nREGISTER ".$nick." ".$email." ".$pass." ". getenv('REMOTE_ADDR')."\r\n");

	/* $tab = array(); */
	stream_set_timeout($fp, 2);
	while($get = trim(fgets($fp, 512)))
	{
		//echo $get. '<br />';
		$res = strtok($get, ' ');
		if($res == "OK") break;
		/* En cas d'erreur */
                $tab[$res] = substr(strchr($get, ' '), 1);

                if($res == "ERROR")
                {
                        $tab['erreur'] = $tab[$res];
                         break;
                }

                if($get[0] == '=') { /* parse different items */
                        $fl = explode(' ', substr($get, 1));

                        foreach($fl as $st) {
                                $tmp = explode('=', $st);
                                //echo "ADD ".$tmp[0]." = ".$tmp[1] .'<br />';
                                $tab[$tmp[0]] = $tmp[1];
                        }
                        unset($tab[$res]);
                }

		return substr(strchr($get, ' '), 1);
	}
	fclose($fp);
 	return $tab ? $tab : ( ($res == "OK") ?  $tab : array('erreur' => 'Connexion impossible ...') );
}

function w2c_accessflag2str($i,$j=0)
{
/* define('A_OP', 0x1);
define('A_VOICE', 0x2);
define('A_PROTECT', 0x4);
define('A_SUSPEND', 0x8);
define('A_WAITACCESS', 0x10); */
	global $trad,$lang;
	if(!($trad || $lang)) $j = 0;
	$str = '';
	if(!$j) {
		if($i & A_OP) $str .= 'o';
		if($i & A_PROTECT) $str .= 'p';
		if($i & A_SUSPEND) $str .= 's';
		if($i & A_VOICE) $str .= 'v';
		if($i & A_WAITACCESS) $str .= 'w';
	} else {
		if($i & A_OP) $str .= '<acronym title="'.$trad[$lang]['acronym']['o'].'">o</acronym>';
		if($i & A_PROTECT) $str .= '<acronym title="'.$trad[$lang]['acronym']['p'].'">p</acronym>';
		if($i & A_SUSPEND) $str .= '<acronym title="'.$trad[$lang]['acronym']['s'].'">s</acronym>';
		if($i & A_VOICE) $str .= '<acronym title="'.$trad[$lang]['acronym']['v'].'">v</acronym>';
		if($i & A_WAITACCESS) $str .= '<acronym title="'.$trad[$lang]['acronym']['w'].'">w</acronym>';
	}
	return $str;
}

/* 
   $tab[$i][0] = chan
		[1] = level
		[2] = flag
		[3] = lastseen
		[4] = infoline
*/

function w2c_extractaccess($a) {
	$tab = array();
	if($a['accesscount'] == 0) return $tab;
	for($i=1;$a['access'.$i];$i++) {
		list($infos, $text) = explode(':', $a['access'.$i], 2);
		$infos = trim($infos);
		$text = trim($text);
		$j = 0;
		list($tab[$i][$j++], $tab[$i][$j++], $tab[$i][$j++], $tab[$i][$j++]) = explode(' ', $infos, 4);
		$tab[$i][$j++] = $text;
	}
	return $tab;
}

function user_access($chan ,$tab)
{
	$total = $tab['accesscount'];
	$res = array();
	$res[0] = 0;
	$res[1] = 0;	
	$res[2] = 0;		
	$i = 1;
	while($i <= $total) {
			$info = strchr($tab['access'.$i], ':');
			$ac = explode(' ', $tab['access'.$i]);
			if ($ac[0] == $chan) {
				$res[0] = 1;
				$res[1] = $ac[1];
				$res[2] = $ac[2];			
			}
			$i++;
		}
	return $res;
}

function user_isowner($tab)
{
	$total = $tab['accesscount'];

	$i = 1;
	while($i <= $total) {
			$info = strchr($tab['access'.$i], ':');
			$ac = explode(' ', $tab['access'.$i]);
			if ($ac[1] == 500 ) {
				return $ac[0];
			}
			$i++;
		}
	return ;
}
?>
