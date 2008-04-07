<?php

#include_once(dirname(__FILE__).'/template-lite/class.template.php');
include_once('/usr/share/php/smarty/Smarty.class.php');
#/usr/share/php/smarty/Smarty_Compiler.class.php

// Templating
#$tpl = new Template_Lite();
$tpl = new Smarty();
$tpl->compile_dir = dirname(__FILE__).'/../c_templates/';
$tpl->template_dir = dirname(__FILE__).'/../templates/';
$tpl->plugins_dir[] = dirname(__FILE__).'/smarty-plugins/';

// Globals
//$tpl->assign('root_url', '/dir.xiph.org');
$tpl->assign('root_url', '');

?>
