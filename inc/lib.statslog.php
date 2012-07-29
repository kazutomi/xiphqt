<?php

class statsLog
{
    const SEARCH_TYPE_FREEFORM = 1;
    const SEARCH_TYPE_GENRE = 2;
    const SEARCH_TYPE_FORMAT = 3;
    
    public static function playlistAccessed($mountpoint_id, $stream_name)
    {
/*        $db = DirXiphOrgLogDBC::getInstance();

        $ip = utils::getRealIp();
        $ip = $ip !== false ? $ip : '127.0.0.1';

        $db = DirXiphOrgLogDBC::getInstance();
        $sql = "INSERT INTO `playlist_log_%s` (`mountpoint_id`, `stream_name_hash`, `accessed_by`) "
              ."VALUES (%d, '%s', INET_ATON('%s'));";
        $sql = sprintf($sql, date('Ymd'), $mountpoint_id,
                        $db->escape(hash('md5', $stream_name)),
                        $db->escape($ip));
        $db->query($sql);*/
        return true;
    }
    
    public static function keywordsSearched($search_type, $search_keywords)
    {
/*        $db = DirXiphOrgLogDBC::getInstance();
        
        $ip = utils::getRealIp();
        $ip = $ip !== false ? $ip : '127.0.0.1';
        
        $db = DirXiphOrgLogDBC::getInstance();
        $sql = "INSERT INTO `search_log_%s` (`search_keywords`, `search_type`, `searched_by`) "
              ."VALUES ('%s', %d, INET_ATON('%s'));";
        $sql = sprintf($sql, date('Ymd'), $db->escape($search_keywords),
                        intval($search_type), $db->escape($ip));
        $db->query($sql);*/
        return true;
    }
}

?>
