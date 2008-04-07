<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

class ToDeleteException extends Exception { }

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Old stuff that "timeouted"
$res = $db->selectQuery('SELECT * FROM `server` WHERE `checked` = 0;');
while (!$res->endOf())
{
    try
    {
        // Get the URL
        $url = @parse_url($res->current('listen_url'));
        if (!$url)
        {
            throw new ToDeleteException();
        }
        
        // Now, verify!
        if (empty($url['scheme']) || $url['scheme'] != 'http'
            || !array_key_exists('host', $url))
        {
            throw new ToDeleteException();
        }
        
        // Try to open a connection to the server
        $ok = false;
        $count = 0;
        while ($count < 3 && !$ok)
        {
            $fp = fsockopen($url['host'],
                            array_key_exists('port', $url) ? $url['port'] : 80, // as per HTTP RFC
                            $errno, $errstr, 5);
            if (!$fp)
            {
                $count++;
                continue;
            }
            
            // Now send a request
            $req = sprintf("GET %s HTTP/1.0\r\n\r\n", $url['path']);
            $r = fwrite($fp, $req);
            if (!$r || $r != strlen($req))
            {
                $count++;
                continue;
            }
            $r = 0;
            $headers = array();
            do
            {
                $data = fgets($fp);
                if (trim($data) != '')
                {
                    list($header, $value) = explode(':', $data);
                    $headers[strtolower(trim($header))] = trim($value);
                }
                $r++;
            }
            while (trim($data) != '' && $r < 10);
            
            // Extremely dangerous, desactivated.
/*            if (!array_key_exists('server', $headers)
                || !stristr($headers['server'], 'icecast'))
            {
                throw new ToDeleteException();
            }*/
            fclose($fp);
            
            $count++;
            $ok = true;
        }
        if (!$ok)
        {
            throw new ToDeleteException();
        }
    }
    catch (ToDeleteException $e)
    {
        // TODO: remove the stream
        echo("Delete it! ".$res->current('listen_url')."\n");
    }
    
    $res->next();
}

exit();
// Useless mountpoint
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
	
	// Next!
	$toDelete->next();
}

// Now prune the tags
$sql = 'DELETE FROM `tag` WHERE `id` IN (SELECT `tag_id` FROM `tag_cloud` WHERE `tag_usage` <= 0);';
$db->noReturnQuery($sql);
$sql = 'DELETE FROM `tag_cloud` WHERE `tag_usage` <= 0;';
$db->noReturnQuery($sql);

?>
