<?php

/**
 * Connection parameters for dir.xiph.org
 **/

define('CP_DB_HOST', 'localhost');
define('CP_DB_USER', 'dir_xiph_org_t');
define('CP_DB_PASS', '6.NvxjR7B5j3Q');
define('CP_DB_NAME', 'dir_xiph_org_test');

/**
 * Database connection class with semi-hard-coded (?!) login and stuff.
 */
class DirXiphOrgDBC extends DatabaseConnection
{
    private static $instance;
	public $queries = 0;
	public $log = array();
	
	/**
	* Constructor.
	*/
	protected function __construct()
	{
		parent::__construct(CP_DB_HOST, CP_DB_USER, CP_DB_PASS, CP_DB_NAME);
	}
	
	public function query($sql)
	{
		$this->queries++;
		$start = microtime(true);
		
		$res = parent::query($sql);
		$nb = $this->affectedRows();
		
		$time = (microtime(true) - $start) * 1000;
		$this->log[] = array('query'=>$sql,
		                     'time'=>round($time, 3),
		                     'rows'=>$nb);
		
		return $res;
	}
	
	/**
	 * Returns an instance of the DatabaseConnection.
	 */
	public static function getInstance($force_new = false, $auto_connect = true)
	{ 
		if (!isset(self::$instance) || $force_new)
		{
			self::$instance = new DirXiphOrgDBC();
		}
		
		if ($auto_connect)
		{
		    self::$instance->connect();
		}
		
		return self::$instance;
	}
}

?>
