<?php

define('ERROR_RECIPIENT', 'annie.dupont1137@gmail.com');
define('ERROR_SENDER', 'bug');

function mail_error($mail_to, $mail_from,
                    $errstr, $errfile, $errline, $trace='')
{
    $data = <<<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
    <head>
        <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
        <title>Erreur</title>
        <style type="text/css">

        </style>
    </head>
    <body>\n
EOF;
    $data .= sprintf("        <h1>Error: %s</h1>
        <h2><code>%s</code>, <code>line %d</code></h2>\n",
                     $errstr, $errfile, $errline);
    $data .= sprintf("        <hr />
        <dl>
            <dt>Date:</dt>
            <dd>%s</dd>
            <dt>Remote address:</dt>
            <dd>%s</dd>
            <dt>Path:</dt>
            <dd><code>%s</code></dd>
        </dl>
        <hr />
        <h3>Trace</h3>
        <pre>%s</pre>
        <hr />
        <h3>\$_SERVER</h3>
        <pre>%s</pre>
        <h3>\$_REQUEST</h3>
        <pre>%s</pre>\n",
                     gmdate('r'),
                     array_key_exists('REMOTE_ADDR', $_SERVER)
                        ? $_SERVER['REMOTE_ADDR'] : '???',
                     array_key_exists('REQUEST_PATH', $_SERVER)
                        ? $_SERVER['REQUEST_PATH'] : '???',
                     print_r($trace, true),
                     print_r($_SERVER, true),
                     print_r($_REQUEST, true));
    $data .= <<<EOF
    </body>
</html>
EOF;
    
    $env = defined('ENVIRONMENT') ? ' '.ENVIRONMENT : '';
    @mail($mail_to, '[DXO'.$env.'] Error: '.$errstr, $data,
          'From: <' . $mail_from . '>' . "\n" .
          'Content-Type: text/html; charset=iso-8859-1' . "\n");
}

function custom_exception_handler($e)
{
    $msg = $e->getMessage();
    $msg = empty($msg) ? 'Exception '.get_class($e) : $msg;
    
    mail_error(ERROR_RECIPIENT, ERROR_SENDER,
               $msg, $e->getFile(), $e->getLine(), $e->getTrace());
}

function error_handler_exception_callback($errno, $errstr, $errfile='Unknown', $errline=0, $errcontext=null)
{
    // @-ignored error
    if (($errno & error_reporting()) === 0)
    {
        return;
    }

    // NOTICE
    if ($errno == E_NOTICE || $errno == E_USER_NOTICE)
    {
        return;
    }

    // STRICT
    if ($errno == E_STRICT)
    {
        return;
    }

    $e = new DXOException($errstr, $errno);
    $e->setErrorFile($errfile);
    $e->setErrorLine($errline);
    
    throw $e;
}

set_exception_handler('custom_exception_handler');
set_error_handler('error_handler_exception_callback');

?>
