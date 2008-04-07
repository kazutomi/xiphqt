<?php

include_once(dirname(__FILE__).'/inc/prepend.php');

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Header
$tpl->display("head.tpl");

// Get the data from the Memcache server
$top = $memcache->get('prod_home_top');
$top = array_map(array('Mountpoint', 'retrieveByPk'), $top);
$tpl->assign('data', $top);
$tpl->assign('servers_total', $memcache->get('servers_total'));
$tpl->assign('servers_mp3', $memcache->get('servers_'.CONTENT_TYPE_MP3));
$tpl->assign('servers_vorbis', $memcache->get('servers_'.CONTENT_TYPE_OGG_VORBIS));
$tpl->assign('tag_cloud', $memcache->get('prod_tagcloud'));
$tpl->display('index.tpl');

// Footer
$tpl->assign('generation_time', (microtime(true) - $begin_time) * 1000);
$tpl->assign('sql_queries', isset($db) ? $db->queries : 0);
$tpl->assign('mc_gets', isset($memcache) ? $memcache->gets : 0);
$tpl->assign('mc_sets', isset($memcache) ? $memcache->sets : 0);
if (isset($memcache))
{
    $tpl->assign('mc_debug', $memcache->log);
}
$tpl->display('foot.tpl');

?>
