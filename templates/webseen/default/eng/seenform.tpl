<FORM action="seen" method=get name="seenform">
<table border="0">
<tr>
  <td colspan="3">
    <INPUT type=text value="<?query?>" framewidth=4 name=query size=50>
  </td>
  <td>
    <INPUT type=submit value="seen">
  </td>
</tr>
<tr>
  <td align="left">
    <input name=fuzzy type=checkbox value="0">exact search
  </td>
  <td align="right">
    Language:
    <select name="lang" style="height=19;font=10;" onChange="SetWebseenCookie('lang', document.seenform.lang.value)">
      <?langlist 
      <option value="<$name$>" <$ifactivelang selected$>><$desc$>
      ?>
    </select>
  </td>
  <td align="right">
    Skin:
    <select name="skin" style="height=19;font=10;" onChange="SetWebseenCookie('skin', document.seenform.skin.value)">
      <?skinlist
      <option value="<$name$>" <$ifactiveskin selected$>><$desc$>
      ?>
    </select>
  </td>
  <td>
  </td>
</tr>
</table>
</FORM>