/*
OpenIO SDS meta2v2
Copyright (C) 2014 Worldine, original work as part of Redcurrant
Copyright (C) 2015-2016 OpenIO, as part of OpenIO Software Defined Storage

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.
*/

#include <errno.h>
#include <strings.h>

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>

#include <meta2v2/meta2_macros.h>
#include <meta2v2/meta2_utils.h>
#include <meta2v2/meta2v2_remote.h>
#include <meta2v2/meta2_bean.h>
#include <meta2v2/generic.h>
#include <meta2v2/autogen.h>

static MESSAGE
_m2v2_build_request(const char *name, struct oio_url_s *url, GByteArray *body)
{
	EXTRA_ASSERT(name != NULL);
	EXTRA_ASSERT(url != NULL);

	MESSAGE msg = metautils_message_create_named(name);
	metautils_message_add_url (msg, url);
	if (body)
		metautils_message_add_body_unref (msg, body);
	return msg;
}

static GByteArray *
_m2v2_pack_request_with_flags(const char *name, struct oio_url_s *url,
		GByteArray *body, guint32 flags)
{
	MESSAGE msg = _m2v2_build_request(name, url, body);
	flags = g_htonl(flags);
	metautils_message_add_field(msg, NAME_MSGKEY_FLAGS, &flags, sizeof(flags));
	return message_marshall_gba_and_clean(msg);
}

static GByteArray *
_m2v2_pack_request(const char *name, struct oio_url_s *url, GByteArray *body)
{
	return message_marshall_gba_and_clean(_m2v2_build_request(name, url, body));
}

//------------------------------------------------------------------------------

void
m2v2_list_result_init (struct list_result_s *p)
{
	EXTRA_ASSERT(p != NULL);
	p->props = g_tree_new_full (metautils_strcmp3, NULL, g_free, g_free);
}

void
m2v2_list_result_clean (struct list_result_s *p)
{
	if (!p) return;
	_bean_cleanl2(p->beans);
	p->beans = NULL;
	if (p->props) g_tree_destroy(p->props);
	p->props = NULL;
	oio_str_clean(&p->next_marker);
	p->truncated = FALSE;
}

gboolean
m2v2_list_result_extract (gpointer ctx, MESSAGE reply)
{
	struct list_result_s *out = ctx;
	EXTRA_ASSERT (out != NULL);

	/* Extract replied aliases */
	GSList *l = NULL;
	GError *e = metautils_message_extract_body_encoded(reply, FALSE, &l,
			bean_sequence_decoder);
	if (e) {
		GRID_DEBUG("Callback error: (%d) %s", e->code, e->message);
		return FALSE;
	}

	out->beans = metautils_gslist_precat (out->beans, l);

	/* Extract list flags */
	e = metautils_message_extract_boolean (reply,
			NAME_MSGKEY_TRUNCATED, FALSE, &out->truncated);
	if (e)
		g_clear_error (&e);
	gchar *tok = metautils_message_extract_string_copy (reply, NAME_MSGKEY_NEXTMARKER);
	oio_str_reuse (&out->next_marker, tok);

	/* Extract properties and merge them into the temporary TreeSet. */
	gchar **names = metautils_message_get_field_names (reply);
	for (gchar **n=names ; n && *n ;++n) {
		if (!g_str_has_prefix (*n, NAME_MSGKEY_PREFIX_PROPERTY))
			continue;
		g_tree_replace (out->props,
				g_strdup((*n) + sizeof(NAME_MSGKEY_PREFIX_PROPERTY) - 1),
				metautils_message_extract_string_copy(reply, *n));
	}
	if (names)
		g_strfreev (names);

	return TRUE;
}

