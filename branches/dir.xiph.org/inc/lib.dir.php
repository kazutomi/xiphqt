<?php

define('CONTENT_TYPE_UNKNOWN', 100);
define('CONTENT_TYPE_OGG_VORBIS', 101);
define('CONTENT_TYPE_OGG_THEORA', 102);
define('CONTENT_TYPE_MP3', 103);
define('CONTENT_TYPE_NSV', 104);
define('CONTENT_TYPE_AAC', 105);
define('CONTENT_TYPE_AACPLUS', 106);

/**
 * Returns the content type ID.
 * 
 * @param string $content_type MIME type to lookup.
 * @param string $server_subtype The subtype (if exists).
 * @return int
 */
function content_type_lookup($content_type)
{
	// Ogg
	if ($content_type == "application/x-ogg" || $content_type == "application/ogg" || $content_type == 'application/ogg+vorbis')
	{
		return CONTENT_TYPE_OGG_VORBIS;
	}
	elseif ($content_type == "application/ogg+theora")
	{
		return CONTENT_TYPE_OGG_THEORA;
	}
	
	// MP3
	elseif ($content_type == "audio/mpeg" || $content_type == "audio/x-mpeg" || $content_type == 'application/mp3')
	{
		return CONTENT_TYPE_MP3;
	}
	
	// NSV
	elseif ($content_type == "video/nsv")
	{
		return CONTENT_TYPE_NSV;
	}
	
	// AAC / AAC+
	elseif ($content_type == "audio/aac")
	{
		return CONTENT_TYPE_AAC;
	}
	elseif ($content_type == "audio/aacp")
	{
		return CONTENT_TYPE_AACPLUS;
	}
	
	// Fallback
	return CONTENT_TYPE_UNKNOWN;
}

/**
 * Inverse of content_type_lookup.
 * 
 * @param int $type_id
 * @return string
 */
function get_media_type_string($type_id)
{
	$type = 'Unknown';
	switch ($type_id)
	{
		case CONTENT_TYPE_OGG_VORBIS:
			$type = 'Ogg Vorbis';
			break;
		case CONTENT_TYPE_OGG_THEORA:
			$type = 'Ogg Theora';
			break;
		case CONTENT_TYPE_MP3:
			$type = 'MP3';
			break;
		case CONTENT_TYPE_NSV:
			$type = 'NSV';
			break;
		case CONTENT_TYPE_AAC:
			$type = 'AAC';
			break;
		case CONTENT_TYPE_AACPLUS:
			$type = 'AAC+';
			break;
	}
	
	return $type;
}

/**
 * Inverse of content_type_lookup.
 * 
 * @param int $type_id
 * @return string
 */
function get_media_type_url($type_id)
{
        $type = 'Unknown';
        switch ($type_id)
        {
                case CONTENT_TYPE_OGG_VORBIS:  
                        $type = 'Ogg_Vorbis';
                        break;
                case CONTENT_TYPE_OGG_THEORA:
                        $type = 'Ogg_Theora';
                        break;
                case CONTENT_TYPE_MP3:
                        $type = 'MP3';
                        break;
                case CONTENT_TYPE_NSV:
                        $type = 'NSV';
                        break;
                case CONTENT_TYPE_AAC:
                        $type = 'AAC';
                        break;
                case CONTENT_TYPE_AACPLUS:
                        $type = 'AAC+';
                        break;
        }

        return $type;
}


/**
 * Inverse of content_type_lookup.
 * 
 * @param int $type_id
 * @param bool $full_type If set to true, will return application/ogg+vorbis as
 *        MIME type for Ogg Vorbis
 * @return string
 */
function get_mime_type_string($type_id, $full_type=false)
{
	$type = 'data';
	switch ($type_id)
	{
		case CONTENT_TYPE_OGG_VORBIS:
			$type = 'application/ogg';
			if ($full_type)
			{
			    $type .= '+vorbis';
			}
			break;
		case CONTENT_TYPE_OGG_THEORA:
			$type = 'application/ogg';
			if ($full_type)
			{
			    $type .= '+theora';
			}
			break;
		case CONTENT_TYPE_MP3:
			$type = 'audio/mpeg';
			break;
		case CONTENT_TYPE_NSV:
			$type = 'video/nsv';
			break;
		case CONTENT_TYPE_AAC:
			$type = 'audio/aac';
			break;
		case CONTENT_TYPE_AACPLUS:
			$type = 'audio/aacp';
			break;
	}
	
	return $type;
}

?>
