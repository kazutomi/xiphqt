<?php

define('MC_HOST', 'localhost');
define('MC_PORT', 11211);

class DirXiphOrgMCC extends MemcacheConnection
{
	/**
	 * Singleton instance.
	 */
	protected static $instance;
	public $gets = 0;
	public $sets = 0;
	public $log = array();
	
    protected function __construct()
    {
        parent::__construct(MC_HOST, MC_PORT);
    }
    
    public function get($key)
    {
        $this->gets++;
		$start = microtime(true);
		
		$res = parent::get($key);
		
		$time = (microtime(true) - $start) * 1000;
		$this->log[] = array('key'=>is_string($key) ? $key : implode(', ', $key),
		                     'hit'=>($res !== false),
		                     'time'=>round($time, 3),
		                     'type'=>'get');
		
		return $res;
    }
    
    public function set($key, $var, $flag=0, $expire=0)
    {
        $this->sets++;
		$start = microtime(true);
		
		$res = parent::set($key, $var, $flag, $expire);
		
		$time = (microtime(true) - $start) * 1000;
		$this->log[] = array('key'=>$key,
		                     'ok'=>($res != false),
		                     'time'=>round($time, 3),
		                     'type'=>'set');
		
		return $res;
    }
	
	/**
	 * Returns an instance of the MemcacheConnection.
	 */
	public static function getInstance($force_new = false)
	{ 
		if (!isset(self::$instance) || $force_new)
		{
			self::$instance = new DirXiphOrgMCC();
		}
		
		return self::$instance;
	}
}

?>
