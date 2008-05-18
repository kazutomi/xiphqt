<?php

class DXOException extends Exception
{
    public function setErrorLine($el)
    {
        $this->line = $el;
    }
    
    public function setErrorFile($ef)
    {
        $this->file = $ef;
    }
}

class BadIDException extends DXOException { }
class EnvironmentUndefinedException extends DXOException { }

class APIException extends DXOException { }
class ServerRefusedAPIException extends APIException
{
    protected $listen_url = false;
    
    public function __construct($errstr, $errno, $listen_url = null)
    {
        if ($listen_url !== null)
        {
            $this->listen_url = $listen_url;
        }
        
        parent::__construct($errstr, $errno);
    }
    
    public function getListenUrl()
    {
        return $this->listen_url;
    }
    
    public function setListenUrl($url)
    {
        $this->listen_url = $url;
    }
}
class NoSuchSIDAPIException extends APIException { }

if (getenv('ENVIRONMENT') !== false)
{
	define('ENVIRONMENT', strtolower(getenv('ENVIRONMENT')));
}
elseif (array_key_exists('SERVER_NAME', $_SERVER)
        && $_SERVER['SERVER_NAME'] == 'directory-test.radiopytagor.net')
{
	define('ENVIRONMENT', 'preprod');
}
elseif (array_key_exists('SERVER_NAME', $_SERVER)
        && ($_SERVER['SERVER_NAME'] == 'directory.radiopytagor.net'
       		|| $_SERVER['SERVER_NAME'] == 'dir.xiph.org'))
{
	define('ENVIRONMENT', 'prod');
}
else
{
	define('ENVIRONMENT', 'test');
}
//define('DEBUG', 'true');

if (ENVIRONMENT != 'prod')
{
	ini_set('error_reporting', E_ALL);
	ini_set('display_errors', 'on');
}

define('MAX_RESULTS_PER_PAGE', 20);
define('MAX_SEARCH_RESULTS', 100);
define('PAGES_IN_PAGER', 5);

$begin_time = microtime(true);

include_once(dirname(__FILE__).'/lib.errors.php');

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
include_once(dirname(__FILE__).'/lib.genfile.php');
include_once(dirname(__FILE__).'/lib.uuidgen.php');
include_once(dirname(__FILE__).'/lib.dir.php');
include_once(dirname(__FILE__).'/lib.utils.php');
include_once(dirname(__FILE__).'/lib.apilog.php');
include_once(dirname(__FILE__).'/lib.statslog.php');

?>
