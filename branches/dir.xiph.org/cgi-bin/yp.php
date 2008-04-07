<?php

include_once(dirname(__FILE__).'/../inc/prepend.php');

// Do we have enough data?
if (!array_key_exists('action', $_REQUEST))
{
	echo("No action key provided.");
	exit();
}

// Log the request
if (defined('DEBUG'))
{
	$fp = fopen('/tmp/dxo_'.md5(microtime()).'.test', 'w');
	ob_start();
	printf("======================================\n%s\n",
		   date('Y-m-d H:i:s'));
	print_r($_REQUEST);
	$data = ob_get_contents();
	ob_end_clean();
	fwrite($fp, $data);
}

try
{
// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Then process it
switch ($_REQUEST['action'])
{
	case 'add':
		// TODO: handle case where the index isn't defined.
		// TODO: check that the data we get is consistent with the previous cluster data.
		
		// Check the args are here
		$mandatory_args = array('sn', 'type', 'genre', 'b', 'listenurl');
		foreach ($mandatory_args as $a)
		{
		    if (!array_key_exists($a, $_REQUEST))
		    {
			    // Return failure
			    header("YPResponse: 0");
			    header("YPMessage: Not enough arguments.");
			    header("SID: -1");
				if (defined('DEBUG'))
			    	fwrite($fp, "\YPResponse: 0\nYPMessage: Not enough arguments.\nSID: -1");
		    }
		}
		// Remote IP
		$ip = $_SERVER['REMOTE_ADDR'];
		// Stream name
		$stream_name = $_REQUEST['sn'];
		// Media type
		$media_type = $_REQUEST['type'];
		if (array_key_exists('stype', $_REQUEST))
		{
			if (preg_match('/vorbis/i', $_REQUEST['stype']))
			{
				$media_type .= '+vorbis';
			}
			if (preg_match('/theora/i', $_REQUEST['stype']))
			{
				$media_type .= '+theora';
			}
		}
		// Genre, space-normalized
		$genre = $_REQUEST['genre'];
		$genre = str_replace(array('+', '-', '*', '<', '>', '~', '"', '(', ')', '|', '!', '?'),
							 array('', '', '', '', '', '', '', '', '', '', '', '', ''),
							 $genre);
		$genre = preg_replace('/\s+/', ' ', $genre);
		$genre_list = array_slice(explode(' ', $genre), 0, 10);
		// Bitrate
		$bitrate = $_REQUEST['b'];
		// Listen URL
		$listen_url = $_REQUEST['listenurl'];
		
		// Cluster password
		$cluster_password = array_key_exists('cpswd', $_REQUEST) ? $_REQUEST['cpswd'] : null;
		// Description
		$description = array_key_exists('desc', $_REQUEST) ? $_REQUEST['desc'] : null;
		// URL
		$url = array_key_exists('url', $_REQUEST) ? $_REQUEST['url'] : null;
		
		// MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Look for the mountpoint (same listen URL)
		$server_id = $sid = null;
		$query = 'SELECT `id`, `mountpoint_id` FROM `server` WHERE `listen_url` = "%s";';
		$query = sprintf($query, mysql_real_escape_string($listen_url));
		$mp_id = $server_id = false; // MySQL auto-increment keys can't be 0
		$mp = $server = null;
		try
		{
			// The mountpoint exists yet. Either an error from the Icecast server, or
			// we didn't wipe out old stuff fast enough.
			$res = $db->singleQuery($query);
			$mp_id = $res->current('mountpoint_id');
			$mp = Mountpoint::retrieveByPk($mp_id);
			$server_id = $res->current('id');
			$server = Server::retrieveByPk($server_id);
		}
		catch (SQLNoResultException $e)
		{
			// The mountpoint doesn't exist yet in our database (it's OK)
		}
		
		// Look for the mountpoint, bis (different listen URL, same stream name)
		$query = 'SELECT `id` FROM `mountpoint` WHERE `stream_name` = "%s";';
		$query = sprintf($query, mysql_real_escape_string($stream_name));
		try
		{
			// The mountpoint exists, only a different server.
			$res = $db->singleQuery($query);
			$mp_id = $res->current('id');
			$mp = Mountpoint::retrieveByPk($mp_id);
		}
		catch (SQLNoResultException $e)
		{
			// The mountpoint doesn't exist yet in our database (it's OK)
		}
		
		// SID
		$sid = UUIDGen::generate();
		
		// Mountpoint
		if (!($mp instanceOf Mountpoint))
		{
		    $mp = new Mountpoint(0, false, true);
		    $mp->setStreamName($stream_name);
		    $mp->setDescription($description);
		    $mp->setUrl($url);
		    $mp->setMediaTypeId(content_type_lookup($media_type));
		    $mp->setBitrate($bitrate);
		    $mp->setClusterPassword($cluster_password);
		    
		    $mp_id = $mp->save();
			
			if ($mp instanceOf Mountpoint)
			{
				// Add the tags
				Tag::massTagMountpoint($mp, $genre_list);
			}
		}
		if (defined('DEBUG'))
			fwrite($fp, "Affected mountpoint ID: ".$mp_id."\n");
		
		// Server
		if ($mp instanceOf Mountpoint && !$server_id)
		{
		    $server = new Server(0, false, true);
		    $server->setMountpointId($mp_id);
		    $server->setSid($sid);
		    $server->setListenUrl($listen_url);
		    $server->setLastTouchedFrom($ip);
		    
		    $server_id = $server->save();
		}
		if (defined('DEBUG'))
			fwrite($fp, "Affected server ID: ".$server_id."\n");
		
		// Tags and stuff
		if ($mp instanceOf Mountpoint && $server instanceOf Server)
		{
			// Increment the "total servers" key in memcache
			if (!$memcache->increment('servers_total'))
 			{
				$memcache->set(ENVIRONMENT.'_servers_total', 1);
			}
			$ct_id = content_type_lookup($media_type);
			$ct_key = ENVIRONMENT.'_servers_'.intval($ct_id);
			if (!$memcache->increment($ct_key))
			{
				$memcache->set($ct_key, 1);
			}
			
			// Return success
			header("YPResponse: 1");
			header("YPMessage: Successfully added.");
			header("SID: ".$sid);
			header("TouchFreq: 250");
			if (defined('DEBUG'))
				fwrite($fp, "YPResponse: 1\nYPMessage: Successfully added.\nSID: ".$sid."\nTouchFreq: 60");
		}
		else
		{
			// Return failure
			header("YPResponse: 0");
			header("YPMessage: Error occured while processing your request.");
			header("SID: -1");
			if (defined('DEBUG'))
				fwrite($fp, "\YPResponse: 0\nYPMessage: Error occured while processing your request.\nSID: -1");
		}
		
		break;
	case 'touch':
		// TODO: handle the case where the index isn't defined.
		// SID
		$sid = $_REQUEST['sid'];
		// Remote IP
		$ip = $_SERVER['REMOTE_ADDR'];
		// Song title
		$current_song = mb_convert_encoding($_REQUEST['st'], 'UTF-8', 'UTF-8,ISO-8859-1,auto');
		// Listeners
		$listeners = $_REQUEST['listeners'];
		// Max listeners
		$max_listeners = $_REQUEST['max_listeners'];
		
		// MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Update the data
		$query = 'UPDATE `server` SET `current_song` = "%s", `listeners` = %d, `last_touched_from` = INET_ATON("%s"), `last_touched_at` = NOW() WHERE `sid` = "%s";';
		$query = sprintf($query, mysql_real_escape_string($current_song), $listeners, mysql_real_escape_string($ip), mysql_real_escape_string($sid));
		if (defined('DEBUG'))
		{
		    fwrite($fp, $query."\n");
		}
		$res = $db->noReturnQuery($query);
		if ($res && $db->affected_rows > 0)
		{
			// Return success
			header("YPResponse: 1");
			header("YPMessage: Updated server info.");
			if (defined('DEBUG'))
				fwrite($fp, "\nYPResponse: 1\nYPMessage: Updated server info.");
		}
		else
		{
			// Return failure
			header("YPResponse: 0");
			header("YPMessage: SID does not exist.");
			if (defined('DEBUG'))
				fwrite($fp, "\nYPResponse: 0\nYPMessage: SID does not exist.\n");
		}
		
		break;
	case 'remove':
		// SID
		$sid = $_REQUEST['sid'];
		
		// MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Remove the data
		$query = 'SELECT s.`id`, s.`mountpoint_id`, m.`media_type_id` FROM `server` AS s INNER JOIN `mountpoint` AS m ON s.`mountpoint_id` = m.`id` WHERE s.`sid` = "%s";';
		$query = sprintf($query, mysql_real_escape_string($sid));
		try
		{
			$res = $db->selectQuery($query);
			$server_id = $res->current('id');
			$mp_id = $res->current('mountpoint_id');
			$media_type = $res->current('media_type_id');
			
			// Remove server
			$query = 'DELETE FROM `server` WHERE `id` = %d;';
			$query = sprintf($query, $server_id);
			$res = $db->singleQuery($query);
			
			if ($res)
			{
				// Decrement the servers keys in memcache
				$memcache->decrement('servers_total');
				$memcache->decrement('servers_'.intval($media_type));
				
				// Decrement the tag cloud values
/*				$query = 'UPDATE `tag_cloud` SET `tag_usage` = `tag_usage` - 1 WHERE `tag_name` IN (SELECT `tag_id` FROM `mountpoints_tags` WHERE `mountpoint_id` = %d);';
				$query = sprintf($query, $mp_id);
				$db->noReturnQuery($query);*/
				
				// Return success
				header("YPResponse: 1");
				header("YPMessage: Deleted server info.");
				if (defined('DEBUG'))
					fwrite($fp, "\nYPResponse: 1\nYPMessage: Deleted server info.");
			}
			else
			{
				// Return failure
				header("YPResponse: 0");
				header("YPMessage: Error occured while processing your request.");
				if (defined('DEBUG'))
					fwrite($fp, "\YPResponse: 0\nYPMessage: Error occured while processing your request.");
			}
			
/*			if ($cluster_id != null)
			{
				// See if cluster is empty
				$query = 'SELECT 1 FROM `mountpoints` WHERE `cluster_id` = %d LIMIT 1;';
				$query = sprintf($query, $cluster_id);
				try
				{
					$res = $db->singleQuery($query);
					
					// OK, we can leave the cluster
				}
				catch (SQLNoResultException $e)
				{
					// No other server: delete the cluster
					$query = 'DELETE FROM `clusters` WHERE `id` = %d;';
					$query = sprintf($query, $cluster_id);
					$res = $db->singleQuery($query);
				}
			}*/
		}
		catch (SQLNoResultException $e)
		{
			// Return failure
			header("YPResponse: 0");
			header("YPMessage: SID does not exist.");
			if (defined('DEBUG'))
				fwrite($fp, "\nYPResponse: 0\nYPMessage: SID does not exist.");
		}
		
		break;
	default:
		if (defined('DEBUG'))
			fwrite($fp, "\nUnrecognized action '".$_REQUEST['action']."'");
}
}
catch (SQLException $e)
{
	if (defined('DEBUG'))
		fwrite($fp, "\nMySQL Error: ".$e->getMessage());
}

if (defined('DEBUG'))
{
	fwrite($fp, "\n");
	fclose($fp);
}

?>
