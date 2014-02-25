<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgDBC::getInstance();


// Total
$query = "SELECT COUNT(*) AS `count` FROM `server` WHERE `checked` = 0";
$total = $db->singleQuery($query)->current('count');
printf("total.value %s\n", $total);

// radionomy
$query = 'SELECT COUNT(*) AS `count` FROM `server` WHERE `checked` = 0 AND `listen_url` LIKE "%radionomy%" ';
$radionomy = $db->singleQuery($query)->current('count');
printf("radionomy.value %s\n", $radionomy);

// radionomy slow hosts
$query = 'SELECT COUNT(*) AS `count` FROM `server` WHERE `checked` = 0 AND (`listen_url` LIKE "%streaming205%" OR `listen_url` LIKE "%streaming207%" )';
$radionomys = $db->singleQuery($query)->current('count');
printf("radionomyslow.value %s\n", $radionomys);


// Total
$query = 'SELECT COUNT(*) AS `count` FROM `server` WHERE `checked` = 0 AND `listen_url` NOT LIKE "%radionomy.com%"';
$totalmr = $db->singleQuery($query)->current('count');
printf("totalmr.value %s\n", $totalmr);

?>
