<?php
include("libw2c.inc.php");
include("config.inc.php");
?>
<?php do_head(); ?>


<?
$fp = fopen("motd.txt","r"); 
while (!feof($fp)) 
{ 
echo  fgets ($fp,4096); 

} 


?>

<?php do_footer(); ?>

