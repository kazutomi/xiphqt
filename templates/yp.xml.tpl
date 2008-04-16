<?xml version="1.0" encoding="UTF-8"?>
<directory>
{foreach item=stream from=$streams}
    <entry>
        <server_name>{$stream.stream_name|force_utf8|escape}</server_name>
        <listen_url>{$stream.listen_url|escape}</listen_url>
        <server_type>{$stream.media_type_id|get_mime_type_string}</server_type>
        <bitrate>{$stream.bitrate|escape}</bitrate>
        <channels>{$stream.channels|intval}</channels>
        <samplerate>{$stream.samplerate|intval}</samplerate>
{if !empty($stream.genre)}
        <genre>{$stream.genre|force_utf8|escape}</genre>
{else}
        <genre>various</genre>
{/if}
        <current_song>{$stream.current_song|force_utf8|escape}</current_song>
    </entry>
{/foreach}
</directory>
