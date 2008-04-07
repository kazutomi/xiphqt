<?php

class BadIDException extends Exception { }
class EnvironmentUndefinedException extends Exception { }

if (getenv('ENVIRONMENT') !== false)
{
	define('ENVIRONMENT', strtolower(getenv('ENVIRONMENT')));
}
elseif ($_SERVER['SERVER_NAME'] == 'directory-test.radiopytagor.net')
{
	define('ENVIRONMENT', 'preprod');
}
elseif ($_SERVER['SERVER_NAME'] == 'directory.radiopytagor.net'
		|| $_SERVER['SERVER_NAME'] == 'dir.xiph.org')
{
	define('ENVIRONMENT', 'prod');
}
else
{
	define('ENVIRONEMENT', 'test');
}
//define('DEBUG', 'true');

if (ENVIRONMENT != 'prod')
{
	ini_set('error_reporting', E_ALL);
	ini_set('display_errors', 'on');
}

define('MAX_RESULTS_PER_PAGE', 20);
define('MAX_SEARCH_RESULTS', 100);

$begin_time = microtime(true);

// Classes
include_once(dirname(__FILE__).'/prepend.php');
include_once(dirname(__FILE__).'/class.db.php');
include_once(dirname(__FILE__).'/class.izterator.php');
include_once(dirname(__FILE__).'/class.izteratorbuilder.php');
include_once(dirname(__FILE__).'/class.mc.php');
include_once(dirname(__FILE__).'/class.mountpoint.php');
include_once(dirname(__FILE__).'/class.server.php');
include_once(dirname(__FILE__).'/class.tag.php');

// Inclusions
include_once(dirname(__FILE__).'/inc.db.php');
include_once(dirname(__FILE__).'/inc.mc.php');
include_once(dirname(__FILE__).'/inc.templating.php');

// Libs
include_once(dirname(__FILE__).'/lib.uuidgen.php');
include_once(dirname(__FILE__).'/lib.dir.php');

?>
