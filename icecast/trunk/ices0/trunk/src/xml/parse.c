/* parse.c
 * - Functions for xml file parsing
 * Copyright (c) 2000 Alexander Haväng
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "definitions.h"

#ifdef HAVE_LIBXML
# ifdef HAVE_LIBXML_PARSER_H
#  include <libxml/xmlmemory.h>
#  include <libxml/parser.h>
# elif HAVE_GNOME_XML_PARSER_H
#  include <gnome-xml/parser.h>
# else
/* Probably 1.8.7 backwards crap */
#  include <parser.h>
#  include <xmlmemory.h>
#endif

/* Private function declarations */
static int ices_xml_parse_file (const char *configfile, ices_config_t *ices_config);
static void ices_xml_parse_setup_defaults ();
static void ices_xml_parse_playlist_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_execution_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_server_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_stream_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static xmlNodePtr ices_xml_get_root_element (xmlDocPtr doc);
static xmlNodePtr ices_xml_get_children_node (xmlNodePtr cur);
#endif

/* Global function definitions */

/* Top level XML configfile parser */
int
ices_xml_parse_config_file (ices_config_t *ices_config, const char *configfile)
{
#ifndef HAVE_LIBXML
	ices_log_error ("XML Parser cannot be run, because LIBXML was not found at compile time.");
	return -1;
#else

	char namespace[1024];
	
/* Wonder what idjit came up with this...  */
#ifdef LIBXML_TEST_VERSION
	LIBXML_TEST_VERSION
#endif
		if (!ices_util_verify_file (configfile)) {
			ices_log_error ("XML Parser Error: Could not open configfile. [%s]  Error: [%s]", configfile, ices_util_strerror (errno, namespace, 1024));
			return 0;
		}
	
	/* Setup the parser options */
	ices_xml_parse_setup_defaults ();
	
	/* Parse the file and be happy */
	return ices_xml_parse_file (configfile, ices_config);
#endif
}

#ifdef HAVE_LIBXML

/* I hope you can tell this is my first try at xml and libxml :)  */
static int
ices_xml_parse_file (const char *configfile, ices_config_t *ices_config)
{
	xmlDocPtr doc;
	xmlNsPtr ns;
	xmlNodePtr cur;

	/* This does the actual parsing */
	if (!(doc = xmlParseFile (configfile))) {
		ices_log_error ("XML Parser: Error while parsing %s", configfile);
		return 0;
	}
	
	/* Gimme the root element dammit! */
	if (!(cur = ices_xml_get_root_element (doc))) {
		ices_log_error ("XML Parser: Empty documenent %s", configfile);
		xmlFreeDoc (doc);
		return 0;
	}
	
	/* Verify that the document is of the right type in the right namespace */
	if (!(ns = xmlSearchNsByHref (doc, cur, (unsigned char *)"http://www.icecast.org/projects/ices"))) {
		ices_log_error ("XML Parser: Document of invalid type, no ices namespace found");
		xmlFreeDoc (doc);
		return 0;
	}

	/* First element should be configuration */
	if (!cur->name || (strcmp (cur->name, "Configuration"))) {
		ices_log_error ("XML Parser: Document of invalid type, root node is not 'Configuration'");
		xmlFreeDoc (doc);
		return 0;
	}

	/* Get the configuration tree */
	/* Tree traversal */
	cur = ices_xml_get_children_node (cur);

/* If we have libxml/parser.h, it's libxml 2.x */
#ifdef HAVE_LIBXML_PARSER_H
	while (cur && xmlIsBlankNode (cur)) {
		cur = cur->next;
	}
#endif

	if (cur == NULL) {
		/* Right type of document, but no configuration, guess that is ok */
		xmlFreeDoc (doc);
		return 1;
	}

	/* Separate the parsing into different submodules,
	 * Server, Stream, Playlist and Execution */
	while (cur != NULL) {

		if (strcmp (cur->name, "Server") == 0) {
			ices_xml_parse_server_nodes (doc, ns, ices_xml_get_children_node (cur), ices_config);
		} else if (strcmp (cur->name, "Stream") == 0) {
			ices_xml_parse_stream_nodes (doc, ns, ices_xml_get_children_node (cur), ices_config);
		} else if (strcmp (cur->name, "Playlist") == 0) {
			ices_xml_parse_playlist_nodes (doc, ns, ices_xml_get_children_node (cur), ices_config);
		} else if (strcmp (cur->name, "Execution") == 0) {
			ices_xml_parse_execution_nodes (doc, ns, ices_xml_get_children_node (cur), ices_config);
		} else {
			ices_log ("Unknown Node: %s", cur->name);
		}

		cur = cur->next;

	}
	
	/* Be a good boy and cleanup */
	xmlFreeDoc (doc);
	return 1;
}

