<?php

include_once(dirname(__FILE__).'/../lib.dir.php');

/**
 * template_lite capitalize modifier plugin
 *
 * Type:     modifier
 * Name:     get_media_type
 * Purpose:  Return the media type from the media type ID
 */
function smarty_modifier_get_media_type_url($id)
{
	return get_media_type_url($id);
}
?>
