<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// You need at least that many listeners to make it to the homepage. SQL "optimization"
// that allows to lower the number of rows to sort on. Should be set correctly, or the
// CPU will burn.
define('MIN_LISTENERS_FOR_HOMEPAGE', 10);
// How many streams will appear on the homepage
define('MAX_STREAMS_ON_HOMEPAGE', 20);

// Clusters
// ORDER BY `listeners` DESC LIMIT %d --> this was removed since it triggered temporary
// + filesort total loser combo.
/*try
{
	$query = 'SELECT CONCAT("c", c.`id`) AS `id`, "cluster" AS `type`, m.`stream_name`, m.`description`, m.`genre`, SUM(m.`listeners`) AS `listeners`, m.`url`, m.`current_song`, m.`media_type`, m.`bitrate`, m.`channels` FROM `mountpoints` AS m LEFT JOIN `clusters` AS c ON m.`cluster_id` = c.`id` WHERE `listeners` > %d AND m.`cluster_id` IS NOT NULL GROUP BY m.`cluster_id`;';
	$query = sprintf($query, MIN_LISTENERS_FOR_HOMEPAGE, MAX_STREAMS_ON_HOMEPAGE);
	$res0 = $db->selectQuery($query)->array_data;
}
catch (SQLNoResultException $e)
{
	$res0 = array();
}

// Standalone stuff
// ORDER BY `listeners` DESC LIMIT %d --> this was removed, see above (except for
// temporary, not triggered since everything exists in DB here)
try
{
	$query = 'SELECT CONCAT("s", `id`) AS `id`, "standalone" AS `type`, `stream_name`, `description`, `genre`, `listeners`, `url`, `current_song`, `media_type`, `bitrate`, `channels` FROM `mountpoints` WHERE `listeners` > %d AND `cluster_id` IS NULL;';
	$query = sprintf($query, MIN_LISTENERS_FOR_HOMEPAGE, MAX_STREAMS_ON_HOMEPAGE);
	$res1 = $db->selectQuery($query)->array_data;
}
catch (SQLNoResultException $e)
{
	$res1 = array();
}

// Merging -- *not* done in DB, see above.
function stream_compare($s0, $s1)
{
	return ($s0['listeners'] > $s1['listeners']) ? -1 : (($s0['listeners'] == $s1['listeners']) ? 0 : +1);
}
$data = array_merge($res0, $res1);
usort($data, 'stream_compare');

// Add a "rank" item
function set_rank(&$v, $k)
{
	$v['rank'] = $k + 1;
}
array_walk($data, 'set_rank');*/

try
{
//    $query = 'SELECT * FROM `mountpoint` ORDER BY `listeners` DESC LIMIT %d;';
    $query = 'SELECT `id` FROM `mountpoint` ORDER BY RAND() LIMIT %d;';
    $query = sprintf($query, MAX_STREAMS_ON_HOMEPAGE);
    $data = $db->selectQuery($query);
	$res = array();
	while (!$data->endOf())
	{
		$res[] = intval($data->current('id'));
		$data->next();
	}
	$data = $res;
}
catch (SQLNoResultException $e)
{
    $data = array();
}

// array_slice($data, 0, MAX_STREAMS_ON_HOMEPAGE)
$memcache->set('prod_home_top', $data, false, 120) or die("Unable to save data on the Memcache server.\n");
echo("OK.\n");

?>
