<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
//$memcache = DirXiphOrgMCC::getInstance();


// Base query
$query_pattern = 'SELECT COUNT(*) AS `count` FROM `server` AS s INNER JOIN `mountpoint` AS m ON m.`id` = s.`mountpoint_id` WHERE %s;';
$where_pattern = 'm.`media_type_id` = "%s"';

// Total
$query = sprintf($query_pattern, '1');
$total = $db->singleQuery($query)->current('count');
printf("total.value %s\n", $total);

// Vorbis
$where = array();
$where = sprintf($where_pattern, CONTENT_TYPE_OGG_VORBIS);
$query = sprintf($query_pattern, $where);
$vorbis = $db->singleQuery($query)->current('count');
printf("vorbis.value %s\n", $vorbis);

// Opus
$where = array();
$where = sprintf($where_pattern, CONTENT_TYPE_OPUS);
$query = sprintf($query_pattern, $where);
$opus = $db->singleQuery($query)->current('count');
printf("opus.value %s\n", $opus);

// Theora
$where = array();
$where = sprintf($where_pattern, CONTENT_TYPE_OGG_THEORA);
$query = sprintf($query_pattern, $where);
$theora = $db->singleQuery($query)->current('count');
printf("theora.value %s\n", $theora);

// WebM
$where = array();
$where = sprintf($where_pattern, CONTENT_TYPE_WEBM);
$query = sprintf($query_pattern, $where);
$vorbis = $db->singleQuery($query)->current('count');
printf("webm.value %s\n", $vorbis);

// MP3
$where = sprintf($where_pattern, CONTENT_TYPE_MP3);
$query = sprintf($query_pattern, $where);
$mp3 = $db->singleQuery($query)->current('count');
printf("mp3.value %s\n", $mp3);

// AAC
$where = sprintf($where_pattern, CONTENT_TYPE_AAC);
$query = sprintf($query_pattern, $where);
$aac = $db->singleQuery($query)->current('count');
printf("aac.value %s\n", $aac);

// AAC+
$where = sprintf($where_pattern, CONTENT_TYPE_AACPLUS);
$query = sprintf($query_pattern, $where);
$aacp = $db->singleQuery($query)->current('count');
printf("aacp.value %s\n", $aacp);

// NSV
$where = sprintf($where_pattern, CONTENT_TYPE_NSV);
$query = sprintf($query_pattern, $where);
$nsv = $db->singleQuery($query)->current('count');
printf("nsv.value %s\n", $nsv);

// OTHER
$where = sprintf($where_pattern, CONTENT_TYPE_OTHER);
$query = sprintf($query_pattern, $where);
$other = $db->singleQuery($query)->current('count');
printf("other.value %s\n", $other);

// radionomy
$query = "SELECT COUNT(*) AS count FROM `server` WHERE `last_touched_from` LIKE '52089%'";
$radionomy = $db->singleQuery($query)->current('count');
printf("radionomy.value %s\n", $radionomy);

//echo("OK.\n");

?>
