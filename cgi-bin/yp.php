<?php

include_once(dirname(__FILE__).'/../inc/prepend.php');

define('REQUEST_ADD', 'add');
define('REQUEST_TOUCH', 'touch');
define('REQUEST_REMOVE', 'remove');

define('REQUEST_OK', 1);
define('REQUEST_FAILED', 0);
define('REQUEST_REFUSED', -1);

// Do we have enough data?
if (!array_key_exists('action', $_REQUEST))
{
	echo("No action key provided.");
	exit();
}

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Then process it
switch ($_REQUEST['action'])
{
	case REQUEST_ADD:
		// TODO: check that the data we get is consistent with the previous cluster data.
		try
		{
		    // Check the args are here
		    $mandatory_args = array('sn', 'type', 'genre', 'b', 'listenurl');
		    foreach ($mandatory_args as $a)
		    {
		        if (!array_key_exists($a, $_REQUEST))
		        {
			        throw new ServerRefusedAPIException('Not enough arguments.');
		        }
		    }
		    // Remote IP
		    $ip = array_key_exists('REMOTE_ADDR', $_SERVER)
		            ? $_SERVER['REMOTE_ADDR'] : null;
		    // Stream name
		    $stream_name = mb_convert_encoding($_REQUEST['sn'], 'UTF-8',
                                               'UTF-8,ISO-8859-1,auto');
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
		    $genre = mb_convert_encoding($_REQUEST['genre'], 'UTF-8',
                                         'UTF-8,ISO-8859-1,auto');
		    $genre = str_replace(array('+', '-', '*', '<', '>', '~', '"', '(', ')', '|', '!', '?', ',', ';', ':'),
							     array('', '', '', '', '', '', '', '', '', '', '', '', '', '', '', ''),
							     $genre);
		    $genre = preg_replace('/\s+/', ' ', $genre);
		    $genre_list = array_slice(explode(' ', $genre), 0, 10);
		    // Bitrate
		    $bitrate = $_REQUEST['b'];
		    // Listen URL
		    $listen_url = $_REQUEST['listenurl'];
            // Verify the URL
            $url = @parse_url($listen_url);
            if (!$url)
            {
                throw new ServerRefusedAPIException('Could not parse listen_url.');
            }
            if (empty($url['scheme']) || $url['scheme'] != 'http'
                || !array_key_exists('host', $url)
                || !preg_match('/^.*[A-Za-z0-9\-]+\.[A-Za-z0-9]+$/', $url['host'])
                || preg_match('/^(10\.|192\.168\.|127\.)/', $url['host']))
            {
                throw new ServerRefusedAPIException('Illegal listen_url.');
            }
		
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
		    $query = 'SELECT `id` FROM `mountpoint` WHERE `stream_name` = "%s" AND `media_type_id` = %d AND `bitrate` = %d;';
		    $query = sprintf($query, mysql_real_escape_string($stream_name), content_type_lookup($media_type), mysql_real_escape_string($bitrate));
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
			
			    if ($mp_id !== false)
			    {
			        // Log
			        APILog::mountpointAdded(true, $mp_id, $listen_url);
			        
				    // Add the tags
				    Tag::massTagMountpoint($mp, $genre_list);
			    }
			    else
			    {
			        $mp = false;
			        
			        // Log
			        APILog::mountpointAdded(false, $mp_id, $listen_url);
			    }
		    }
		
		    // Server
		    if ($mp instanceOf Mountpoint && !$server_id)
		    {
		        $server = new Server(0, false, true);
		        $server->setMountpointId($mp_id);
		        $server->setSid($sid);
		        $server->setListenUrl($listen_url);
		        $server->setLastTouchedFrom($ip);
		        
		        $server_id = $server->save();
		        
		        if ($server_id !== false)
		        {
        		    // Log
        		    APILog::serverAdded(true, $server_id, $mp_id, $listen_url);
		        }
		        else
		        {
		            $server = false;
		            
        		    // Log
        		    APILog::serverAdded(false, $server_id, $mp_id, $listen_url);
		        }
		    }
	        
		    // Tags and stuff
		    if ($mp instanceOf Mountpoint && $server instanceOf Server)
		    {
			    // Increment the "total servers" key in memcache
			    if (!$memcache->increment(ENVIRONMENT.'_servers_total'))
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
                
                // Log stuff
                APILog::request(REQUEST_ADD, true, $listen_url, $server_id,
                                $mp_id);
		    }
		    else
		    {
			    // Return failure
			    header("YPResponse: 0");
			    header("YPMessage: Error occured while processing your request.");
			    header("SID: -1");
                
                // Log stuff
                APILog::request(REQUEST_ADD, false, $listen_url,
                                $server_id !== false ? $server_id : null,
                                $mp_id !== false ? $mp : null);
		    }
	    }
	    catch (ServerRefusedAPIException $e)
	    {
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: Add refused. Reason: ".$e->getMessage());
		    header("SID: -1");
		    
            // Log stuff
            APILog::request(REQUEST_ADD, REQUEST_REFUSED, $listen_url,
                            $server_id !== false ? $server_id : null,
                            $mp_id !== false ? $mp_id : null);
	    }
		
		break;
	case REQUEST_TOUCH:
	    try
	    {
		    // Check the args are here
		    $mandatory_args = array('sid');
		    foreach ($mandatory_args as $a)
		    {
		        if (!array_key_exists($a, $_REQUEST))
		        {
			        // Return failure
			        header("YPResponse: 0");
			        header("YPMessage: Not enough arguments.");
			        header("SID: -1");
			        
			        throw new APIException("Not enough arguments.");
		        }
		    }
		
		    // SID
		    $sid = preg_replace('/[^A-F0-9\-]/', '', strtoupper ($_REQUEST['sid']));
		    // Remote IP
		    $ip = array_key_exists('REMOTE_ADDR', $_SERVER)
		            ? $_SERVER['REMOTE_ADDR'] : null;
		    // Song title
		    $current_song = array_key_exists('st', $_REQUEST)
		                    ? mb_convert_encoding($_REQUEST['st'], 'UTF-8',
                                                  'UTF-8,ISO-8859-1,auto')
                            : null;
		    // Listeners
		    $listeners = array_key_exists('listeners', $_REQUEST)
		                 ? intval($_REQUEST['listeners']) : 0;
		    // Max listeners
		    $max_listeners = array_key_exists('max_listeners', $_REQUEST)
        		             ? intval($_REQUEST['max_listeners']) : 0;
		
		    // MySQL Connection
		    $db = DirXiphOrgDBC::getInstance();
		
		    // Update the data
		    $server = Server::retrieveBySID($sid);
		    if (!($server instanceOf Server))
		    {
		        throw new NoSuchSIDAPIException();
		    }
		    $query = 'UPDATE `server` SET `current_song` = "%s", `listeners` = %d, `last_touched_from` = INET_ATON("%s"), `last_touched_at` = NOW() WHERE `sid` = "%s";';
		    $query = sprintf($query, mysql_real_escape_string($current_song), $listeners, mysql_real_escape_string($ip), mysql_real_escape_string($sid));
		    $res = $db->noReturnQuery($query);
		    if ($res && $db->affected_rows > 0)
		    {
			    // Return success
			    header("YPResponse: 1");
			    header("YPMessage: Updated server info.");
			    
                // Log stuff
                APILog::request(REQUEST_TOUCH, true, $server->getListenUrl(),
                                $server->getId(), $server->getMountpointId());
		    }
		    else
		    {
		        throw new NoSuchSIDAPIException();
		    }
		}
		catch (NoSuchSIDAPIException $e)
		{
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: SID does not exist.");
		    
            // Log stuff
            APILog::request(REQUEST_TOUCH, false, null, null, null);
		}
	    catch (APIException $e)
	    {
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: Touch impossible. Reason: ".$e->getMessage());
		    header("SID: -1");
		    
            // Log stuff
            APILog::request(REQUEST_TOUCH, false, $listen_url,
                            $server_id !== false ? $server_id : null,
                            $mp_id !== false ? $mp_id : null);
	    }
		
		break;
	case REQUEST_REMOVE:
	    try
	    {
		    // Check the args are here
		    $mandatory_args = array('sid');
		    foreach ($mandatory_args as $a)
		    {
		        if (!array_key_exists($a, $_REQUEST))
		        {
			        // Return failure
			        header("YPResponse: 0");
			        header("YPMessage: Not enough arguments.");
			        header("SID: -1");
			        
			        throw new APIException("Not enough arguments.");
		        }
		    }
		
		    // SID
		    $sid = preg_replace('/[^a-f0-9\-]/', '', strtolower($_REQUEST['sid']));
		
		    // MySQL Connection
		    $db = DirXiphOrgDBC::getInstance();
		
		    // Remove the data
		    $server = Server::retrieveBySID($sid);
		    if (!($server instanceOf Server))
		    {
		        throw new NoSuchSIDAPIException();
		    }
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
				    $memcache->decrement(ENVIRONMENT.'_servers_total');
				    $memcache->decrement(ENVIRONMENT.'_servers_'.intval($media_type));
				
				    // Return success
				    header("YPResponse: 1");
				    header("YPMessage: Deleted server info.");
				    
                    // Log stuff
                    APILog::request(REQUEST_REMOVE, true, $server->getListenUrl(),
                                    $server->getId(), $server->getMountpointId());
			    }
			    else
			    {
				    // Return failure
				    header("YPResponse: 0");
				    header("YPMessage: Error occured while processing your request.");
				    
                    // Log stuff
                    APILog::request(REQUEST_REMOVE, false, $server->getListenUrl(),
                                    $server->getId(), $server->getMountpointId());
			    }
		    }
		    catch (SQLNoResultException $e)
		    {
			    throw new NoSuchSIDAPIException();
		    }
		}
		catch (NoSuchSIDAPIException $e)
		{
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: SID does not exist.");
		
            // Log stuff
            APILog::request(REQUEST_REMOVE, false, $listen_url,
                            $server_id !== false ? $server_id : null,
                            $mountpoint_id !== false ? $mountpoint_id : null);
		}
		
		break;
}

?>
