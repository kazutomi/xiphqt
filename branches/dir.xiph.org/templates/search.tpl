{include file="menu_right.tpl"}
				<div id="content">
					<h2>Search results</h2>
{if !empty($results)}
{include file="streams_list.tpl" stream_list=$results display_counter=false}
{elseif $display_tag_cloud && !empty($tag_cloud)}
					<ul class="tag-cloud">
{foreach item=tag from=$tag_cloud}
						<li class="popularity-{$tag.popularity}"><span class="context">{$tag.tag_usage} streams tagged with </span><a href="{$root_url}/by_genre/{$tag.tag_name|capitalize}" class="tag">{$tag.tag_name|capitalize}</a></li>
{/foreach}
					</ul>
{else}
					<p class="warning">Sorry, no result for your search of '{$search_keyword}'.</p>
{/if}
				</div>
