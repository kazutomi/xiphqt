<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Maximum number of streams in the tagcloud
define('MAX_TAGS_IN_CLOUD', 15);
// Maximum popularity rank defined in CSS
define('POPULARITY_RANKS', 5);

// Get data
$query = 'SELECT t.`tag_name`, tc.`tag_usage` FROM `tag_cloud` AS tc INNER JOIN `tag` AS t ON tc.`tag_id` = t.`id` ORDER BY tc.`tag_usage` DESC LIMIT %d;';
$query = sprintf($query, MAX_TAGS_IN_CLOUD);
$res = $db->selectQuery($query)->array_data;

// Sort alphabetically, and compute min and max
$min = 1000000;
$max = -1;
function sort_by_name($s0, $s1)
{
	global $min, $max;
	$min = min($min, $s0['tag_usage']);
	$max = max($max, $s0['tag_usage']);
	
	// The tags can't be equal. For they are tags.
	return ($s0['tag_name'] > $s1['tag_name']) ? +1 : -1;
}
usort($res, 'sort_by_name');

// Add popularity info
$delta = $max - $min;
$delta = $delta > 0 ? $delta : 1;
function add_popularity($s)
{
	global $min, $delta;
	
	$s['popularity'] = round(($s['tag_usage'] - $min) / ($delta / POPULARITY_RANKS));
	
	return $s;
}
$res = array_map('add_popularity', $res);
var_dump($res);
// Save into memcache
$memcache->set('prod_tagcloud', $res, 0, 600); // 10 mins
echo "OK.\n";

?>
