<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>


<head>
<title>WebSeen: <?numresults?> Ergebnisse</title>
<?template javascript?>
</head>


<body bgcolor="white">
<b><?numresults?></b> Treffer in <b><?numseens?></b> Datensätzen. Suchdauer: <b><?searchtime?></b> Sekunden.
<br><br>
<?ifonchan <H1><$nick$> ist jetzt gerade im IRC.</H1>?>
<table border="0" width="90%">
<tr><th align="left">Nick</th><th align="left">zuletzt gesehen</th><th align="left">Host</th><th align="left">Log</th></tr>
<!-- Results Start -->
<?results
<tr>
  <td><$nick$></td>
  <td>vor <$ago$> (<$time %d.%m. %H:%M$>)</td>
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

