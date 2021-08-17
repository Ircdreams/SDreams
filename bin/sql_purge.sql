DELETE FROM chancmdslog WHERE TS<=(UNIX_TIMESTAMP() - 2592000) 
AND (cmd='kick' OR cmd='deop' OR cmd='deopall'  or cmd='info'
OR cmd='opall' OR cmd='op' OR cmd='voice' OR cmd='devoice' OR cmd='devoiceall'
OR cmd='voiceall' OR cmd='invite' OR cmd='mode' OR cmd='topic' OR cmd='rdefmodes'
OR cmd='rdeftopic' OR cmd='locktopic' or cmd='alist' or cmd='kickban'
OR cmd='ban' or cmd='unban' OR cmd='noops' or cmd='nobans'
OR cmd='strictop' OR cmd='clearbans' or cmd='unbanme');

delete from usercmdslog where cmd='memos' or cmd='myaccess'
or cmd='vote' or cmd='inviteme' or cmd='listbadnicks' or
cmd='listbadchans' or cmd='uptime' or (
(TS <= (UNIX_TIMESTAMP() - 2592000)) and (
cmd='user' or cmd='chan' or cmd='login' or cmd='recover' or cmd='set' or cmd='whois'));

select cmd, count(*) as Uses from usercmdslog group by cmd order by Uses desc;

select cmd, count(*) as Uses from chancmdslog group by cmd order by Uses desc;