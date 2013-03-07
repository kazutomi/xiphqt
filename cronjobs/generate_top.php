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
//define('MIN_LISTENERS_FOR_HOMEPAGE', 10);
// How many streams will appear on the homepage
//define('MAX_STREAMS_ON_HOMEPAGE', 20);
// How many Opus streams appear
define('ID_OPUS', '107' );
define('MAX_STREAMS_OPUS', 3 );
// How many good codec streams appear
define('ID_GOOD', '101, 102, 108');
define('MAX_STREAMS_GOOD', 15 );
// How many bad codec streams appear
define('ID_BAD', '103, 104, 105, 106, 100'); // 100 is 'unknown' and likely ogg... need to check
define('MAX_STREAMS_BAD', 5 );

try
{
//    $query = 'SELECT `id` FROM `mountpoint` ORDER BY `listeners` DESC LIMIT %d;';
    $query = 'SELECT m.`id`, SUM(s.`checked`) / COUNT(*) AS `checked`'
            .'FROM `mountpoint` AS m INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` '
            .'WHERE m.`media_type_id` IN (%s) AND s.`listeners` > 0 '
            .'GROUP BY m.`id` '
            .'HAVING `checked` = 1 '
            .'ORDER BY RAND() LIMIT %d;';
//    $query = 'SELECT `id` FROM `mountpoint` ORDER BY RAND() LIMIT %d;';
    $query_opus = sprintf($query, ID_OPUS ,MAX_STREAMS_OPUS);
    $data_opus = $db->selectQuery($query_opus);
    $query_good = sprintf($query, ID_GOOD, MAX_STREAMS_GOOD);
    $data_good = $db->selectQuery($query_good);
    $query_bad = sprintf($query, ID_BAD, MAX_STREAMS_BAD);
    $data_bad = $db->selectQuery($query_bad);
	$res = array();
	while (!$data_opus->endOf())
	{
		$res[] = intval($data_opus->current('id'));
		$data_opus->next();
	}
	while (!$data_good->endOf())
	{
		$res[] = intval($data_good->current('id'));
		$data_good->next();
	}
	while (!$data_bad->endOf())
	{
		$res[] = intval($data_bad->current('id'));
		$data_bad->next();
	}
	$data = $res;
}
catch (SQLNoResultException $e)
{
    $data = array();
}

// array_slice($data, 0, MAX_STREAMS_ON_HOMEPAGE)
genfile::write(genfile::makeGenfileName('home_top'), $data) or die("Unable to save data in a genfile.\n");
echo("OK.\n");

?>
