<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#" py:extends="'master.kid'">
<head>
  <meta content="text/html; charset=utf-8" http-equiv="Content-Type" py:replace="''" />
  <title>List</title>
</head>
<body>
<table border="1">
    <tr>
      <td>Artist</td>
      <td>Album</td>
      <td>Title</td>
      <td>Genre</td>
      <td>Path</td>
    </tr>
    <tr py:for="list in data">
      <td py:content="list.artist1">artist</td>
      <td py:content="list.album1">album</td>
      <td py:content="list.title1">title</td>
      <td py:content="list.genre1">genre</td>
      <td py:content="list.path1">path</td>
    </tr>
</table>
</body>
</html>