GByteArray*
m2v2_remote_pack_CREATE(struct oio_url_s *url, struct m2v2_create_params_s *pols)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_CREATE, url, NULL);
	if (pols && pols->storage_policy)
		metautils_message_add_field_str(msg, NAME_MSGKEY_STGPOLICY, pols->storage_policy);
	if (pols && pols->version_policy)
		metautils_message_add_field_str(msg, NAME_MSGKEY_VERPOLICY, pols->version_policy);
	if (pols && pols->properties) {
		for (gchar **p=pols->properties; *p && *(p+1) ;p+=2) {
			gchar *k = g_strconcat (NAME_MSGKEY_PREFIX_PROPERTY, *p, NULL);
			metautils_message_add_field_str (msg, k, *(p+1));
			g_free (k);
		}
	}

	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_DESTROY(struct oio_url_s *url, guint32 flags)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_DESTROY, url, NULL);
	if (flags & M2V2_DESTROY_FORCE)
		metautils_message_add_field_str(msg, NAME_MSGKEY_FORCE, "1");
	if (flags & M2V2_DESTROY_FLUSH)
		metautils_message_add_field_str(msg, NAME_MSGKEY_FLUSH, "1");
	if (flags & M2V2_DESTROY_EVENT)
		metautils_message_add_field_str(msg, NAME_MSGKEY_EVENT, "1");

	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_HAS(struct oio_url_s *url)
{
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_HAS, url, NULL);
}

GByteArray*
m2v2_remote_pack_FLUSH(struct oio_url_s *url)
{
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_FLUSH, url, NULL);
}

GByteArray*
m2v2_remote_pack_PURGE(struct oio_url_s *url)
{
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_PURGE, url, NULL);
}

GByteArray*
m2v2_remote_pack_DEDUP(struct oio_url_s *url)
{
	return _m2v2_pack_request (NAME_MSGNAME_M2V2_DEDUP, url, NULL);
}

GByteArray*
m2v2_remote_pack_BEANS(struct oio_url_s *url, const char *pol,
		gint64 size, gboolean append)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_BEANS, url, NULL);
	metautils_message_add_field_strint64 (msg, NAME_MSGKEY_CONTENTLENGTH, size);
	metautils_message_add_field_str (msg, NAME_MSGKEY_STGPOLICY, pol);
	if (append)
		metautils_message_add_field_str (msg, NAME_MSGKEY_APPEND, "true");

	/* si policy est NULL, le paramètre ne sera pas ajouté. On profite que
	 * ce soit ldernier argument de la liste */
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_SPARE(struct oio_url_s *url, const char *pol,
		GSList *notin_list, GSList *broken_list)
{
	gchar *spare_type = M2V2_SPARE_BY_STGPOL;
	GSList *beans = NULL;

	if (notin_list != NULL) {
		spare_type = M2V2_SPARE_BY_BLACKLIST;
		for (GSList *l = notin_list; l != NULL; l = l->next) {
			if (DESCR(l->data) != &descr_struct_CHUNKS)
				continue;
			beans = g_slist_prepend(beans, _bean_dup(l->data));
		}
	}

	for (GSList *l = broken_list; l != NULL; l = l->next) {
		if (DESCR(l->data) != &descr_struct_CHUNKS)
			continue;
		struct bean_CHUNKS_s *chunk = _bean_dup(l->data);
		// This makes difference between valid and broken chunks
		CHUNKS_set_size(chunk, -1);
		beans = g_slist_prepend(beans, chunk);
	}

	/* body is only mandatory for M2V2_SPARE_BY_BLACKLIST so when
	 * notin_list != NULL. If not_in_list != NULL, beans is always
	 * != NULL so body is sent.
	 */
	GByteArray *body = NULL;
	if (beans != NULL)
		body = bean_sequence_marshall(beans);

	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_BEANS, url, body);
	metautils_message_add_field_str (msg, NAME_MSGKEY_STGPOLICY, pol);
	metautils_message_add_field_str (msg, NAME_MSGKEY_SPARE, spare_type);
	_bean_cleanl2(beans);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_PUT(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	MESSAGE msg = _m2v2_build_request (NAME_MSGNAME_M2V2_PUT, url, body);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_OVERWRITE(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_PUT, url, body);
	metautils_message_add_field_str(msg, NAME_MSGKEY_OVERWRITE, "1");
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_UPDATE(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_PUT, url, body);
	metautils_message_add_field_str(msg, NAME_MSGKEY_UPDATE, "1");
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_APPEND(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_APPEND, url, body);
}

GByteArray*
m2v2_remote_pack_COPY(struct oio_url_s *url, const char *src)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_PUT, url, NULL);
	metautils_message_add_field_str(msg, NAME_MSGKEY_COPY, src);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_DEL(struct oio_url_s *url)
{
	return _m2v2_pack_request_with_flags(NAME_MSGNAME_M2V2_DEL, url, NULL, 0);
}

