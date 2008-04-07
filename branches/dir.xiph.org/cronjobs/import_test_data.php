<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

define('XML_OUTPUT', dirname(__FILE__).'/../yp.xml');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Get data
$query = "SELECT m.`stream_name`, s.`listen_url`, m.`media_type_id`, m.`bitrate`, m.`channels`, m.`samplerate`, GROUP_CONCAT(t.`tag_name` SEPARATOR ' ') AS `genre`, m.`current_song` FROM `mountpoint` AS m INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` INNER JOIN `mountpoints_tags` AS mt ON mt.`mountpoint_id` = m.`id` INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` GROUP BY s.`id` ORDER BY NULL;";
$res = $db->selectQuery($query)->array_data;
$tpl->assign_by_ref('streams', $res);
$xml = $tpl->fetch('yp.xml.tpl');

// Write data
$fp = fopen(XML_OUTPUT.'.tmp', 'w');
$l = fwrite($fp, $xml);
fclose($fp);
if ($l == strlen($xml))
{
    if (rename(XML_OUTPUT.'.tmp', XML_OUTPUT))
    {
        echo "OK.\n";
        exit();
    }
}

echo "KO.\n";

?>
