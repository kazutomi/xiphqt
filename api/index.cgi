#!/bin/bash
echo Content-Type: application/json
#echo Content-Type: text/html
echo
QUERY="SELECT json_array(stream_name, description, url, listeners, media_type_id, bitrate) AS '' FROM mountpoint WHERE media_type_id = 107 ; "
QUERY="SELECT json_object(m.stream_name AS stream_name, s.listen_url AS listen_url, m.media_type_id AS media_type_id, m.bitrate AS bitrate, m.channels as channels, m.samplerate as samplerate, GROUP_CONCAT(t.tag_name SEPARATOR ' ') AS genre, m.current_song as current_song) AS ''  FROM mountpoint AS m  INNER JOIN server  AS s ON m.id = s.mountpoint_id  INNER JOIN mountpoints_tags AS mt ON mt.mountpoint_id = m.id  INNER JOIN tag AS t ON mt.tag_id = t.id WHERE s.listeners >1 OR s.listen_url NOT LIKE 'http://%.radionomy.com:80/%' GROUP BY s.id ORDER BY m.listeners DESC ;"
echo $QUERY| mysql -udir_xiph_org -p5wCjLEVmAJnmM dir_xiph_org
