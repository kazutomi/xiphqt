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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif

/* Private function declarations */
static int ices_xml_parse_file (const char *configfile, ices_config_t *ices_config);
static void ices_xml_parse_setup_defaults ();
static void ices_xml_parse_playlist_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_execution_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_server_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);
static void ices_xml_parse_stream_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config);

/* Global function definitions */
int
ices_xml_parse_config_file (ices_config_t *ices_config, const char *configfile)
{
#ifndef HAVE_LIBXML
	ices_log_error ("XML Parser cannot be run, because LIBXML was not found at compile time.");
	return -1;
#else
	char namespace[1024];

	if (!ices_util_verify_file (configfile)) {
		ices_log_error ("XML Parser Error: Could not open configfile. Error: [%s]", ices_util_strerror (errno, namespace, 1024));
		return 0;
	}

	ices_xml_parse_setup_defaults ();

	return ices_xml_parse_file (configfile, ices_config);
}

/* I hope you can tell this is my first try at xml and libxml :) */
static int
ices_xml_parse_file (const char *configfile, ices_config_t *ices_config)
{
	xmlDocPtr doc;
	xmlNsPtr ns;
	xmlNodePtr cur;

	if (!(doc = xmlParseFile (configfile))) {
		ices_log_error ("XML Parser: Error while parsing %s", configfile);
		return 0;
	}

	if (!(cur = xmlDocGetRootElement (doc))) {
		ices_log_error ("XML Parser: Empty documenent %s", configfile);
		xmlFreeDoc (doc);
		return 0;
	}

	if (!(ns = xmlSearchNsByHref (doc, cur, "http://www.icecast.org/projects/ices"))) {
		ices_log_error ("XML Parser: Document of invalid type, no ices namespace found");
		xmlFreeDoc (doc);
		return 0;
	}

	if (!cur->name || (strcmp (cur->name, "Configuration"))) {
		ices_log_error ("XML Parser: Document of invalid type, root node is not 'Configuration'");
		xmlFreeDoc (doc);
		return 0;
	}

	/* Tree traversal */
	cur = cur->xmlChildrenNode;

	while (cur && xmlIsBlankNode (cur)) {
		cur = cur->next;
	}

	if (cur == NULL) {
		/* Right type of document, but no configuration, guess that is ok */
		xmlFreeDoc (doc);
		return 1;
	}

	while (cur != NULL) {

		if (strcmp (cur->name, "Server") == 0) {
			ices_xml_parse_server_nodes (doc, ns, cur->xmlChildrenNode, ices_config);
		} else if (strcmp (cur->name, "Stream") == 0) {
			ices_xml_parse_stream_nodes (doc, ns, cur->xmlChildrenNode, ices_config);
		} else if (strcmp (cur->name, "Playlist") == 0) {
			ices_xml_parse_playlist_nodes (doc, ns, cur->xmlChildrenNode, ices_config);
		} else if (strcmp (cur->name, "Execution") == 0) {
			ices_xml_parse_execution_nodes (doc, ns, cur->xmlChildrenNode, ices_config);
		} else {
			ices_log ("Unknown Node: %s", cur->name);
		}

		cur = cur->next;

	}
	
	xmlFreeDoc (doc);
	return 1;
}

static void
ices_xml_parse_stream_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Name") == 0) {
			ices_config->name = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Genre") == 0) {
			ices_config->genre = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Description") == 0) {
			ices_config->description = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "URL") == 0) {
			ices_config->url = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Bitrate") == 0) {
			ices_config->bitrate = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Public") == 0) {
			ices_config->ispublic = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else {
			ices_log ("Unknown Stream keyword: %s", cur->name);
		}

		cur = cur->next;
	}
}

static void
ices_xml_parse_server_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Port") == 0) {
			ices_config->port = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Hostname") == 0) {
			ices_config->host = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Mountpoint") == 0) {
			ices_config->mount = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Password") == 0) {
			ices_config->password = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Dumpfile") == 0) {
			ices_config->dumpfile = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Protocol") == 0) {
			char *str = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
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

static void
ices_xml_parse_execution_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Background") == 0) {
			ices_config->daemon = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Verbose") == 0) {
			ices_config->verbose = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Base_Directory") == 0) {
			ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else {
			ices_log ("Unknown Execution keyword: %s", cur->name);
		}

		cur = cur->next;
	}
}

static void
ices_xml_parse_playlist_nodes (xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, ices_config_t *ices_config)
{
	while (cur != NULL) {
		if (strcmp (cur->name, "Randomize") == 0) {
			ices_config->randomize_playlist = atoi (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));
		} else if (strcmp (cur->name, "Type") == 0) {
			char *str = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
			if (str && (strcmp (str, "python") == 0))
				ices_config->playlist_type = ices_playlist_python_e;
			else if (str && (strcmp (str, "perl") == 0))
				ices_config->playlist_type = ices_playlist_perl_e;
			else
				ices_config->playlist_type = ices_playlist_builtin_e;
		} else if (strcmp (cur->name, "File") == 0) {
			ices_config->playlist_file = ices_util_strdup (xmlNodeListGetString (doc, cur->xmlChildrenNode, 1));

		} else {
			ices_log ("Unknown playlist keyword: %s", cur->name);
		}

		cur = cur->next;
	}		

}

static void
ices_xml_parse_setup_defaults ()
{
	xmlKeepBlanksDefault(0);
}
#endif
			








