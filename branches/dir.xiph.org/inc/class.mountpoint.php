<?php

class Mountpoint
{
    protected $mountpoint_id = null;
    protected $table_name = 'mountpoint';
    protected $cache_expiration = 60;
    public $loaded = false;
    
    protected $stream_name;
    protected $description;
    protected $url;
    protected $listeners;
    protected $current_song;
    protected $media_type;
    protected $media_type_id;
    protected $bitrate;
    protected $channels;
    protected $samplerate;
    protected $cluster_password;
    
    public function __construct($mountpoint_id, $force_reload = false, $no_load = false)
    {
        if (!is_numeric($mountpoint_id) || $mountpoint_id < 0)
        {
            throw new BadIdException("Bad ID: " . (string) $mountpoint_id);
        }
        $this->mountpoint_id = intval($mountpoint_id);
        
        if (!$no_load)
        {
            if ($force_reload)
            {
                $this->loadFromDb();
            }
            else
            {
                if (!$this->loadFromCache())
                {
                    $this->loadFromDb();
                    $this->saveIntoCache();
                }
            }
        }
    }
    
    /**
     * Saves the mountpoint data both into the database and into the cache.
     * 
     * @return boolean
     */
    public function save()
    {
        $mp_id = $this->saveIntoDb();
        $this->saveIntoCache();
        
        return $mp_id;
    }
    
    /**
     * Retrieves the mountpoint from either the db or the cache.
     * 
     * @return Mountpoint or false if an error occured.
     */
    public static function retrieveByPk($pk)
    {
        $m = new Mountpoint($pk);
        
        return ($m->loaded ? $m : false);
    }
    
    /**
     * Saves the mountpoint into the database.
     * 
     * @return integer
     */
    protected function saveIntoDb()
    {
        // MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Query
        $query = 'INSERT INTO `%1$s` (`stream_name`, `description`, `url`, `listeners`, `current_song`, `media_type_id`, `bitrate`, `channels`, `samplerate`, `cluster_password`) '
			    .'VALUES ("%2$s", %3$s, %4$s, %5$d, %6$s, %7$d, "%8$s", %9$s, %10$s, %11$s) '
			    .'ON DUPLICATE KEY UPDATE `stream_name` = "%2$s", `description` = %3$s, `url` = %4$s, `listeners` = %5$d, `current_song` = %6$s, `media_type_id` = %7$d, `bitrate` = "%8$s", `channels` = %9$s, `samplerate` = %10$s, `cluster_password` = %11$s;';
	    $query = sprintf($query, $this->table_name,
	                             mysql_real_escape_string($this->stream_name),
							     ($this->description != null) ? '"'.mysql_real_escape_string($this->description).'"' : 'NULL',
							     ($this->url != null) ? '"'.mysql_real_escape_string($this->url).'"' : 'NULL',
							     ($this->listeners != null) ? $this->listeners : 0,
							     ($this->current_song != null) ? '"'.mysql_real_escape_string($this->current_song).'"' : 'NULL',
							     $this->media_type_id,
							     mysql_real_escape_string($this->bitrate),
							     ($this->channels != null) ? intval($this->channels) : 'NULL',
							     ($this->samplerate != null) ? intval($this->samplerate) : 'NULL',
							     ($this->cluster_password != null) ? '"'.mysql_real_escape_string($this->cluster_password).'"' : 'NULL');
	    $mp_id = $db->insertQuery($query);
	    if ($mp_id !== false && $this->mountpoint_id == 0)
	    {
	        $this->mountpoint_id = $mp_id;
	    }
	    
	    return $mp_id;
    }
    
    /**
     * Loads the data from the database.
     * 
     * @return boolean
     */
    protected function loadFromDb()
    {
        // MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Query
        try
        {
            $query = "SELECT * FROM `%s` WHERE `id` = %d;";
            $query = sprintf($query, $this->table_name, $this->mountpoint_id);
            $m = $db->singleQuery($query);
            
            $this->loadFromArray($m->array_data[0]);
            
            return true;
        }
        catch (SQLNoResultException $e)
        {
            return false;
        }
    }
    
    /**
     * Saves the data into the cache.
     * 
     * @return boolean
     */
    protected function saveIntoCache()
    {
        // Memcache connection
        $cache = DirXiphOrgMCC::getInstance();
        
        $a = $this->__toArray();
        
        return $cache->set($this->getCacheKey(), $a, false, $this->cache_expiration);
    }
    
