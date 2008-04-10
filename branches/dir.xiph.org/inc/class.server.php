<?php

class Server
{
    protected $server_id;
    protected static $table_name = 'server';
    protected $cache_expiration = 60;
    public $loaded = false;
    
    protected $mountpoint_id;
    protected $sid;
    protected $current_song;
    protected $listen_url;
    protected $listeners;
    protected $last_touched_at;
    protected $last_touched_from;
    
    public function __construct($server_id, $force_reload = false, $no_load = false)
    {
        if (!is_numeric($server_id) || $server_id < 0)
        {
            throw new BadIdException($mountpoint_id);
        }
        $this->server_id = intval($server_id);
        
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
                }
            }
        }
    }
    
    /**
     * Saves the server data both into the database and into the cache.
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
     * Retrieves the server from either the db or the cache.
     * 
     * @return Server or false if an error occured.
     */
    public static function retrieveByPk($pk)
    {
        $s = new Server($pk);
        
        return ($s->loaded ? $s : false);
    }
    
    /**
     * Retrieves the server from the db.
     * 
     * @return Server or false if an error occured.
     */
    public static function retrieveBySID($sid)
    {
        // MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		try
		{
		    $sql = "SELECT `id` FROM %s WHERE `sid` = '%s';";
		    $sql = sprintf($sql, self::$table_name, mysql_real_escape_string($sid));
		    $res = $db->singleQuery($sql);
	    }
	    catch (SQLNoResultException $e)
		{
		    return false;
		}
		
		return self::retrieveByPk(intval($res->current('id')));
    }
    
    /**
     * Saves the server into the database.
     * 
     * @return integer
     */
    protected function saveIntoDb()
    {
        // MySQL Connection
		$db = DirXiphOrgDBC::getInstance();
		
		// Query
        $query = 'INSERT INTO `%1$s` (`mountpoint_id`, `sid`, `current_song`, `listen_url`, `listeners`, `last_touched_at`, `last_touched_from`) '
			    .'VALUES (%2$d, "%3$s", %4$s, "%5$s", %6$d, %7$s, INET_ATON("%8$s")) '
			    .'ON DUPLICATE KEY UPDATE `mountpoint_id` = %2$d, `sid` = "%3$s", `current_song` = %4$s, `listen_url` = "%5$s", `listeners` = %6$d, `last_touched_at` = %7$s, `last_touched_from` = INET_ATON("%8$s");';
	    $query = sprintf($query, self::$table_name,
	                             $this->mountpoint_id,
							     mysql_real_escape_string($this->sid),
							     ($this->current_song != null) ? '"'.mysql_real_escape_string($this->current_song).'"' : 'NULL',
							     mysql_real_escape_string($this->listen_url),
							     ($this->listeners != null) ? $this->listeners : 0,
							     ($this->last_touched_at != null) ? '"'.mysql_real_escape_string($this->last_touched_at).'"' : 'NOW()',
							     mysql_real_escape_string($this->last_touched_from));
	    $server_id = $db->insertQuery($query);
	    
	    return $server_id;
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
            $query = "SELECT `mountpoint_id`, `sid`, `current_song`, `listen_url`, `listeners`, `last_touched_at`, INET_NTOA(`last_touched_from`) AS `last_touched_from` FROM `%s` WHERE `id` = %d;";
            $query = sprintf($query, self::$table_name, $this->server_id);
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
        
        return sprintf("%s_server_%d", ENVIRONMENT, $this->server_id);
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
        $this->mountpoint_id = $a['mountpoint_id'];
        $this->sid = $a['sid'];
        $this->current_song = $a['current_song'];
        $this->listen_url = $a['listen_url'];
        $this->listeners = intval($a['listeners']);
        $this->last_touched_at = $a['last_touched_at'];
        $this->last_touched_from = $a['last_touched_from'];
        
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
        
        $a['mountpoint_id'] = $this->mountpoint_id;
        $a['sid'] = $this->sid;
        $a['current_song'] = $this->current_song;
        $a['listen_url'] = $this->listen_url;
        $a['listeners'] = intval($this->listeners);
        $a['last_touched_at'] = $this->last_touched_at;
        $a['last_touched_from'] = $this->last_touched_from;
        
        return $a;
    }
    
    public function getId()
    {
        return $this->server_id;
    }
    
    public function setMountpointId($mp_id)
    {
        $this->mountpoint_id = $mp_id;
    }
    
    public function getMountpointId()
    {
        return $this->mountpoint_id;
    }
    
    public function setSid($s)
    {
        $this->sid = $s;
    }
    
    public function getSid()
    {
        return $this->sid;
    }
    
    public function setCurrentSong($cs)
    {
        $this->current_song = $cs;
    }
    
    public function getCurrentSong()
    {
        return $this->current_song;
    }
    
    public function setListenUrl($u)
    {
        $this->listen_url = $u;
    }
    
    public function getListenUrl()
    {
        return $this->listen_url;
    }
    
    public function setListeners($l)
    {
        $this->listeners = $l;
    }
    
    public function getListeners()
    {
        return $this->listeners;
    }
    
    public function setLastTouchedAt($at = null)
    {
        if ($at === null)
        {
            $at = date('Y-m-d H:i:s');
        }
        
        $this->last_touched_at = $at;
    }
    
    public function getLastTouchedAt()
    {
        return $this->last_touched_at;
    }
    
    public function setLastTouchedFrom($f)
    {
        $this->last_touched_from = $f;
    }
    
    public function getLastTouchedFrom()
    {
        return $this->last_touched_from;
    }
}

?>
