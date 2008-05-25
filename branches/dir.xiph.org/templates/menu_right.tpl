				<div id="navbar">
					<div id="sidebar-statistics">
						<h3>Statistics</h3>
						<ul>
{if !empty($servers_total)}
							<li>Total streams: <strong>{$servers_total|intval}</strong></li>
{/if}
{if !empty($servers_vorbis)}
							<li>Ogg Vorbis: <strong>{$servers_vorbis|intval}</strong></li>
{/if}
{if !empty($servers_mp3)}
							<li>MP3: <strong>{$servers_mp3|intval}</strong></li>
{/if}
						</ul>
					</div>
					<div id="sidebar-search">
						<h3>Search</h3>
						<ul>
							<li><form method="get" action="{$root_url}/search" id="search-box">
								<div><label for="search">Search:</label> <input type="text" name="search" id="search" maxlength="20"{if !empty($search_keyword)} value="{$search_keyword}"{/if} />
								<input type="submit" value="OK" class="button" /></div>
							</form></li>
						</ul>
						<h3>By genre</h3>
						<div id="search-genre">
{if !empty($tag_cloud)}
							<ul class="tag-cloud">
{foreach item=tag from=$tag_cloud}
								<li class="popularity-{$tag.popularity}"><span class="context">{$tag.tag_usage} <a href="http://dir.xiph.org/">radio</a> <a href="http://dir.xiph.org/">streams</a> tagged with </span><a href="{$root_url}/by_genre/{$tag.tag_name|capitalize:true|escape}" class="tag" title="{$tag.tag_name|capitalize:true|escape} radios">{$tag.tag_name|capitalize:true|escape}</a></li>
{/foreach}
							</ul>
{else}
							<ul class="tag-cloud">
								<li><a href="{$root_url}/by_genre/Alternative" class="tag">Alternative</a></li>
								<li><a href="{$root_url}/by_genre/Classical" class="tag">Classical</a></li>
								<li><a href="{$root_url}/by_genre/Comedy" class="tag">Comedy</a></li>
								<li><a href="{$root_url}/by_genre/Country" class="tag">Country</a></li>
								<li><a href="{$root_url}/by_genre/Dance" class="tag">Dance</a></li>
								<li><a href="{$root_url}/by_genre/Funk" class="tag">Funk</a></li>
								<li><a href="{$root_url}/by_genre/Jazz" class="tag">Jazz</a></li>
								<li><a href="{$root_url}/by_genre/Metal" class="tag">Metal</a></li>
								<li><a href="{$root_url}/by_genre/Mixed" class="tag">Mixed</a></li>
								<li><a href="{$root_url}/by_genre/Pop" class="tag">Pop</a></li>
								<li><a href="{$root_url}/by_genre/Rap" class="tag">Rap</a></li>
								<li><a href="{$root_url}/by_genre/RnB" class="tag">RnB</a></li>
								<li><a href="{$root_url}/by_genre/Rock" class="tag">Rock</a></li>
								<li><a href="{$root_url}/by_genre/Talk" class="tag">Talk</a></li>
								<li><a href="{$root_url}/by_genre/Techno" class="tag">Techno</a></li>
								<li><a href="{$root_url}/by_genre/80s" class="tag">80s</a></li>
								<li><a href="{$root_url}/by_genre/70s" class="tag">70s</a></li>
								<li class="plus"><a href="{$root_url}/by_genre">more genres &raquo;</a></li>
							</ul>
{/if}
						</div>
						<h3>By format</h3>
						<div id="search-format">
							<ul>
								<li><a href="{$root_url}/by_format/Ogg_Vorbis">Ogg Vorbis</a></li>
								<li><a href="{$root_url}/by_format/MP3">MP3</a></li>
								<li><a href="{$root_url}/by_format/AAC">AAC</a></li>
								<li><a href="{$root_url}/by_format/AAC+">AAC+</a></li>
								<li><a href="{$root_url}/by_format/Ogg_Theora">Ogg Theora</a></li>
								<li><a href="{$root_url}/by_format/NSV">NSV</a></li>
							</ul>
						</div>
					</div>
				</div>
