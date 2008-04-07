<?php

abstract class MemcacheConnection extends Memcache
{
	
	/**
	 * Constructor.
	 * 
	 * @param string $host
	 * @param string $port
	 */
	protected function __construct($host, $port = 11211)
	{
		$this->addServer($host, $port);
	}
	
	/**
	 * Returns an instance of the MemcacheConnection.
	 */
	abstract public static function getInstance($force_new = false);
}

?>
