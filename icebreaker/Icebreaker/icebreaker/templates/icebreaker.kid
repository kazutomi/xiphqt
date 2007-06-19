<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#">
<head>
  <meta content="text/html; charset=utf-8" http-equiv="Content-Type" py:replace="''" />
  <title>Welcome to TurboGears</title>
</head>
<body>
<h1>Now playing</h1>
<span py:replace="greeting">whatever song is currently playing</span>
<span py:replace="greeting">whatever playlist(or shuffle)</span>
<h1>Next up</h1>
<span py:replace="greeting">Next song in queue</span>
<span py:replace="greeting">What playlist/shuffle</span>

<p>some stuff about making lists, etc.</p>

</body>
</html>
