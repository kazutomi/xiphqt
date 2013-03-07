<?php

include_once(dirname(__FILE__).'/inc/prepend.php');

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Header
$tpl->display("head.tpl");

// Get the data from the Memcache server
$top = genfile::get(genfile::makeGenfileName('home_top'));
$top = array_map(array('Mountpoint', 'retrieveByPk'), $top);
$tpl->assign_by_ref('data', $top);
$tpl->assign('tag_cloud', genfile::get(genfile::makeGenfileName('tagcloud')));

// Stats
$tpl->assign('servers_total', $memcache->get(ENVIRONMENT.'_servers_total'));
$tpl->assign('servers_mp3', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_MP3));
$tpl->assign('servers_vorbis', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_OGG_VORBIS));
$tpl->assign('servers_opus', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_OPUS));
$tpl->assign('servers_webm', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_WEBM));
$tpl->display('index.tpl');

// Footer
if (ENVIRONMENT != 'prod')
{
	$tpl->assign('generation_time', (microtime(true) - $begin_time) * 1000);
	$tpl->assign('sql_queries', isset($db) ? $db->queries : 0);
	$tpl->assign('mc_gets', isset($memcache) ? $memcache->gets : 0);
	$tpl->assign('mc_sets', isset($memcache) ? $memcache->sets : 0);
	if (isset($memcache))
	{
		$tpl->assign('mc_debug', $memcache->log);
	}
}
$tpl->assign('google_tag', 'home');
$tpl->display('foot.tpl');

?>
