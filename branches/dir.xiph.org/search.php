<?php

include_once(dirname(__FILE__).'/inc/prepend.php');

// Memcache connection
$memcache = DirXiphOrgMCC::getInstance();

// Get the args
$page_n = array_key_exists('page', $_GET) ? intval($_GET['page']) : 0;
if ($page_n > (MAX_SEARCH_RESULTS / MAX_RESULTS_PER_PAGE))
{
    $page_n = 0;
}
$keyword = array_key_exists('search', $_GET) ? $_GET['search'] : false;
if ($keyword !== false)
{
	// Clean the input string
	// We can't do further cleaning on things like '-', '=', '+',
	// because regexps are not made for matching things like
	// '品情報、映画音楽 ゲーム金融'. Too bad.
	$keyword = trim(strip_tags($keyword));
	$keyword = substr($keyword, 0, 20);
	$tpl->assign('search_keyword', $keyword);
	$keyword = preg_replace('/\s+/', ' ', $keyword);
	
	// Database connection
	$db = DirXiphOrgDBC::getInstance();
	
	// Now generate the search string that will be used in the LIKE
	// clause. No more than 3 words, unless we want to screw up the
	// MySQL server big time by a) overloading it with genuine queries
	// b) opening a San Andreas' security breach for DOS.
	$keywords = explode(' ', $keyword);
	$search_string = '%';
	$search_in = array();
	$count = 0;
	foreach ($keywords as $k)
	{
		if (strlen($k) < 3)
		{
			continue;
		}
		if (strlen($k) > 10)
		{
			$k = substr($k, 0, 10);
		}
		
		$search_string .= $k.'%';
		$search_in[] = '"'.$db->escape($k).'"';
		
		if (++$count >= 3)
		{
			break;
		}
	}
    if ($count > 0)
    {
        $search_string_hash = jenkins_hash_hex($search_string);
        $search_in = implode(', ', $search_in);

        // Get the data from the Memcache server
        if (($results = $memcache->get(ENVIRONMENT.'_search_'.$search_string_hash)) === false)
        {
			// Cache miss. Now query the database.
			try
			{
//                $query = 'SELECT * FROM `mountpoint` WHERE `stream_name` LIKE "%1$s" OR `description` LIKE "%1$s" OR `id` IN (SELECT `mountpoint_id` FROM `mountpoints_tags` AS mt INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` WHERE `tag_name` IN (%2$s)) ORDER BY `listeners` DESC LIMIT %3$d;';
                 $query = 'SELECT m.* FROM '
                                .'(SELECT * FROM `mountpoint` '
                                .'WHERE `stream_name` LIKE "%1$s" OR `description` LIKE "%1$s" '
                                .'OR `id` IN '
                                        .'(SELECT `mountpoint_id` FROM `mountpoints_tags` AS mt '
                                        .'INNER JOIN `tag` AS t ON mt.`tag_id` = t.`id` '
                                        .'WHERE `tag_name` IN (%2$s)) '
                                .'ORDER BY `listeners` DESC) AS m '
                         .'INNER JOIN `server` AS s ON m.`id` = s.`mountpoint_id` ' 
                         .'GROUP BY m.`id` ORDER BY NULL';
                $query = sprintf($query,
                                 mysql_real_escape_string($search_string),
                                 $search_in,
                                 MAX_SEARCH_RESULTS);
                $results = $db->selectQuery($query);
                $res = array();
                while (!$results->endOf())
                {
                    $mp = Mountpoint::retrieveByPk($results->current('id'));
                    if ($mp instanceOf Mountpoint)
                    {
                        $res[] = $mp;
                    }
                    $results->next();
                }
                $results = $res;
			}
            catch (SQLNoResultException $e)
            {
                $results = array();
            }

            // Cache the resultset
            $memcache->set(ENVIRONMENT.'_search_'.$search_string_hash, $results,
                            false, 60);
        }
        
	    // Logging
	    try
	    {
	        statsLog::keywordsSearched(statsLog::SEARCH_TYPE_FREEFORM, $search_string);
	    }
	    catch (SQLException $e)
	    {
	        // Do nothing, it's just logging after all...
	    }
	    
        // Now assign the results to a template var
        if ($results !== false && $results !== array())
        {
            $n_results = count($results);
            $results_pages = ceil($n_results / MAX_RESULTS_PER_PAGE);
            if ($page_n > $results_pages)
            {
                $page_n = 0;
            }
            $pages = array();
            if ($results_pages < PAGES_IN_PAGER)
            {
                $pages = range(1, $results_pages);
            }
            elseif ($page_n > PAGES_IN_PAGER)
            {
                $pages = range($page_n + 1 - PAGES_IN_PAGER, $pages_n + 1 + PAGES_IN_PAGER);
            }
            elseif ($page_n + PAGES_IN_PAGER > $results_pages)
            {
                $pages = range($results_pages - PAGES_IN_PAGER + 1, $results_pages);
            }
            else
            {
                $pages = range(1, PAGES_IN_PAGER);
            }
            $offset = $page_n * MAX_RESULTS_PER_PAGE;
            $results = array_slice($results, $offset,
                                   MAX_RESULTS_PER_PAGE);
            $tpl->assign_by_ref('results', $results);
            $tpl->assign_by_ref('results_pages', $pages);
            $tpl->assign_by_ref('results_pages_total', $results_pages);
            $tpl->assign('results_page_no', $page_n + 1);
        }
	}
}

// Header
$tpl->display("head.tpl");

// Display the results
$tpl->assign('servers_total', $memcache->get(ENVIRONMENT.'_servers_total'));
$tpl->assign('servers_mp3', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_MP3));
$tpl->assign('servers_vorbis', $memcache->get(ENVIRONMENT.'_servers_'.CONTENT_TYPE_OGG_VORBIS));
$tpl->assign('tag_cloud', genfile::get(genfile::makeGenfileName('tagcloud')));
$tpl->display('search.tpl');

// Footer
if (ENVIRONMENT != 'prod')
{
	$tpl->assign('generation_time', (microtime(true) - $begin_time) * 1000);
	$tpl->assign('sql_queries', isset($db) ? $db->queries : 0);
	if (isset($db))
	{
		$tpl->assign('sql_debug', $db->log);	
	}
	$tpl->assign('mc_gets', isset($memcache) ? $memcache->gets : 0);
	$tpl->assign('mc_sets', isset($memcache) ? $memcache->sets : 0);
	if (isset($memcache))
	{
		$tpl->assign('mc_debug', $memcache->log);
	}
}
$tpl->assign("google_tag", "/search/freeform");
$tpl->display('foot.tpl');

?>
