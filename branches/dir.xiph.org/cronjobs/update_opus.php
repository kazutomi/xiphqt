<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();

// Quick hack to update all mountpoints ending in opus to media_type_id for Opus instead of generic Ogg
$query = "UPDATE mountpoint SET media_type_id = 107 WHERE id IN (SELECT mountpoint_id FROM server WHERE listen_url LIKE '%opus');";
$db->noReturnQuery($query);


?>
