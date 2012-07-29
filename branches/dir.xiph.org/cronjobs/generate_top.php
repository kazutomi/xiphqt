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

try
{
//    $query = 'SELECT `id` FROM `mountpoint` ORDER BY `listeners` DESC LIMIT %d;';
    $query = 'SELECT m.`id`, SUM(s.`checked`) / COUNT(*) AS `checked` '
            .'FROM `mountpoint` AS m INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` '
            .'GROUP BY m.`id` '
            .'HAVING `checked` = 1 ORDER BY m.`listeners` DESC LIMIT %d;';
//    $query = 'SELECT `id` FROM `mountpoint` ORDER BY RAND() LIMIT %d;';
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
genfile::write(genfile::makeGenfileName('home_top'), $data) or die("Unable to save data in a genfile.\n");
echo("OK.\n");

?>