GByteArray*
m2v2_remote_pack_TRUNC(struct oio_url_s *url, gint64 size)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_TRUNC, url, NULL);
	metautils_message_add_field_strint64(msg, NAME_MSGKEY_CONTENTLENGTH, size);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_RAW_DEL(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_RAW_DEL, url, body);
}

GByteArray*
m2v2_remote_pack_RAW_ADD(struct oio_url_s *url, GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_RAW_ADD, url, body);
}

GByteArray*
m2v2_remote_pack_RAW_SUBST(struct oio_url_s *url,
		GSList *new_chunks, GSList *old_chunks)
{
	GByteArray *new_chunks_gba = bean_sequence_marshall(new_chunks);
	GByteArray *old_chunks_gba = bean_sequence_marshall(old_chunks);
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_RAW_SUBST, url, NULL);
	metautils_message_add_field_gba(msg, NAME_MSGKEY_NEW, new_chunks_gba);
	metautils_message_add_field_gba(msg, NAME_MSGKEY_OLD, old_chunks_gba);
	g_byte_array_unref (new_chunks_gba);
	g_byte_array_unref (old_chunks_gba);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_GET(struct oio_url_s *url, guint32 flags)
{
	return _m2v2_pack_request_with_flags(NAME_MSGNAME_M2V2_GET, url, NULL, flags);
}


static void
_pack_list_params (MESSAGE msg, struct list_params_s *p)
{
	guint32 flags = 0;
	if (p->flag_allversion) flags |= M2V2_FLAG_ALLVERSION;
	if (p->flag_headers) flags |= M2V2_FLAG_HEADERS;
	if (p->flag_nodeleted) flags |= M2V2_FLAG_NODELETED;
	flags = g_htonl(flags);
	metautils_message_add_field(msg, NAME_MSGKEY_FLAGS, &flags, sizeof(flags));

	metautils_message_add_field_str(msg, NAME_MSGKEY_PREFIX, p->prefix);
	metautils_message_add_field_str(msg, NAME_MSGKEY_MARKER, p->marker_start);
	metautils_message_add_field_str(msg, NAME_MSGKEY_MARKER_END, p->marker_end);
	if (p->maxkeys > 0)
		metautils_message_add_field_strint64(msg, NAME_MSGKEY_MAX_KEYS, p->maxkeys);
}

GByteArray*
m2v2_remote_pack_LIST(struct oio_url_s *url, struct list_params_s *p)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_LIST, url, NULL);
	_pack_list_params (msg, p);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_LIST_BY_CHUNKID(struct oio_url_s *url, struct list_params_s *p,
		const char *chunk)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_LCHUNK, url, NULL);
	_pack_list_params (msg, p);
	metautils_message_add_field_str (msg, NAME_MSGKEY_KEY, chunk);
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_LIST_BY_HEADERHASH(struct oio_url_s *url, struct list_params_s *p,
		GBytes *h)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_LHHASH, url, NULL);
	_pack_list_params (msg, p);
	metautils_message_add_field (msg, NAME_MSGKEY_KEY,
			g_bytes_get_data(h,NULL), g_bytes_get_size(h));
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_LIST_BY_HEADERID(struct oio_url_s *url, struct list_params_s *p,
		GBytes *h)
{
	MESSAGE msg = _m2v2_build_request(NAME_MSGNAME_M2V2_LHID, url, NULL);
	_pack_list_params (msg, p);
	metautils_message_add_field (msg, NAME_MSGKEY_KEY,
			g_bytes_get_data(h,NULL), g_bytes_get_size(h));
	return message_marshall_gba_and_clean(msg);
}

GByteArray*
m2v2_remote_pack_PROP_DEL(struct oio_url_s *url, GSList *names)
{
	GByteArray *body = strings_marshall_gba(names, NULL);
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_PROP_DEL, url, body);
}

GByteArray*
m2v2_remote_pack_PROP_SET(struct oio_url_s *url, guint32 flags,
		GSList *beans)
{
	GByteArray *body = bean_sequence_marshall(beans);
	return _m2v2_pack_request_with_flags(NAME_MSGNAME_M2V2_PROP_SET, url, body, flags);
}

GByteArray*
m2v2_remote_pack_PROP_GET(struct oio_url_s *url, guint32 flags)
{
	return _m2v2_pack_request_with_flags(NAME_MSGNAME_M2V2_PROP_GET, url, NULL, flags);
}

