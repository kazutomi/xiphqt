<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

class ToDeleteException extends Exception { }

// Lock
if (!defined('ENVIRONMENT'))
{
    throw new EnvironmentUndefinedException();
}

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Old stuff that "timeouted"
try
{
    $res = $db->selectQuery('SELECT `id`, `listen_url` FROM `server` WHERE `checked` = 0;');
    while (!$res->endOf())
    {
        try
        {
            printf("Processing %s...\n", $res->current('listen_url'));
            // Get the URL
            $url = @parse_url($res->current('listen_url'));
            if (!$url)
            {
                throw new ToDeleteException();
            }
            
            // Now, verify!
            if (empty($url['scheme']) || $url['scheme'] != 'http'
                || !array_key_exists('host', $url)
                || !preg_match('/^.*[A-Za-z0-9\-]+\.[A-Za-z0-9]+$/', $url['host'])
                || preg_match('/^(10\.|192\.168\.|127\.)/', $url['host']))
            {
                throw new ToDeleteException();
            }

            // Check if this is a radionomy host.
            // If yes, trust it blindly as they can't figure out their IDS and constantly fail checks and destabilize listings.
            if (preg_match('/^streaming20.\.radionomy.com$/', $url['host']))
            {
                $ok = true;
            }
            else
            {
                // Try to open a connection to the server
                $ok = false;
                $count = 0;
                while ($count < 3 && !$ok)
                {
                    $fp = @fsockopen($url['host'],
                                     array_key_exists('port', $url) ? $url['port'] : 80, // as per HTTP RFC
                                     $errno, $errstr, 3);
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
                        fclose($fp);
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
                
                    // Extremely dangerous, disabled.
/*                  if (!array_key_exists('server', $headers)
                        || !stristr($headers['server'], 'icecast'))
                    {
                        throw new ToDeleteException();
                    }*/
                    fclose($fp);
                
                    $count++;
                    $ok = true;
                }
            }
            if (!$ok)
            {
                throw new ToDeleteException();
            }
            
            // If we're here, everything's ok.
            $sql = 'UPDATE `server` SET `checked` = 1, `checked_at` = NOW() WHERE `id` = %d;';
            $sql = sprintf($sql, $res->current('id'));
            $db->noReturnQuery($sql);
        } 
        catch (ToDeleteException $e)
        {
            // TODO: remove the stream
            echo("Delete it! ".$res->current('listen_url')."\n");
            
/*            $sql = 'UPDATE `server` SET `checked` = 2, `checked_at` = NOW() WHERE `id` = %d;';
            $sql = sprintf($sql, $res->current('id'));
            $db->noReturnQuery($sql);*/
            $server = Server::retrieveByPk($res->current('id'));
	    if ($server instanceOf Server)
	    {
            $mp_id = $server->getMountpointId();
	        $mountpoint = Mountpoint::retrieveByPk($mp_id);
	        $server->remove();
	        if ($mountpoint instanceOf Mountpoint
	        	&& !$mountpoint->hasLinkedServers())
	        {
	        	$mountpoint->remove();
	        }
	    }
        }
        
        $res->next();
    }
}
catch (SQLNoResultException $e)
{
    echo "OK.\n";
}

?>
