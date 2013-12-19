<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>


<head>
<title>WebSeen: <?numresults?> results found</title>
<?template javascript?>
</head>


<body bgcolor="white">
<?template header?>
<br>
<center>
<table border="0" width="100%" cellspacing="0">
<tr bgcolor="#3366cc" nowrap>
  <td align="left" nowrap><font face=arial,sans-serif size=-1 color=white>Database was searched for <b>&quot;<?query?>&quot;</b>.</font></td>
  <td align="right" nowrap><font face=arial,sans-serif size=-1 color=white><b><?numresults?></b> hits in <b><?numseens?></b> Datasets. Search Time: <b><?searchtime?></b> seconds.</font></td>
</tr>
</table>
<br><br>
<?ifonchan <H1><$nick$> is on IRC right now.</H1>?>
<table border="0" width="90%">
<tr><th align="left">nick</th><th align="left">last seen</th><th align="left">host</th><th align="left">log</th></tr>
<!-- Results Start -->
<?init_colorfade 0x697bc9 0xFFFFFF?>
<?results
<tr bgcolor="<$fcolor$>">
  <td><$nick$></td>
  <td><$ago$> ago (<$time %d.%m. %H:%M$>)</td>
  <td><$host$></td>
  <td><$log$></td>
  <$fade_color$>
</tr>
?>
<!-- Results End -->
</table>
<br>
<?template searchagain?>
</center>
<?template footer?></body>
</html>

