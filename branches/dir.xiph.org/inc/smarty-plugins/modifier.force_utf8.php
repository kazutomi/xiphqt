<?php

include_once(dirname(__FILE__).'/../lib.utils.php');

function smarty_modifier_force_utf8($str)
{
	$str = mb_ereg_replace('[[:cntrl:]]', '', $str);
	return utils::is_utf8($str) ? $str : utf8_encode($str);
}
?>
