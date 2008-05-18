<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgLogDBC::getInstance();

function makeFutureDate($format = 'Ymd', $days = 1)
{
    return date($format, mktime(0, 0, 0, date('m'), date('d') + $days, date('Y')));
}
$yesterday = makeFutureDate('Ymd', -1);

/******************************************************************************/
/**                               API LOG                                    **/
/******************************************************************************/
try
{
    $sql = 'CREATE INDEX `logged_at` ON `api_log_%s` (`logged_at`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    $sql = 'CREATE INDEX `message` ON `api_log_%s` (`message`, `logged_at`);'
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
}
catch (SQLException $e)
{
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

$sql_pattern = 'CREATE TABLE IF NOT EXISTS `api_log_%s` LIKE `api_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
    }
    catch (SQLException $e)
    {
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}

?>
