<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>


<head>
<title>WebSeen: <?numresults?> Ergebnisse gefunden</title>
<?template javascript?>
</head>


<body bgcolor="white">
<?template header?>
<br>
<center>
<table border="0" width="100%" cellspacing="0">
<tr bgcolor="#3366cc" nowrap>
  <td align="left" nowrap><font face=arial,sans-serif size=-1 color=white>Die Datenbank wurde nach <b>&quot;<?query?>&quot;</b> durchsucht.</font></td>
  <td align="right" nowrap><font face=arial,sans-serif size=-1 color=white><b><?numresults?></b> Treffer in <b><?numseens?></b> Datensätzen. Suchzeit: <b><?searchtime?></b> Sekunden.</font></td>
</tr>
</table>
<br><br>
<?ifonchan <H1><$nick$> ist jetzt grade im IRC.</H1>?>
<table border="0" width="90%">
<tr><th align="left">Nick</th><th align="left">zuletzt beobachtet</th><th align="left">Host</th><th align="left">Log</th></tr>
<!-- Results Start -->
<?init_colorfade 0x697bc9 0xFFFFFF?>
<?results
<tr bgcolor="<$fcolor$>">
  <td><$nick$></td>
  <td>vor <$ago$> (<$time %d.%m. %H:%M$>)</td>
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