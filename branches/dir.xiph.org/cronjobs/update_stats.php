<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Base query
$query_pattern = 'SELECT COUNT(id) AS `count` FROM `mountpoint` WHERE %s;';
$where_pattern = '`media_type_id` = "%s"';

// MP3
$where = sprintf($where_pattern, CONTENT_TYPE_MP3);
$query = sprintf($query_pattern, $where);
$count = $db->singleQuery($query)->current('count');
$memcache->set(ENVIRONMENT.'_servers_'.CONTENT_TYPE_MP3, $count, false, 600); // 10 minutes

// Vorbis
$where = array();
$where = sprintf($where_pattern, CONTENT_TYPE_OGG_VORBIS);
$query = sprintf($query_pattern, $where);
$count = $db->singleQuery($query)->current('count');
$memcache->set(ENVIRONMENT.'_servers_'.CONTENT_TYPE_OGG_VORBIS, $count, false, 600); // 10 minutes

// Total
$query = sprintf($query_pattern, '1');
$count = $db->singleQuery($query)->current('count');
$memcache->set(ENVIRONMENT.'_servers_total', $count, false, 600); // 10 minutes

echo("OK.\n");

?>
