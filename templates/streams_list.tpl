{if $stream_list !== false}
					<table class="servers-list">
{foreach item=stream from=$stream_list key=rowCounter}
						<tr class="row{cycle values="0,1"}">
{if ($display_counter)}
							<td class="rank">#{counter}</td>
{/if}
							<td class="description">
								<p class="stream-name">
									<span class="name">{if $stream->getUrl() != ''}<a href="{$stream->getUrl()|escape}">{/if}{$stream->getStreamName()|escape}{if $stream->getUrl() != ''}</a>{/if}</span>
									<span class="listeners">[{$stream->getListeners()|intval}&nbsp;listener{if (($stream->getListeners()|intval) != 1)}s{/if}]</span>
								</p>
{*if $stream->getIcon()}
								<div class="img"><img src="{$root_url}/images/default_icon.jpg" alt="Purely illustrative image." /></div>
{/if*}
{if $stream->getDescription() != ''}
								<p class="stream-description">{$stream->getDescription()|escape}</p>
{/if}
{if $stream->getCurrentSong() != ''}
								<p class="stream-onair"><strong>On Air:</strong> {$stream->getCurrentSong()|escape}</p>
{/if}
{assign var=tags value=$stream->getTags()}
{if $tags != false}
                                <div class="stream-tags"><strong>Tags:</strong>
                                    <ul class="inline-tags">
{foreach item=tag_name key=tag_id from=$tags}
                                        <li><a href="{$root_url}/by_genre/{$tag_name|capitalize|escape}">{$tag_name|capitalize|escape}</a></li>
{/foreach}
                                    </ul>
                                </div>
{/if}
							</td>
							<td class="tune-in">
							    <p class="format">Tune in:</p>
								<p>[ <a href="{$root_url}/listen/{$stream->getId()}/listen.m3u" title="Listen to '{$stream->getStreamName()|truncate:20:"...":true|escape}'"{* class="tune-in-button"*}>M3U</a> | <a href="{$root_url}/listen/{$stream->getId()}/listen.xspf" title="Listen to '{$stream->getStreamName()|truncate:20:"...":true|escape}'"{* class="tune-in-button"*}>XSPF</a> ]</p>
{if $stream->getMediaTypeId() != ''}
								<p class="format"{if $stream->getBitrate() != ''} title="{if is_numeric($stream->getBitrate())}{$stream->getBitrate()|intval} kbps{else}{$stream->getBitrate()}{/if}"{/if}>
									{$stream->getMediaTypeId()|get_media_type}
									{*if (!empty($stream.channels) && is_numeric($stream.channels))}{if ($stream.channels == 1)}mono{elseif $stream.channels == 2}stereo{else}{$stream.channels} ch{/if}{/if*}
								</p>
{/if}
							</td>
						</tr>
{/foreach}
					</table>
{if !empty($results_pages)}
                                        <ul class="pager">
{if $results_page_no != 1}
						<li><a href="?page={$results_page_no-2}">«</a></li>
{/if}
{foreach item=page from=$results_pages}
                                                <li><a{if $page == $results_page_no} class="active"{/if} href="?page={$page-1}">{$page}</a></li>
{/foreach}
{if $results_page_no != $results_pages_total}
                                                <li><a href="?page={$results_page_no}">»</a></li>
{/if}

                                        </ul>
{/if}
{/if}
