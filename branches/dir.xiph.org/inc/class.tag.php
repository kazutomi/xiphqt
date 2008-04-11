<?php

class Tag
{
    protected static $mountpoints_tags_cache_expiration = 900; // 15 min
    
    /**
     * Adds many tags to a mountpoint.
     * 
     * @param Mountpoint $mountpoint
     * @param array $tags
     * @return boolean
     */
    public static function massTagMountpoint(Mountpoint $mountpoint, $tags)
    {
        // MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Add the tags
		$query = 'INSERT IGNORE INTO `tag` (`tag_name`) VALUES %s;';
		$tags_clause = array();
		foreach ($tags as $t)
		{
			$tags_clause[] = sprintf('("%s")', mysql_real_escape_string(strtolower($t)));
		}
		$tags_clause = implode(', ', $tags_clause);
		$query = sprintf($query, $tags_clause);
//		fwrite($fp, "Adding tags: ".$query."\n");
		$db->noReturnQuery($query);
		
		// Get back the tags IDs
		$query = 'SELECT `id` FROM `tag` WHERE `tag_name` IN (%s);';
		$tags_clause = array();
		foreach ($tags as $t)
		{
			$tags_clause[] = sprintf('"%s"', mysql_real_escape_string(strtolower($t)));
		}
		$tags_clause = implode(', ', $tags_clause);
		$query = sprintf($query, $tags_clause);
//		fwrite($fp, "Fetching back tags: ".$query."\n");
		$tags = $db->selectQuery($query);
		
		// Link the mountpoint to the tags
		$query = 'INSERT IGNORE INTO `mountpoints_tags` (`mountpoint_id`, `tag_id`) VALUES %s;';
		$tags_clause = array();
		while (!$tags->endOf())
		{
			$tags_clause[] = sprintf('(%d, %d)', $mountpoint->getId(), $tags->current('id'));
			$tags->next();
		}
		$tags->rewind();
		$tags_clause = implode(', ', $tags_clause);
		$query = sprintf($query, $tags_clause);
//		fwrite($fp, "Linking tags: ".$query."\n");
		$db->noReturnQuery($query);
		
		// Increment the tag cloud values
		$query = 'INSERT INTO `tag_cloud` (`tag_id`, `tag_usage`) VALUES %s ON DUPLICATE KEY UPDATE `tag_usage` = `tag_usage` + 1;';
		$tags_clause = array();
		while (!$tags->endOf())
		{
			$tags_clause[] = sprintf('(%d, 1)', $tags->current('id'));
			$tags->next();
		}
		$tags_clause = implode(', ', $tags_clause);
		$query = sprintf($query, $tags_clause);
//		fwrite($fp, "Incrementing tags: ".$query."\n");
		$db->noReturnQuery($query);
    }
    
    /**
     * Returns all of the tags linked to a particular mountpoint.
     * 
     * @param int $mountpoint_id
     * @return array
     */
    public static function getMountpointTags($mountpoint_id)
    {
        // Memcache Connection
		$mc = DirXiphOrgMCC::getInstance();
		
		if (!defined('ENVIRONMENT'))
        {
            throw new EnvironmentUndefinedException();
        }
        
        $key = sprintf("%s_mountpointtags_%d", ENVIRONMENT, $mountpoint_id);
        
        $tags = $mc->get($key);
        if ($tags !== false)
        {
            return $tags;
        }
        else
        {
            // MySQL Connection
		    $db = DirXiphOrgDBC::getInstance();
		    
		    $sql = "SELECT mt.`tag_id`, t.`tag_name` FROM `mountpoints_tags` AS mt INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` WHERE mt.`mountpoint_id` = %d;";
		    $sql = sprintf($sql, intval($mountpoint_id));
		    try
		    {
    		    $data = $db->selectQuery($sql);
    		    
    		    $res = array();
    		    while (!$data->endOf())
    		    {
    		        $res[$data->current('tag_id')] = $data->current('tag_name');
    		        $data->next();
    		    }
    		    
    		    $mc->set($key, $res, false, self::$mountpoints_tags_cache_expiration);
    		    return $res;
		    }
		    catch (SQLNoResultException $e)
		    {
		        return array();
	        }
		}
    }
    
    /**
     * Delete the mountpoint-to-tags associations for a given mountpoint (plus
     * decrement the tag cloud).
     */
    public static function deleteMountpointTags($mountpoint_id)
    {
        // MySQL Connection
	    $db = DirXiphOrgDBC::getInstance();
	    
	    $sql = 'UPDATE `tag_cloud` SET `tag_usage` = `tag_usage` - 1 WHERE `tag_id` IN (SELECT `tag_id` FROM `mountpoints_tags` WHERE `mountpoint_id` = %d);';
	    $sql = sprintf($sql, intval($mountpoint_id));
	    $res0 = $db->noReturnQuery($sql);
	    
	    $sql = "DELETE FROM `mountpoints_tags` WHERE `mountpoint_id` = %d;";
	    $sql = sprintf($sql, intval($mountpoint_id));
	    $res1 = $db->noReturnQuery($sql);
	    
	    return $res0 && $res1;
    }
}

?>