/* Parse the stream specific configuration */
static void
ices_xml_parse_stream_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Name") == 0) {

			if (ices_config->name)
				ices_util_free (ices_config->name);

			ices_config->name = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Genre") == 0) {

			if (ices_config->genre)
				ices_util_free (ices_config->genre);

			ices_config->genre = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Description") == 0) {

			if (ices_config->description)
				ices_util_free (ices_config->description);

			ices_config->description = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "URL") == 0) {

			if (ices_config->url)
				ices_util_free (ices_config->url);

			ices_config->url = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Bitrate") == 0) {

			ices_config->bitrate = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Public") == 0) {

			ices_config->ispublic = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else {
			ices_log ("Unknown Stream keyword: %s", cur->name);
		}

		cur = cur->next;
	}
}

/* Parse the server specific configuration */
static void
ices_xml_parse_server_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Port") == 0) {

			ices_config->port = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Hostname") == 0) {

			if (ices_config->host)
				ices_util_free (ices_config->host);

			ices_config->host = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Mountpoint") == 0) {
			
			if (ices_config->mount)
				ices_util_free (ices_config->mount);

			ices_config->mount = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Password") == 0) {
			
			if (ices_config->password)
				ices_util_free (ices_config->password);

			ices_config->password = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Dumpfile") == 0) {
			
			if (ices_config->dumpfile)
				ices_util_free (ices_config->dumpfile);

			ices_config->dumpfile = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Protocol") == 0) {

			unsigned char *str = xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1);

			if (str && (str[0] == 'i' || str[0] == 'I'))
				ices_config->header_protocol = icy_header_protocol_e;
			else
				ices_config->header_protocol = xaudiocast_header_protocol_e;

		} else {
			ices_log ("Unknown Server keyword: %s", cur->name);
		}
		
		cur = cur->next;
	}
}

/* Parse the execution specific options */
static void
ices_xml_parse_execution_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Background") == 0) {

			ices_config->daemon = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Verbose") == 0) {
			
			ices_config->verbose = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Samplerate") == 0) {
			ices_config->out_samplerate = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));
		} else if (strcmp (cur->name, "Channels") == 0) {
			ices_config->out_numchannels = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));
		} else if (strcmp (cur->name, "Base_Directory") == 0) {
			
			if (ices_config->base_directory)
				ices_util_free (ices_config->base_directory);
			
			ices_config->base_directory = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));
			
		} else if (strcmp (cur->name, "Reencode") == 0) {

			int res = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

#ifndef HAVE_LIBLAME
			if (res == 1) {
				ices_log ("Support for reencoding with liblame was not found. You can't reencode this.");
				ices_setup_shutdown ();
			}
#endif
			
			ices_config->reencode = res;

		} else {

			ices_log ("Unknown Execution keyword: %s", cur->name);

		}

		cur = cur->next;
	}
}

/* Parse the playlist specific configuration */
static void
ices_xml_parse_playlist_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Randomize") == 0) {

			ices_config->randomize_playlist = atoi ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Type") == 0) {
			unsigned char *str = xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1);

			if (str && (strcmp (str, "python") == 0))
				ices_config->playlist_type = ices_playlist_python_e;
			else if (str && (strcmp (str, "perl") == 0))
				ices_config->playlist_type = ices_playlist_perl_e;
			else
				ices_config->playlist_type = ices_playlist_builtin_e;
			
		} else if (strcmp (cur->name, "File") == 0) {

			if (ices_config->playlist_file)
				ices_util_free (ices_config->playlist_file);

			ices_config->playlist_file = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else if (strcmp (cur->name, "Module") == 0) {

			if (ices_config->interpreter_file)
				ices_util_free (ices_config->interpreter_file);

			ices_config->interpreter_file = ices_util_strdup ((char *)xmlNodeListGetString (doc, ices_xml_get_children_node(cur), 1));

		} else {

			ices_log ("Unknown playlist keyword: %s", cur->name);

		}

		cur = cur->next;
	}		

}

/* Wrapper function to get xml children, to work with old libxml */
static xmlNodePtr
ices_xml_get_children_node (xmlNodePtr cur)
{
	/* libxml version 2*/
#ifdef HAVE_LIBXML_PARSER_H
	return cur->xmlChildrenNode;
#else
	return cur->childs;
#endif
}

/* Wrapper function to get xml root element, to work with old libxml */
static xmlNodePtr
ices_xml_get_root_element (xmlDocPtr doc)
{
#ifdef HAVE_LIBXML_PARSER_H
	return xmlDocGetRootElement (doc);
#else
	return doc->root;
#endif
}

/* Setup some libxml parser options */
static void
ices_xml_parse_setup_defaults ()
{
#ifdef HAVE_LIBXML_PARSER_H
	xmlKeepBlanksDefault(0);
#endif
}
#endif
