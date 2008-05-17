<?php

class statsLog
{
    public static function playlistAccessed($mountpoint_id, $stream_name)
    {
        $db = DirXiphOrgLogDBC::getInstance();

        $ip = utils::getRealIp();
        $ip = $ip !== false ? $ip : '127.0.0.1';

        $db = DirXiphOrgLogDBC::getInstance();
        $sql = "INSERT INTO `playlist_log_%s` (`mountpoint_id`, `stream_name_hash`, `accessed_by`) "
              ."VALUES (%d, '%s', INET_ATON('%s'));";
        $sql = sprintf($sql, date('Ymd'), $mountpoint_id,
                        $db->escape(hash('md5', $stream_name)),
                        $db->escape($ip));
        $db->query($sql);
    }
}

?>
