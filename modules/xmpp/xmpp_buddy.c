#include "xmpp_buddy.h"
#include "xmpp_iq.h"

static GHashTable *xmpp_buddies = NULL;

/**
 * Scribe the presence information of the roster by
 * sending a <presence/> label to the server.
 */
static void
xmpp_buddy_presence(XmppStream *stream)
{
	xmlnode *node;
	gchar *xml_string;

	g_return_if_fail(stream != NULL);

	node = xmlnode_create("presence");

	xml_string = xmlnode_to_string(node);
	
	hybrid_debug_info("xmpp", "subscribe presence,send:\n%s", xml_string);

	if (hybrid_ssl_write(stream->ssl, xml_string, strlen(xml_string)) == -1) {

		hybrid_account_error_reason(stream->account->account,
				"subscribe presence failed");
		g_free(xml_string);

		return;
	}

	g_free(xml_string);
}

void
xmpp_buddy_process_roster(XmppStream *stream, xmlnode *root)
{
	gchar *value;
	gchar *jid;
	gchar *name;
	gchar *group_name;
	xmlnode *node;
	xmlnode *group_node;
	xmlnode *item_nodes;
	HybridGroup *group;
	HybridAccount *account;
	HybridBuddy *hd;
	XmppBuddy *buddy;

	g_return_if_fail(stream != NULL);
	g_return_if_fail(root != NULL);

	account = stream->account->account;

	hybrid_account_set_connection_status(account, 
					HYBRID_CONNECTION_CONNECTED);

	if (!xmlnode_has_prop(root, "type")) {
		goto roster_err;
	}

	value = xmlnode_prop(root, "type");
	if (g_strcmp0(value, "result") != 0) {
		goto roster_err;
	}
	g_free(value);

	if (!(node = xmlnode_find(root, "query"))) {
		goto roster_err;
	}

	item_nodes = xmlnode_child(node);

	for (node = item_nodes; node; node = node->next) {

		jid = xmlnode_prop(node, "jid");
		name = xmlnode_prop(node, "name");

		if (!(group_node = xmlnode_find(node, "group"))) {
			group_name = g_strdup(_("Buddies"));

		} else {
			group_name = xmlnode_content(group_node);
		}

		/* add this buddy's group to the buddy list. */
		group = hybrid_blist_add_group(account, group_name, group_name);

		/* add this buddy to the buddy list. */
		hd = hybrid_blist_add_buddy(account, group, jid, name);

		buddy = xmpp_buddy_create(hd);
		xmpp_buddy_set_name(buddy, name);
		xmpp_buddy_set_group(buddy, group_name);

		if (!hybrid_blist_get_buddy_checksum(hd)) {
			xmpp_buddy_get_info(stream, buddy);
		}

		g_free(group_name);
		g_free(jid);
		g_free(name);
	}

	/* subsribe the presence of the roster. */
	xmpp_buddy_presence(stream);

	return;

roster_err:
	hybrid_account_error_reason(stream->account->account,
			_("request roster failed."));
}

XmppBuddy*
xmpp_buddy_create(HybridBuddy *hybrid_buddy)
{
	XmppBuddy *buddy;

	g_return_val_if_fail(hybrid_buddy != NULL, NULL);

	if (!xmpp_buddies) {
		xmpp_buddies = g_hash_table_new(g_str_hash, g_str_equal);
	}

	if ((buddy = g_hash_table_lookup(xmpp_buddies, hybrid_buddy->id))) {
		return buddy;
	}

	buddy = g_new0(XmppBuddy, 1);
	buddy->jid = g_strdup(hybrid_buddy->id);
	buddy->buddy = hybrid_buddy;

	g_hash_table_insert(xmpp_buddies, buddy->jid, buddy);

	return buddy;
}

void
xmpp_buddy_set_name(XmppBuddy *buddy, const gchar *name)
{
	g_return_if_fail(buddy != NULL);

	g_free(buddy->name);
	buddy->name = g_strdup(name);
}

void
xmpp_buddy_set_status(XmppBuddy *buddy, const gchar *status)
{
	g_return_if_fail(buddy != NULL);
	
	g_free(buddy->status);
	buddy->status = g_strdup(status);

	hybrid_blist_set_buddy_mood(buddy->buddy, status);
}

void
xmpp_buddy_set_show(XmppBuddy *buddy, const gchar *show)
{
	gint state = 0;

	g_return_if_fail(buddy != NULL);
	g_return_if_fail(show != NULL);

	if (g_ascii_strcasecmp(show, "away") == 0) {
		state = HYBRID_STATE_AWAY;

	} else if (g_ascii_strcasecmp(show, "avaiable") == 0) {
		state = HYBRID_STATE_ONLINE;
	} else if (g_ascii_strcasecmp(show, "dnd") == 0) {
		state = HYBRID_STATE_BUSY;
	}

	hybrid_blist_set_buddy_state(buddy->buddy, state);
}

void
xmpp_buddy_set_group(XmppBuddy *buddy, const gchar *group)
{
	g_return_if_fail(buddy != NULL);

	g_free(buddy->group);
	buddy->group = g_strdup(group);
}

gint
xmpp_buddy_get_info(XmppStream *stream, XmppBuddy *buddy)
{
	IqRequest *iq;
	xmlnode *node;

	g_return_val_if_fail(stream != NULL, HYBRID_ERROR);
	g_return_val_if_fail(buddy != NULL, HYBRID_ERROR);

	iq = iq_request_create(stream, IQ_TYPE_GET);

	xmlnode_new_prop(iq->node, "to", buddy->jid);
	node = xmlnode_new_child(iq->node, "vCard");
	xmlnode_new_namespace(node, NULL, "vcard-temp");


	if (iq_request_send(iq) != HYBRID_OK) {

		printf("abc\n");
	}


	return HYBRID_OK;
}

XmppBuddy*
xmpp_buddy_find(const gchar *jid)
{
	g_return_val_if_fail(jid != NULL, NULL);

	if (!xmpp_buddies) {
		return NULL;
	}

	return g_hash_table_lookup(xmpp_buddies, jid);
}

void
xmpp_buddy_destroy(XmppBuddy *buddy)
{
	if (buddy) {
		g_hash_table_remove(xmpp_buddies, buddy);

		if (g_hash_table_size(xmpp_buddies) == 0) {
			g_hash_table_destroy(xmpp_buddies);
			xmpp_buddies = NULL;
		}

		g_free(buddy->jid);
		g_free(buddy->name);
		g_free(buddy->status);
		g_free(buddy->group);

		g_free(buddy);
	}
}
