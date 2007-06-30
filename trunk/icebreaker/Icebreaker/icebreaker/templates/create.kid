<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#" py:extends="'master.kid'">
<head>
  <meta content="text/html; charset=utf-8" http-equiv="Content-Type" py:replace="''" />
  <title>Welcome to TurboGears</title>
</head>
<body>
<?python
from selectshuttle.widgets import SelectShuttle
form_name="foo"
create = SelectShuttle(
name="select_shuttle_demo",
label = "The shuttle",
title_available = "Available options",
title_selected = "Selected options",
form_reference = "document.forms['%s']" % form_name,
# All data should be provided as a list of tuples, in the form of
# ("id", "value"). ATM, id should be an int
available_options = [(i, "Option %d"%i) for i in xrange(5)],
default = dict(selected=[(i, "Option %d"%i) for i in xrange(3)])
)
?>
<div>
    <form action="%s/post_data" name="%s" method="POST">
        ${create.display()}
        <br />
        <input type="submit" value="Submit" />
    </form>
</div>
</body>
</html>
