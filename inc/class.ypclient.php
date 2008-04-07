<?php

class YPClient
{
	protected $url;
	
	public function __construct($url)
	{
		$this->url = $url;
	}
	
	public function addServer($name, $type, $genre, $bitrate, $listen_url,
							  $desc=null, $url=null, $stype=null,
							  $cluster_password=null)
	{
		// Mandatory parameters
		$query_string = "?action=add&sn=%s&type=%s&genre=%s&b=%s&listenurl=%s";
		$query_string = sprintf($query_string,
								urlencode($name),
								urlencode($type),
								urlencode($genre),
								urlencode($bitrate),
								urlencode($listen_url));
		
		// Optional parameters
		if ($desc !== null)
		{
			$query_string .= '&desc='.urlencode($desc);
		}
		if ($url !== null)
		{
			$query_string .= '&url='.urlencode($url);
		}
		if ($stype !== null)
		{
			$query_string .= '&stype='.urlencode($stype);
		}
		if ($cpswd !== null)
		{
			$query_string .= '&cpswd='.urlencode($cpswd);
		}
		
		$url = $this->url.$query_string;
		$result = $this->makeAPICall($url);
		
		if (array_key_exists('YPResponse', $result)
			&& intval($result['YPResponse']) == 1
			&& array_key_exists('SID', $result))
		{
			return $result['SID'];
		}
		else
		{
			return false;
		}
	}
	
	public function touch($sid, $song_title=null, $listeners=null,
						  $max_listeners=null)
	{
		$url = $this->url.'?action=touch&sid='.urlencode($sid);
		if ($song_title !== null)
		{
			$url .= '&st='.urlencode($song_title);
		}
		if ($listeners !== null)
		{
			$url .= '&listeners='.urlencode($listeners);
		}
		if ($max_listeners !== null)
		{
			$url .= '&max_listeners='.urlencode($max_listeners);
		}
		$result = $this->makeAPICall($url);
		
		return (array_key_exists('YPResponse', $result)
				&& intval($result['YPResponse']) == 1);
	}
	
	public function removeServer($sid)
	{
		$url = $this->url.'?action=remove&sid='.urlencode($sid);
		$result = $this->makeAPICall($url);
		
		return (array_key_exists('YPResponse', $result)
				&& intval($result['YPResponse']) == 1);
	}
	
	protected function makeAPICall($url)
	{
		$url = parse_url($url);
		$fp = fsockopen($url['host'],
						array_key_exists('port', $url) ? $url['port'] : 80,
						$errno,
						$errstr,
						2); // 2 s timeout
		if (!$fp)
		{
			printf("Error %d: %s.\n", $errno, $errstr);
			
			return false;
		}
		
		fwrite($fp, sprintf("GET %s?%s HTTP/1.0\r\n\r\n", $url['path'],
														  $url['query']));
		$data = array();
		while ($n = fgets($fp))
		{
			if (trim($n) != '')
			{
				list($header, $value) = explode(':', $n);
				$data[trim($header)] = trim($value);
			}
		}
		fclose($fp);
		
		return $data;
	}
}

function make_prononcable_string($length = 10, $length_variation = 0.1, $space_probability = 20)
{
	$vowels = array('a', 'e', 'i', 'o', 'u', 'y');
	$consonnants = array('b', 'c', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n',
				 		 'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'z');
	
	$l = rand($length - ($length * $length_variation),
			  $length + ($length * $length_variation));
	$acc0 = 0;
	$acc1 = 0;
	$str = '';
	for ($i = 0 ; $i < $l ; $i++)
	{
		$r = rand(0, $acc1);
		if ($r == 0)
		{
			$str .= ' ';
			$acc1 = $space_probability;
		}
		else
		{
			if ($acc0 % 2 == 0)
			{
				shuffle($consonnants);
				$str .= current($consonnants);
			}
			else
			{
				shuffle($vowels);
				$str .= current($vowels);
			}
		}
		
		$acc0++;
		$acc1--;
	}
	
	return preg_replace('/ +/', ' ', trim($str));
}

// New client
$c = new YPClient("http://localhost/dir.xiph.org/yp.php");

// Loop
$stream_types = array('audio/mpeg', 'application/ogg+vorbis',
					  'audio/aac', 'audio/aacp', 'application/ogg+theora',
					  'video/nsv');
$stations = array();
$creation_probability = 10;
$deletion_probability = 100;
while (true)
{
	foreach ($stations as $k=>$s)
	{
		if ($s['timeout'] == 0)
		{
			// Stream title
			$artist = ucwords(make_prononcable_string(13, 0.4, 10));
			$song = ucwords(make_prononcable_string(20, 0.7, 10));
			$st = sprintf('%s -- %s', $artist, $song);
			
			// Listeners
			$listeners_delta = $s['previous_listeners'] * 0.4;
			$listeners = rand(min($s['previous_listeners'] - $listeners_delta,
								  0),
							  max($s['previous_listeners'] + $listeners_delta,
								  $s['max_listeners']));
			$listeners += rand(0, max(10, $listeners_delta)); // kickstart + random peaks
			
			printf("Touching SID %s\n", $s['sid']);
			$c->touch($s['sid'], $st, $listeners, $s['max_listeners']);
			$stations[$k]['previous_listeners'] = $listeners;
			$stations[$k]['timeout'] = rand(25, 35); // ~30 s
		}
		else
		{
			$stations[$k]['timeout']--;
		}
	}
	
	if (rand(0, $creation_probability) == 0)
	{
		// Name
		$sn = ucwords(make_prononcable_string(10, 0.1, 15));
		
		// Type
		$type = $stream_types[array_rand($stream_types)];
		
		// Tags
		$tags = make_prononcable_string(25, 0.4, 10);
		
		// Bitrate
		$bitrate = rand(0, 384);
		
		// URL
		$url = 'http://localhost:'.rand(1024, 65535).'/'
				.preg_replace('/ +/', '-', strtolower($sn));
		
		// Description
		$desc = ucfirst(make_prononcable_string(40, 0.4, 10));
		
		// Now register
		$sid = $c->addServer($sn, $type, $tags, $bitrate, $url, $desc);
		printf("Added server '%s', SID %s\n", $sn, $sid);
		
		if ($sid !== false)
		{
			// Remember, camembert.
			$max_listeners = rand(300, 30000);
			$timeout = rand(1, 5);
			$s = array('name'=>$sn, 'type'=>$type, 'tags'=>$tags,
					   'bitrate'=>$bitrate, 'url'=>$url,
					   'previous_listeners'=>0, 'max_listeners'=>0,
					   'timeout'=>$timeout, 'sid'=>$sid);
			$stations[] = $s;
		}
		else
		{
			echo("Unable to register server.\n");
		}
	}
	
	if (rand(0, $deletion_probability) == 0)
	{
		$id = array_rand($stations);
		if ($id !== false)
		{
			$sid = $stations[$id]['sid'];
		
			printf("Removing SID %s\n", $sid);
			if ($c->removeServer($sid))
			{
				unset($stations[$id]);
			}
		}
	}
	
	sleep(1);
}

/*
$sid = $c->addServer('Test Server', 'audio/mpeg', 'Random', 32, 'http://localhost:2435/prout.ogg');
var_dump($sid);
var_dump($c->touch($sid, 'Unknown', 234, 2452));
var_dump($c->removeServer($sid));
*/
?>