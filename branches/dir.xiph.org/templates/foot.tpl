			<div id="copyright">
{if $smarty.const.ENVIRONMENT != 'prod'}
				<p>Generated in {$generation_time} ms.</p>
				<p>Executed {$sql_queries} SQL queries.</p>
{if !empty($sql_debug)}
				<h4>Queries:</h4>
				<ul>
{foreach item=query from=$sql_debug}
					<li>[{$query.time} ms] {$query.query}</li>
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
{/if}
			</div>
		</div>
<!-- GAU -->
<script type="text/javascript">
var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
</script>
<script type="text/javascript">
var pageTracker = _gat._getTracker("UA-4486419-1");
pageTracker._initData();
{if isset($google_tag)}
pageTracker._setVar("{$google_tag}");
{/if}
pageTracker._trackPageview(); 
</script>
<!-- /GAU -->
	</body>
</html>
