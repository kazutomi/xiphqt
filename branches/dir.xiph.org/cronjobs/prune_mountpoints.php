<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Old stuff that "timeouted"
try
{
	$toDelete = $db->selectQuery('SELECT `id` FROM `server` WHERE `last_touched_at` <= DATE_SUB(NOW(), INTERVAL 20 MINUTE);');
	
	while (!$toDelete->endOf())
	{
	printf("%s Processing %s... ", date(DATE_ATOM), $toDelete->current('id'));
        $server = Server::retrieveByPk($toDelete->current('id'));
        $mp_id = $server->getMountpointId();
        $mountpoint = Mountpoint::retrieveByPk($mp_id);
        $server->remove();
        if ($mountpoint instanceOf Mountpoint && !$mountpoint->hasLinkedServers())
        {
        	$mountpoint->remove();
		printf("removing mountpoint... ");
        }

	echo "ok.\n";
		
		// Next!
		$toDelete->next();
	}
}
catch (SQLNoResultException $e)
{
	// it's ok, everything's clean.
}

// Useless mountpoint
/*try
{
    $toDelete = $db->selectQuery('SELECT m.`id` AS `mountpoint_id`, s.`id` FROM `mountpoint` AS m LEFT OUTER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` HAVING s.`id` IS NULL;');
    while (!$toDelete->endOf())
    {
	    // Deletion
	    $mp_id = $toDelete->current('mountpoint_id');
	    $sql = 'DELETE FROM `mountpoint` WHERE `id` = %d;';
	    $db->noReturnQuery(sprintf($sql, $mp_id));
	    
	    // Tag cloud update
	    $sql = 'UPDATE `tag_cloud` SET `tag_usage` = `tag_usage` - 1 WHERE `tag_id` IN (SELECT `tag_id` FROM `mountpoints_tags` WHERE `mountpoint_id` = %d);';
	    $db->noReturnQuery(sprintf($sql, $mp_id));
	    
	    // Mountpoints/tags relation table update
	    $sql = 'DELETE FROM `mountpoints_tags` WHERE `mountpoint_id` = %d;';
	    $db->noReturnQuery(sprintf($sql, $mp_id));
	    
	    // Next!
	    $toDelete->next();
    }
}
catch (SQLNoResultException $e)
{
}*/

// Now prune the tags
/*$sql = 'DELETE FROM `tag` WHERE `id` IN (SELECT `tag_id` FROM `tag_cloud` WHERE `tag_usage` <= 0);';
$db->noReturnQuery($sql);
$sql = 'DELETE FROM `tag_cloud` WHERE `tag_usage` <= 0;'; // impossible since the field is UNSIGNED :(
$db->noReturnQuery($sql);
try
{
    $sql = 'SELECT mt.`mountpoint_id` FROM `mountpoints_tags` AS mt LEFT OUTER JOIN `mountpoint` AS m ON mt.`mountpoint_id` = m.`id` WHERE m.`id` IS NULL GROUP BY mt.`mountpoint_id` ORDER BY NULL;';
    $res = $db->selectQuery($sql);
    $ids = array();
    while (!$res->endOf())
    {
        $ids[] = $res->current('mountpoint_id');
        $res->next();
    }
    $sql = 'DELETE FROM `mountpoints_tags` WHERE `mountpoint_id` IN (%s);';
    $sql = sprintf($sql, implode(', ', $ids));
    $db->noReturnQuery($sql);
}
catch (SQLNoResultException $e)
{
}*/

?>
