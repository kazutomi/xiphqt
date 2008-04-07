			<div id="copyright">
				<p>Generated in {$generation_time} ms.</p>
				<p>Executed {$sql_queries} SQL queries.</p>
{if !empty($sql_debug)}
				<h4>Queries:</h4>
				<ul>
{foreach item=query from=$sql_debug}
					<li>[{$query.time} ms] {$query.query|truncate}</li>
{/foreach}
				</ul>
{/if}
				<p>Executed {$mc_gets} Memcache gets and {$mc_sets} Memcache sets.</p>
{if !empty($mc_debug)}
				<h4>Queries:</h4>
				<ul>
{foreach item=query from=$mc_debug}
					<li>[{$query.time} ms] [{$query.type|upper}] {if $query.type|upper == 'GET'}[{if $query.hit}HIT{else}MISS{/if}]{/if} {$query.key}</li>
{/foreach}
				</ul>
{/if}
			</div>
		</div>
	</body>
</html>