GByteArray*
m2v2_remote_pack_EXITELECTION(struct oio_url_s *url)
{
	return _m2v2_pack_request(NAME_MSGNAME_M2V2_EXITELECTION, url, NULL);
}

GByteArray*
m2v2_remote_pack_TOUCHC(struct oio_url_s *url)
{
	return _m2v2_pack_request(NAME_MSGNAME_M2V1_TOUCH_CONTENT, url, NULL);
}

GByteArray*
m2v2_remote_pack_TOUCHB(struct oio_url_s *url, guint32 flags)
{
	return _m2v2_pack_request_with_flags(NAME_MSGNAME_M2V1_TOUCH_CONTAINER, url, NULL, flags);
}

GByteArray*
m2v2_remote_pack_ISEMPTY (struct oio_url_s *url)
{
	return message_marshall_gba_and_clean(_m2v2_build_request
			(NAME_MSGNAME_M2V2_ISEMPTY, url, NULL));
}

GByteArray*
m2v2_remote_pack_LINK(struct oio_url_s *url)
{
	return message_marshall_gba_and_clean(_m2v2_build_request(NAME_MSGNAME_M2V2_LINK, url, NULL));
}

/* ------------------------------------------------------------------------- */

static GError*
_m2v2_request_ex(const char *url, GByteArray *req, gdouble timeout, GSList **out)
{
	EXTRA_ASSERT (req != NULL);
	return gridd_client_exec_and_decode (url, timeout, req, out, bean_sequence_decoder);
}

static GError*
_m2v2_request(const char *url, GByteArray *req, GSList **out)
{
	return _m2v2_request_ex(url, req, M2V2_CLIENT_TIMEOUT, out);
}

GError*
m2v2_remote_execute_DESTROY(const char *target, struct oio_url_s *url, guint32 flags)
{
	return _m2v2_request(target, m2v2_remote_pack_DESTROY(url, flags), NULL);
}

GError*
m2v2_remote_execute_DESTROY_many(gchar **targets, struct oio_url_s *url, guint32 flags)
{
	if (!targets)
		return NEWERROR(CODE_INTERNAL_ERROR, "invalid target array (NULL)");

	// TODO: factorize with sqlx_remote_execute_DESTROY_many
	GByteArray *req = m2v2_remote_pack_DESTROY(url, flags);
	struct gridd_client_s **clients = gridd_client_create_many(targets, req, NULL, NULL);
	metautils_gba_unref(req);
	req = NULL;

	if (clients == NULL)
		return NEWERROR(0, "Failed to create gridd clients");

	gridd_clients_start(clients);
	GError *err = gridd_clients_loop(clients);
	for (struct gridd_client_s **p = clients; !err && p && *p ;p++) {
		if (!(err = gridd_client_error(*p)))
			continue;
		GRID_DEBUG("Database destruction attempts failed: (%d) %s",
				err->code, err->message);
		if (err->code == CODE_CONTAINER_NOTFOUND || err->code == CODE_NOT_FOUND) {
			g_clear_error(&err);
			continue;
		}
	}

	gridd_clients_free(clients);
	return err;
}

#if 0
static GError*
_list (const char *target, GByteArray *request,
		struct ist_result_s *out, gchar ***out_properties)
{
	GError *err = NULL;
	GTree *props = NULL;

	EXTRA_ASSERT(target != NULL);

	struct gridd_client_s *client = gridd_client_create_idle(target);
	if (!client)
		err = NEWERROR(2, "errno=%d %s", errno, strerror(errno));
	if (!err)
		err = gridd_client_request(client, request, props, out ? _cb : NULL);
	if (!err) {
		if (out_properties)
			props = g_tree_new_full (metautils_strcmp3, NULL, g_free, g_free);
		err = gridd_client_run (client);
	}
	if (!err && out_properties && props) {
		gboolean _run (gchar *k, gchar *v, GPtrArray *tmp) {
			g_ptr_array_add (tmp, g_strdup(k));
			g_ptr_array_add (tmp, g_strdup(v));
			return FALSE;
		}
		GPtrArray *tmp = g_ptr_array_new ();
		g_tree_foreach (props, (GTraverseFunc)_run, tmp);
		*out_properties = (gchar**) metautils_gpa_to_array (tmp, TRUE);
		tmp = NULL;
	}

	gridd_client_free(client);
	g_byte_array_free(request, TRUE);
	if (props) g_tree_unref (props);
	return err;
}
#endif
