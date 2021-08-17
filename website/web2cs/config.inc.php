<?php

error_reporting(E_ALL& ~E_NOTICE);
session_start();

session_register('w2c_login');
session_register('w2c_password');
session_register('w2c_myaccess');
session_register('w2c_myinfo');
session_register('w2c_mymemo');
session_register('w2c_tmp');
session_register('w2c_lang');
if($_SESSION['w2c_tmp']) $fromupdate = 1;
unset($_SESSION['w2c_tmp']);


include_once("lang.php");
if (isset( $_REQUEST['lang'] ) ) $lang = $_REQUEST['lang'];
elseif (isset( $_SESSION['w2c_myinfo']['lang'] ) ) $lang = $_SESSION['w2c_myinfo']['lang'];
elseif(isset($_SESSION['w2c_lang'])) $lang = $_SESSION['w2c_lang'];
else $lang="francais";


function w2c_printerror($i) {
	global $trad;
	global $lang;
	switch ($i) {
		case 0: /* Already logged */
			echo $trad[$lang]['config.inc']['printerror'][1];
			break;
		case 1: /* Not logged in */
			echo $trad[$lang]['config.inc']['printerror'][2];
			break;
		case 2: /* Not admin ! */
			echo $trad[$lang]['config.inc']['printerror'][3];
			break;
		default: /* Error ?! */
			echo $trad[$lang]['config.inc']['printerror'][0];
	}
	return;
}



function w2c_checkvars()
{
	global $_SESSION;

	if(!($_SESSION['w2c_login'] && $_SESSION['w2c_password'] && $_SESSION['w2c_myinfo'] && !$_SESSION['w2c_myinfo']['erreur']))  return 0;
	return 1;
}

function w2c_checkadmin()
{
	global $_SESSION;
	if($_SESSION['w2c_myinfo']['level'] >= 2) return 1;
	return 0;
}

function do_menu($i,$j=0)
{
	global $trad;
	global $lang;
	if($i)
		return "<li><a href=\"index.php\">".$trad[$lang]['config.inc']['domenu']['home']."</a></li>"
			."<li><a href=\"login.php\">".$trad[$lang]['config.inc']['domenu']['login']."</a></li>"
			."<li><a href=\"register.php\">".$trad[$lang]['config.inc']['domenu']['register']."</a></li>"
			."<br /><a href=\"setlang.php?a=1\" class=\"lang\"><img src=\"iflag_fra.gif\" alt=\"Ce site en français\" /></a> &nbsp;"
			."<a href=\"setlang.php?a=2\" class=\"lang\"><img src=\"iflag_uk.gif\" alt=\"This website in english\" /></a>";
	else
		return "<li><a href=\"index.php\">".$trad[$lang]['config.inc']['domenu']['home']."</a></li>"
			."<li><a href=\"recapmoncompte.php\">".$trad[$lang]['config.inc']['domenu']['recapaccount']."</a></li>"
			."<li><a href=\"moncompte.php\">".$trad[$lang]['config.inc']['domenu']['account']."</a></li>"
			//."<li><a href=\"listchan.php\">".$trad[$lang]['config.inc']['domenu']['manage']."</a></li>"
			."<li><a href=\"chaninfo.php\">".$trad[$lang]['config.inc']['domenu']['chaninfo']."</a></li>"
			."<li><a href=\"access.php\">".$trad[$lang]['config.inc']['domenu']['access']."</a></li>"
			."<li><a href=\"mesmemos.php\">".$trad[$lang]['config.inc']['domenu']['memos']."</a></li>"
			. ($j ? ("<li><a href=\"admin.php\">".$trad[$lang]['config.inc']['domenu']['admin']."</a></li>") : "")
			. ($j ? ("<li><a href=\"adminstats.php\">".$trad[$lang]['config.inc']['domenu']['adminstats']."</a></li>") : "")
			."<li><a href=\"delog.php\">".$trad[$lang]['config.inc']['domenu']['logout']."</a></li>";
}

function do_content()
{
	global $p, $w2c_login, $w2c_password;
	if(!$p) $p = "index";
	$p .= ".inc.php";
	if(!ereg("/",$p) && is_file($p)) include($p);
	else include("hack.inc.php");
	return;
}

function do_head()
{
	global $trad;
	global $lang;
echo '
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="fr" lang="fr">
<head>
	<!-- début meta -->
	<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
	<meta http-equiv="Content-Script-Type" content="text/javascript" />
	<meta http-equiv="Content-Style-Type" content="text/css" />
	<meta http-equiv="Content-Language" content="fr" />
	<title>Web2Cs - Passerelle Web pour Z</title>
	<meta name="revisit-after" content="15 days" />

	<meta name="robots" content="index,follow" />
	<meta name="DC.Language" content="fr" scheme="RFC1766" />
	<meta name="DC.Identifier" content="Page non trouvée" />
	<meta name="DC.IsPartOf" content="article" />
	<meta name="DC.Publisher" content="kouak" />
	<meta name="DC.Rights" content="kouak http://www.kouak.org" />
	<meta name="DC.Creator" content="kouak" />
	<!-- fin meta -->
	<!-- début link -->

	<link rel="start" title="Accueil" href="/" />
	<link rel="shortcut icon" type="images/x-icon" href="/favicon.ico" />
	<link rel="stylesheet" type="text/css" href="css/style.css" media="screen" title="Normal" />

	<!-- fin link -->
</head>
<body>
<div id="page">
	<div id="top"><span class="left">Service Web2CS</span><span class="right"><a href="http://ircube.org"><img src="http://ircube.org/modules/gfx/banniere02.gif" alt="IRCube.org" /></a>
	<br />';
	echo $_SESSION['w2c_login'] ? $trad[$lang]['logged']." ".$_SESSION['w2c_login'] : $trad[$lang]['nologged'];
	echo '</span></div><div id="menu"><ul>';
	echo  do_menu(!w2c_checkvars(), w2c_checkadmin());
	echo '</ul></div><div id="content">';
	return;
}

function do_footer()
{
	echo '
	</div>

	<div id="footer">© Staff IRCube.org</div>
</div>
</body>
</html>';
	return;
}
?>
