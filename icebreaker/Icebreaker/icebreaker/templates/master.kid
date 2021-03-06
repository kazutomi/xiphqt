<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<?python import sitetemplate ?>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#" py:extends="sitetemplate">

<head py:match="item.tag=='{http://www.w3.org/1999/xhtml}head'" py:attrs="item.items()">
    <meta content="text/html; charset=UTF-8" http-equiv="content-type" py:replace="''"/>
    <title py:replace="''">Your title goes here</title>
    <meta py:replace="item[:]"/>
    <style type="text/css">
        #pageLogin
        {
            font-size: 10px;
            font-family: verdana;
            text-align: right;
        }
    </style>
    <style type="text/css" media="screen">
@import "${tg.url('/static/css/screen.css')}";
@import "${tg.url('/static/css/xiphbar.css')}";
</style>
</head>

<body py:match="item.tag=='{http://www.w3.org/1999/xhtml}body'" py:attrs="item.items()">
<div id="xiphbar_outer">
    <div py:if="tg.config('identity.on') and not defined('logging_in')" id="pageLogin"  border="0" cellpadding="0" cellspacing="0">
        <table id="xiphbar" border="0" cellpadding="0" cellspacing="0">
        <tr>
            <td id="xiphlinks" align="right">
               <a href="http://www.xiph.org/">Xiph.org</a>
               <a href="http://www.vorbis.com/">Vorbis</a>
               <a href="http://www.theora.org/">Theora</a>
               <a href="http://www.icecast.org/">Icecast</a>
               <a href="http://www.speex.org/">Speex</a>
               <a href="http://flac.sourceforge.net/">FLAC</a>
               <a href="http://www.xspf.org/">XSPF</a>
               <span py:if="tg.identity.anonymous">
                   <a href="${tg.url('/login')}">Login</a>
               </span>
               <span py:if="not tg.identity.anonymous">
               Welcome ${tg.identity.user.display_name}.
                  <a href="${tg.url('/logout')}">Logout</a>
               </span>
            </td>
        </tr>
        </table>
    </div>
</div>
    <div id="header">&nbsp;</div>
    <div id="main_content">
    <div id="status_block" class="flash" py:if="value_of('tg_flash', None)" py:content="tg_flash"></div>

    <div py:replace="[item.text]+item[:]"/>

    <!-- End of main_content -->
    </div>

</body>

</html>
