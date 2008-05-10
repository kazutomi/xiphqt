<?php

/**
 * Connection parameters for dir.xiph.org
 **/

if (ENVIRONMENT == 'preprod')
{
	define('DATA_DB_HOST', 'localhost');
	define('DATA_DB_USER', 'dir_xiph_org_t');
	define('DATA_DB_PASS', '6.NvxjR7B5j3Q');
	define('DATA_DB_NAME', 'dir_xiph_org_test');
	
	define('LOG_DB_HOST', 'theetete.radiopytagor.org');
	define('LOG_DB_USER', 'dir_xiph_org_t');
	define('LOG_DB_PASS', 'y2V1ydXuXwLKg');
	define('LOG_DB_NAME', 'dir_xiph_org_test');
}
elseif (ENVIRONMENT == 'prod')
{
	define('DATA_DB_HOST', 'localhost');
	define('DATA_DB_USER', 'dir_xiph_org');
	define('DATA_DB_PASS', '5wCjLEVmAJnmM');
	define('DATA_DB_NAME', 'dir_xiph_org';
	
	define('LOG_DB_HOST', 'theetete.radiopytagor.org');
	define('LOG_DB_USER', 'dir_xiph_org');
	define('LOG_DB_PASS', 'xf0M1E1cup0FM');
	define('LOG_DB_NAME', 'dir_xiph_org');
}
else
{
//	die("Unable to do this on test atm.\n");
	define('DATA_DB_HOST', 'localhost');
	define('DATA_DB_USER', 'dir_xiph_org_t');
	define('DATA_DB_PASS', 'pSqvUfzxL5qXz.UH');
	define('DATA_DB_NAME', 'dir_xiph_org_test');
	
	define('LOG_DB_HOST', 'localhost');
	define('LOG_DB_USER', 'dir_xiph_org_t');
	define('LOG_DB_PASS', 'pSqvUfzxL5qXz.UH');
	define('LOG_DB_NAME', 'dir_xiph_org_test');
}

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
		parent::__construct(DATA_DB_HOST, DATA_DB_USER, DATA_DB_PASS, DATA_DB_NAME);
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

class DirXiphOrgLogDBC extends DirXiphOrgDBC
{
	/**
	* Constructor.
	*/
	protected function __construct()
	{
		parent::__construct(LOG_DB_HOST, LOG_DB_USER, LOG_DB_PASS, LOG_DB_NAME);
	}
}

?>