    /**
     * Loads the data from the cache.
     * 
     * @return boolean
     */
    protected function loadFromCache()
    {
        // Memcache connection
        $cache = DirXiphOrgMCC::getInstance();
        
        $a = $cache->get($this->getCacheKey());
        if ($a === false)
        {
            return false;
        }
        
        return $this->loadFromArray($a);
    }
    
    /**
     * Builds a cache key for this mountpoint.
     * 
     * @return string
     */
    protected function getCacheKey()
    {
        if (!defined('ENVIRONMENT'))
        {
            throw new EnvironmentUndefinedException();
        }
        
        return sprintf("%s_mountpoint_%d", ENVIRONMENT, $this->mountpoint_id);
    }
    
    /**
     * Serializes the data into an array
     * 
     * @return array
     */
    protected function saveIntoArray()
    {
        return $this->__toArray();
    }
    
    /**
     * Loads the data from an array.
     * 
     * @return boolean
     */
    protected function loadFromArray($a)
    {
        $this->stream_name = $a['stream_name'];
        $this->description = $a['description'];
        $this->url = $a['url'];
        $this->listeners = intval($a['listeners']);
        $this->current_song = $a['current_song'];
        $this->media_type_id = $a['media_type_id'];
        $this->bitrate = $a['bitrate'];
        $this->channels = $a['channels'];
        $this->samplerate = $a['samplerate'];
        $this->cluster_password = $a['cluster_password'];
        
        $this->loaded = true;
        
        return true;
    }
    
    /**
     * Packs the object contents into an array.
     * 
     * @return array
     */
    protected function __toArray()
    {
        $a = array();
        
        $a['stream_name'] = $this->stream_name;
        $a['description'] = $this->description;
        $a['url'] = $this->url;
        $a['listeners'] = intval($this->listeners);
        $a['current_song'] = $this->current_song;
        $a['media_type_id'] = $this->media_type_id;
        $a['bitrate'] = $this->bitrate;
        $a['channels'] = $this->channels;
        $a['samplerate'] = $this->samplerate;
        $a['cluster_password'] = $this->cluster_password;
        
        return $a;
    }
    
    public function getId()
    {
        return $this->mountpoint_id;
    }
    
    public function setStreamName($sn)
    {
        $this->stream_name = $sn;
    }
    
    public function getStreamName()
    {
        return $this->stream_name;
    }
    
    public function setDescription($d)
    {
        $this->description = $d;
    }
    
    public function getDescription()
    {
        return $this->description;
    }
    
    public function setUrl($u)
    {
        $this->url = $u;
    }
    
    public function getUrl()
    {
        return $this->url;
    }
    
    public function setListeners($l)
    {
        $this->listeners = $l;
    }
    
    public function getListeners()
    {
        return $this->listeners;
    }
    
    public function setCurrentSong($cs)
    {
        $this->current_song = $cs;
    }
    
    public function getCurrentSong()
    {
        return $this->current_song;
    }
    
    public function setMediaTypeId($mti)
    {
        $this->media_type_id = $mti;
    }
    
/*    public function setMediaType(MediaType $mt)
    {
        $this->media_type = $mt;
        $this->media_type_id = $mt->getId();
    }*/
    
    public function getMediaTypeId()
    {
        return $this->media_type_id;
    }
    
/*    public function getMediaType()
    {
        if (!isset($this->media_type))
        {
            $this->media_type = new MediaType($this->media_type_id);
        }
        
        return $this->media_type;
    }*/
    
    public function setBitrate($b)
    {
        $this->bitrate = $b;
    }
    
    public function getBitrate()
    {
        return $this->bitrate;
    }
    
    public function setChannels($c)
    {
        $this->channels = $c;
    }
    
    public function getChannels()
    {
        return $this->channels;
    }
    
    public function setSamplerate($s)
    {
        $this->samplerate = $s;
    }
    
    public function getSamplerate()
    {
        return $this->samplerate;
    }
    
    public function setClusterPassword($cp)
    {
        $this->cluster_password = $cp;
    }
    
    public function getClusterPassword()
    {
        return $this->cluster_password;
    }
    
    public function getTags()
    {
        return Tag::getMountpointTags($this->mountpoint_id);
    }
}

?>
