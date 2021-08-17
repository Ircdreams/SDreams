<?php
include('config.inc.php');
include("libw2c.inc.php");


do_head();

if(w2c_checkvars())
	echo $trad[$lang]['register']['alreadylogged'];
else
{ /* Un erreur ? 1 ere utilisation de la page ? */
?>
<h2>Register</h2>
<br />
<br />
<?php
if(!isset($_REQUEST['erreur']) || $_REQUEST['erreur'] == '' || $_REQUEST['erreur'] == -1)
{

	echo $trad[$lang]['register']['registered']['0'].$_REQUEST['login'].$trad[$lang]['register']['registered']['1'];

}
else {

echo $trad[$lang]['register']['doc'].'<div style="text-align : center;"><form action="register2.php" method="POST">';
if($_REQUEST['erreur']) echo '<tr><td align=\"center\" colspan=\"2\">'.w2c_checkregistererror($_REQUEST['erreur']).'</td></tr>';
php?>
<table width="478" border="0" align="center">

<tr>
	<td align="right">Username :</td>
	<td width="266" align="left"><input type="text" name="login" value="<? echo stripslashes($_REQUEST['login']);?>"></td></tr>
<tr>
	<td align="right">Email :</td>
	<td align="left"><input type="text" name="email" value="<? echo stripslashes($_REQUEST['email']); ?>"></td></tr>
<tr align="center">
  <td colspan="2">

<?
include("gene_code.php");

for($i=1; $i < 6; $i++)
  	echo '<img alt="Code de sécurité" src="code_gfx.php?num='.$i.'&seed='.$seed.'">';
?>

  </td>
</tr>
<tr>
  <td align="right"><? echo $trad[$lang]['register']['copyseed']; ?></td>
  <td align="left"><input type="text" name="secu"></td>
</tr>
<tr align="center">
	<td colspan="2"><input type="hidden" name="seed" value="<? echo $seed;?>">
	<input type="submit" name="Register"></td>
</tr>
</table>
</form>
</div>
<?php
} }
do_footer();

function w2c_setregister($login, $pass, $email)
{
//mail($email ,"Votre Password pour irc.jeux.fr","votre login est : $login \nvotre pass est : $pass\n");

}


function w2c_checkregistererror($i)
{
	global $trad,$lang;
	return isset($trad[$lang]['register']['error'][$i]) ? $trad[$lang]['register']['error'][$i] : $trad[$lang]['register']['error'][0];
}

php?>
