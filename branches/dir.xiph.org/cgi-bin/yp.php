<?php

include_once(dirname(__FILE__).'/../inc/prepend.php');

define('REQUEST_ADD', 'add');
define('REQUEST_TOUCH', 'touch');
define('REQUEST_REMOVE', 'remove');

define('REQUEST_OK', 1);
define('REQUEST_FAILED', 0);
define('REQUEST_REFUSED', -1);

define('SERVER_REFUSED_MISSING_ARG', 0);
define('SERVER_REFUSED_PARSE_ERROR', 1);
define('SERVER_REFUSED_ILLEGAL_URL', 2);
define('SERVER_REFUSED_DUPLICATE',   3);

// Do we have enough data?
if (!array_key_exists('action', $_REQUEST))
{
	echo("No action key provided.");
	exit();
}

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

function clean_string($str)
{
    $str = mb_ereg_replace('[[:cntrl:]]', '', $str);
    $str = utils::is_utf8($str) ? $str : utf8_encode($str);
    $str = mb_convert_encoding($str, 'UTF-8', 'UTF-8,ISO-8859-1,auto');
    $str = trim($str);
    
    return $str;
}

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
			        throw new ServerRefusedAPIException('Not enough arguments.', SERVER_REFUSED_MISSING_ARG);
		        }
		    }
		    // Remote IP
		    $ip = array_key_exists('REMOTE_ADDR', $_SERVER)
		            ? $_SERVER['REMOTE_ADDR'] : null;
		    // Stream name
		    $stream_name = clean_string($_REQUEST['sn']);
		    // Media type
		    $media_type = strtolower(clean_string($_REQUEST['type']));
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
		    $genre = clean_string($_REQUEST['genre']);
		    $genre = str_replace(array('+', '-', '*', '<', '>', '~', '"', '(', ')', '|', '!', '?', ',', ';', ':', '/'),
							     array(' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '),
							     $genre);
		    $genre = preg_replace('/\s+/', ' ', $genre);
		    $genre_list = array_slice(explode(' ', $genre), 0, 10);
		    // Bitrate
		    $bitrate = clean_string($_REQUEST['b']);
		    // Listen URL
		    $listen_url = clean_string($_REQUEST['listenurl']);
            // Verify the URL
            $url = @parse_url($listen_url);
            if (!$url)
            {
                throw new ServerRefusedAPIException('Could not parse listen_url.', SERVER_REFUSED_PARSE_ERROR, $listen_url);
            }
            if (empty($url['scheme']) || $url['scheme'] != 'http'
                || !array_key_exists('host', $url)
                || !preg_match('/^.*[A-Za-z0-9\-]+\.[A-Za-z0-9]+$/', $url['host'])
                || preg_match('/^(10\.|192\.168\.|127\.)/', $url['host']))
            {
                throw new ServerRefusedAPIException('Illegal listen_url. Incorrect <hostname>.', SERVER_REFUSED_ILLEGAL_URL, $listen_url);
            }
		
		    // Cluster password
		    $cluster_password = array_key_exists('cpswd', $_REQUEST)
		                            ? clean_string($_REQUEST['cpswd'])
		                            : null;
		    // Description
		    $description = array_key_exists('desc', $_REQUEST)
	                        ? clean_string($_REQUEST['desc'])
	                        : null;
		    // URL
		    $url = array_key_exists('url', $_REQUEST)
		            ? clean_string($_REQUEST['url'])
		            : null;
		
		    // Look for the server (same listen URL)
		    $server = Server::retrieveByListenUrl($listen_url);
		    if ($server instanceOf Server)
		    {
		        throw new ServerRefusedAPIException('Entry already in the YP.', SERVER_REFUSED_DUPLICATE, $listen_url);
		    }
	        
		    // Look for the mountpoint (different listen URL, same stream name)
            $mountpoint = Mountpoint::findSimilar($stream_name,
                                                  content_type_lookup($media_type),
                                                  $bitrate);
		
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
		            $mp->incrementCounter(Mountpoint::COUNTER_TOTAL);
		            $mp->incrementCounter(Mountpoint::COUNTER_MEDIA_TYPE);
		            
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
		    
		    // SID
		    $sid = UUIDGen::generate();
		    
		    // Server
		    if ($mp instanceOf Mountpoint)
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
	        
		    // Final response
		    if ($mp instanceOf Mountpoint && $server instanceOf Server)
		    {
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
            APILog::serverRefused($e->getCode(), $e->getListenUrl());
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
		    $sid = preg_replace('/[^A-F0-9\-]/', '', strtoupper(clean_string($_REQUEST['sid'])));
		    // Remote IP
		    $ip = array_key_exists('REMOTE_ADDR', $_SERVER)
		            ? $_SERVER['REMOTE_ADDR'] : null;
		    // Song title
		    $current_song = array_key_exists('st', $_REQUEST)
		                        ? clean_string($_REQUEST['st'])
                                : null;
		    // Listeners
		    $listeners = array_key_exists('listeners', $_REQUEST)
    		                 ? intval($_REQUEST['listeners']) : 0;
		    // Max listeners
		    $max_listeners = array_key_exists('max_listeners', $_REQUEST)
        		             ? intval($_REQUEST['max_listeners']) : 0;
		
		    // Find the server
		    $server = Server::retrieveBySID($sid, true);
		    if (!($server instanceOf Server))
		    {
		        throw new NoSuchSIDAPIException();
		    }
		    
		    // Update the data
		    $server->setCurrentSong($current_song);
		    $server->setListeners($listeners);
		    $server->setLastTouchedFrom($ip);
		    $server->setLastTouchedAt();
		    $res = $server->save();
		    
		    if ($res !== false)
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
			    // Return success
			    header("YPResponse: 1");
			    header("YPMessage: Unable to save new server data.");
			    
                // Log stuff
                APILog::request(REQUEST_TOUCH, false, $server->getListenUrl(),
                                $server->getId(), $server->getMountpointId());
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
		    
		    // Find the server
		    $server = Server::retrieveBySID($sid);
		    if (!($server instanceOf Server))
		    {
		        throw new NoSuchSIDAPIException();
		    }
	        
		    // Remove the data
		    $mp_id = $server->getMountpointId();
		    $server_id = $server->getId();
		    $listen_url = $server->getListenUrl();
	        $res = $server->remove();
	        APILog::serverRemoved($res, $server_id, $mp_id, $listen_url);
		    if ($res)
		    {
		        $mountpoint = Mountpoint::retrieveByPk($mp_id);
		        if (!$mountpoint->hasLinkedServers())
		        {
		            $res = $mountpoint->remove();
		            $mountpoint->decrementCounter(Mountpoint::COUNTER_TOTAL);
		            $mountpoint->decrementCounter(Mountpoint::COUNTER_MEDIA_TYPE);
		            
		            // Log stuff
		            APILog::mountpointRemoved($res, $mp_id, $listen_url);
		        }
	        }
		    
		    // Output final status
		    if ($res)
		    {
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
		catch (NoSuchSIDAPIException $e)
		{
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: SID does not exist.");
		
            // Log stuff
            APILog::request(REQUEST_REMOVE, false, $listen_url,
                            $server_id !== false ? $server_id : null,
                            $mp_id !== false ? $mp_id : null);
		}
	    catch (APIException $e)
	    {
		    // Return failure
		    header("YPResponse: 0");
		    header("YPMessage: Touch impossible. Reason: ".$e->getMessage());
		    header("SID: -1");
		    
            // Log stuff
            APILog::request(REQUEST_REMOVE, false, $listen_url,
                            $server_id !== false ? $server_id : null,
                            $mp_id !== false ? $mp_id : null);
	    }
		
		break;
}

?>
