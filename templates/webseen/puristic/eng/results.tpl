<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>


<head>
<title>WebSeen: <?numresults?> results found</title>
<?template javascript?>
</head>


<body bgcolor="white">
<b><?numresults?></b> hits in <b><?numseens?></b> Datasets. Search Time: <b><?searchtime?></b> seconds.
<br><br>
<?ifonchan <H1><$nick$> is on IRC right now.</H1>?>
<table border="0" width="90%">
<tr><th align="left">nick</th><th align="left">last seen</th><th align="left">host</th><th align="left">log</th></tr>
<!-- Results Start -->
<?results
<tr>
  <td><$nick$></td>
  <td><$ago$> ago (<$time %d.%m. %H:%M$>)</td>
  <td><$host$></td>
  <td><$log$></td>
</tr>
?>
<!-- Results End -->
</table>
<br>
<?template searchagain?>
</body>


</html>

