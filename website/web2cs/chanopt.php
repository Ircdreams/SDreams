<?php
include('config.inc.php');
include("libw2c.inc.php");
do_head();
if(w2c_checkvars())
{

	$axx = user_access($_REQUEST['channel'] ,$_SESSION['w2c_myaccess']);
	echo "<h2>".$trad[$lang]['chanopt']['titre']." ".$_REQUEST['channel']."</h2><br />\n";
	if ((!$axx[0]) || ($axx[2] & A_WAITACCESS)) {

		echo $trad[$lang]['chanopt']['noaccess'];
	}
	else
	{
	?>

	<?
		$chaninfo = w2c_db_getchaninfo($_SESSION['w2c_login'], $_SESSION['w2c_password'], $_REQUEST['channel'], 'all');
	?>
	<?
		if ($_REQUEST['Modifier'] ) {
		
		
			$nbopts = 7;
			echo "Modifications :<br />\n";

			$i = 1;
			while ($i <= $nbopts) {
				$opt += $_REQUEST['opt'.$i];
				$i++;
			}
			echo "OPTION :".$opt."<br />\n";				
						
		}
	?>
	<form action="chanopt.php" method="post">

    	<table width="792" border="0">
          <tr>
            <td><? echo $trad[$lang]['chanopt']['channame']; ?></td>
            <td><input type="text" name="channel" value="<? echo $_REQUEST['channel']; ?>"
	   	 			<? if ($chaninfo['options'] & C_ALREADYRENAME) echo 'disabled';?> ></td>
          </tr>
          <tr>
            <td width="290"><? echo $trad[$lang]['chaninfo']['owner']; ?></td>
            <td width="492"><input type="text" name="owner" value="<? echo $chaninfo['owner']; ?>" disabled></td>
          </tr>
          <tr>
            <td><? echo $trad[$lang]['chaninfo']['desc']; ?></td>
            <td><textarea name="desc" cols="40"><? echo $chaninfo['description']; ?></textarea></td>
          </tr>
          <tr>
            <td><? echo $trad[$lang]['chaninfo']['topic']; ?></td>
            <td><textarea name="desc" cols="40"><? echo $chaninfo['topic']; ?></textarea></td>
          </tr>
          <tr>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td><? echo $trad[$lang]['chaninfo']['modes']; ?></td>
            <td>			 
				<? if ($chaninfo['modes'] & C_MMSG) echo 'n'; ?>
			    <? if ($chaninfo['modes'] & C_MTOPIC) echo 't'; ?>
				<? if ($chaninfo['modes'] & C_MNOCTRL) echo 'c'; ?>
				<? if ($chaninfo['modes'] & C_MMODERATE) echo 'm'; ?>
				<? if ($chaninfo['modes'] & C_MINV) echo 'i'; ?>
				<? if ($chaninfo['modes'] & C_MPRIVATE) echo 'p'; ?>
				<? if ($chaninfo['modes'] & C_MSECRET) echo 's'; ?>
				<? if ($chaninfo['modes'] & C_MNOCTCP) echo 'C'; ?>
				<? if ($chaninfo['modes'] & C_MUSERONLY) echo 'r'; ?>
			    <? if ($chaninfo['modes'] & C_MKEY) echo 'k'; ?>
			    <? if ($chaninfo['modes'] & C_MLIMIT) echo 'l'; ?>	
				<? if ($chaninfo['modes'] & C_MKEY) echo " ".$chaninfo['key'];?>
	  			<? if ($chaninfo['modes'] & C_MLIMIT) echo " ".$chaninfo['limit'];?>	
			</td>
          </tr>
          <tr>
            <td><? echo $trad[$lang]['chaninfo']['defmodes']; ?></td>
            <td>
				<? if ($chaninfo['defmodes'] & C_MMSG) echo 'n'; ?>
			    <? if ($chaninfo['defmodes'] & C_MTOPIC) echo 't'; ?>
				<? if ($chaninfo['defmodes'] & C_MNOCTRL) echo 'c'; ?>
				<? if ($chaninfo['defmodes'] & C_MMODERATE) echo 'm'; ?>
				<? if ($chaninfo['defmodes'] & C_MINV) echo 'i'; ?>
				<? if ($chaninfo['defmodes'] & C_MPRIVATE) echo 'p'; ?>
				<? if ($chaninfo['defmodes'] & C_MSECRET) echo 's'; ?>
				<? if ($chaninfo['defmodes'] & C_MNOCTCP) echo 'C'; ?>
				<? if ($chaninfo['defmodes'] & C_MUSERONLY) echo 'r'; ?>
			    <? if ($chaninfo['defmodes'] & C_MKEY) echo 'k'; ?>
			    <? if ($chaninfo['defmodes'] & C_MLIMIT) echo 'l'; ?>	
				<? if ($chaninfo['defmodes'] & C_MKEY) echo " ".$chaninfo['defkey'];?>
	  			<? if ($chaninfo['defmodes'] & C_MLIMIT) echo " ".$chaninfo['deflimit'];?>		
			</td>
          </tr>
          <tr>
            <td><? echo $trad[$lang]['chaninfo']['options']['0']; ?></td>
            <td>&nbsp;</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['bantype']; ?></td>
            <td><select name="bantype">
              <option value="1" <? if ($chaninfo['bantype'] == 1) echo 'selected';?>>*!*ident@*.host</option>
              <option value="2" <? if ($chaninfo['bantype'] == 2) echo 'selected';?>>*!*ident@*</option>
              <option value="3" <? if ($chaninfo['bantype'] == 3) echo 'selected';?>>*nick*!*@*</option>
              <option value="4" <? if ($chaninfo['bantype'] == 4) echo 'selected';?>>*!*@host</option>
            	</select>
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['bantime']; ?></td>
            <td><input name="bantime" type="text" size="3" value="<? echo $chaninfo['bantime']; ?>"></td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['banlevel']; ?></td>
            <td><input name="banlevel" type="text" size="3" value="<? echo $chaninfo['banlevel']; ?>"></td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['chmodlvl']; ?></td>
            <td><input name="cml" type="text" size="3" value="<? echo $chaninfo['cml']; ?>"></td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['noops']; ?></td>
            <td><input name="opt1" type="radio" value="<? echo C_NOOPS; ?>" <? if ($chaninfo['options'] & C_NOOPS) echo 'checked';?>>ON
				<input name="opt1" type="radio" value="0" <? if (!($chaninfo['options'] & C_NOOPS)) echo 'checked';?>>OFF
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['strictop']; ?></td>
            <td><input name="opt2" type="radio" value="<? echo C_STRICTOP; ?>" <? if ($chaninfo['options'] & C_STRICTOP) echo 'checked';?>>ON
                <input name="opt2" type="radio" value="0" <? if (!($chaninfo['options'] & C_STRICTOP)) echo 'checked';?>>OFF 
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['locktopic']; ?></td>
            <td><input name="opt3" type="radio" value="<? echo C_LOCKTOPIC; ?>" <? if ($chaninfo['options'] & C_LOCKTOPIC) echo 'checked';?>>ON
                <input name="opt3" type="radio" value="0" <? if (!($chaninfo['options'] & C_LOCKTOPIC)) echo 'checked';?>>OFF 
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['nobans']; ?></td>
            <td><input name="opt4" type="radio" value="<? echo C_NOBANS; ?>" <? if ($chaninfo['options'] & C_NOBANS) echo 'checked';?>>ON
                <input name="opt4" type="radio" value="0" <? if (!($chaninfo['options'] & C_NOBANS)) echo 'checked';?>>OFF 
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['autoinv']; ?></td>
            <td><input name="opt5" type="radio" value="<? echo C_AUTOINVITE; ?>" <? if ($chaninfo['options'] & C_AUTOINVITE) echo 'checked';?>>ON
                <input name="opt5" type="radio" value="0" <? if (!($chaninfo['options'] & C_AUTOINVITE)) echo 'checked';?>>OFF 
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['autolim']; ?></td>
            <td><input name="opt6" type="radio" value="<? echo C_FLIMIT; ?>" <? if ($chaninfo['options'] & C_FLIMIT) echo 'checked';?>>ON
                <input name="opt6" type="radio" value="0" <? if (!($chaninfo['options'] & C_FLIMIT)) echo 'checked';?>>                
                OFF 
				inc :<input name="alinc" type="text" size="3" value="<? echo $chaninfo['liminc']; ?>">
				grace :<input name="algrace" type="text" size="3" value="<? echo $chaninfo['limgrace']; ?>">				
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['welcome']; ?></td>
            <td><input name="opt7" type="radio" value="<? echo C_SETWELCOME; ?>" <? if ($chaninfo['options'] & C_SETWELCOME) echo 'checked';?>>ON
                <input name="opt7" type="radio" value="0" <? if (!($chaninfo['options'] & C_SETWELCOME)) echo 'checked';?>>OFF 
			</td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['wecomemsg']; ?></td>
            <td><textarea name="welcome" cols="40"><? echo $chaninfo['welcome']; ?></textarea></td>
          </tr>
          <tr>
            <td align="right"><? echo $trad[$lang]['chaninfo']['options']['motd']; ?></td>
            <td><textarea name="motd" cols="40"><? echo $chaninfo['motd']; ?></textarea></td>
          </tr>
        </table>
    	<br>
		<input type="submit" name="Modifier" value="<? echo $trad[$lang]['chanopt']['modif']; ?>">
		<input type="submit" name="Reset" value="<? echo $trad[$lang]['chanopt']['reset']; ?>">
	</form>

	<form action="modifaccess.php" method="post">
		<input type="hidden" name="channel" value="<? echo $_REQUEST['channel']; ?>">
		<input type="submit" name="Modifaxx" value="<? echo $trad[$lang]['chanopt']['modifaxx']; ?>">		
	</form>	

	<?
	}
}
else
	w2c_printerror(1);

do_footer();
?>
