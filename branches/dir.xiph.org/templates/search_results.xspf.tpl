<?xml version="1.0" encoding="UTF-8"?>
<playlist version="1" xmlns="http://xspf.org/ns/0/">
{if !empty($xspf_title)}
  <title>{$xspf_title|escape}</title>
{/if}
  <creator>dir.xiph.org</creator>
  <info>http://dir.xiph.org/</info>
{if $results !== false}
  <trackList>
{foreach from=$results item=stream}
    <track>
      <title>{$stream->getStreamName()|escape}</title>
{if $stream->getUrl() != ''}
      <info>{if substr($stream->getUrl(), 0, 4) == 'http'}{$stream->getUrl()|escape}{else}http://{$stream->getUrl()|escape}{/if}</info>
{/if}
      <meta rel="http://dir.xiph.org/xpsf/listeners">{$stream->getListeners()|intval}</meta>
{if $stream->getDescription() != ''}
      <annotation>{$stream->getDescription()|escape}</annotation>
{/if}
{if $stream->getCurrentSong() != ''}
      <meta rel="http://dir.xiph.org/xspf/currentSong">{$stream->getCurrentSong()|escape}</meta>
{/if}
{assign var=tags value=$stream->getTags()}
{if $tags != false}
{foreach item=tag_name key=tag_id from=$tags}
      <meta rel="http://dir.xiph.org/xspf/tag">{$tag_name|capitalize:true|escape}</meta>
{/foreach}
{/if}
      <location>http://dir.xiph.org/listen/{$stream->getId()|intval}/listen.xspf</location>
    </track>
{/foreach}
  </trackList>
{/if}
</playlist>
