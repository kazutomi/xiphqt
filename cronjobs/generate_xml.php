<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

ini_set('memory_limit', '64M');

define('XML_OUTPUT', dirname(__FILE__).'/../yp.xml');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Get data
//$query = "SELECT m.`stream_name`, s.`listen_url`, m.`media_type_id`, m.`bitrate`, m.`channels`, m.`samplerate`, GROUP_CONCAT(t.`tag_name` SEPARATOR ' ') AS `genre`, m.`current_song` FROM `mountpoint` AS m INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` INNER JOIN `mountpoints_tags` AS mt ON mt.`mountpoint_id` = m.`id` INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` GROUP BY s.`id` ORDER BY m.`listeners` DESC LIMIT 1000;";
//$query = "SELECT m.`stream_name`, s.`listen_url`, m.`media_type_id`, m.`bitrate`, m.`channels`, m.`samplerate`, GROUP_CONCAT(t.`tag_name` SEPARATOR ' ') AS `genre`, m.`current_song` FROM `mountpoint` AS m INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` INNER JOIN `mountpoints_tags` AS mt ON mt.`mountpoint_id` = m.`id` INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` GROUP BY s.`id` ORDER BY m.`listeners` DESC;";
$query = "SELECT m.`stream_name`, s.`listen_url`, m.`media_type_id`, m.`bitrate`, m.`channels`, m.`samplerate`, GROUP_CONCAT(t.`tag_name` SEPARATOR ' ') AS `genre`, m.`current_song`  FROM `mountpoint` AS m  INNER JOIN `server`  AS s ON m.`id` = s.`mountpoint_id`  INNER JOIN `mountpoints_tags` AS mt ON mt.`mountpoint_id` = m.`id`  INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` WHERE s.`listeners` >1 OR s.`listen_url` NOT LIKE 'http://%.radionomy.com:80/%' GROUP BY s.`id` ORDER BY m.`listeners` DESC;";
$res = $db->selectQuery($query)->array_data;
shuffle($res);
$tpl->assign_by_ref('streams', $res);
$xml = $tpl->fetch('yp.xml.tpl');

// Write data
$temp_filename = XML_OUTPUT.'.tmp.'. sha1(uniqid(rand(), true));
echo $temp_filename."\n";
$fp = fopen($temp_filename, 'w');
$l = fwrite($fp, $xml);
fclose($fp);
if ($l == strlen($xml))
{
    if (rename($temp_filename, XML_OUTPUT))
    {
        echo "OK.\n";
        exit();
    }
}

echo "KO.\n";

?>
