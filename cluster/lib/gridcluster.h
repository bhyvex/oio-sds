/*
OpenIO SDS cluster
Copyright (C) 2014 Worldine, original work as part of Redcurrant
Copyright (C) 2015 OpenIO, modified as part of OpenIO Software Defined Storage

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

#ifndef OIO_SDS__cluster__lib__gridcluster_h
# define OIO_SDS__cluster__lib__gridcluster_h 1

#include <metautils/lib/metatypes.h>

#define NS_ACL_ALLOW_OPTION "allow"

#define NS_ACL_DENY_OPTION "deny"

extern gboolean oio_cluster_allow_proxy;

struct service_info_s;
void metautils_srvinfo_ensure_tags (struct service_info_s *si);

/* Requests the the best target (conscience, agent proxy) ------------------- */

GError* conscience_get_namespace (const char *ns, struct namespace_info_s **out);
GError* conscience_get_services (const char *ns, const char *type,
		gboolean full, GSList **out);
GError* conscience_get_types (const char *ns, GSList **out);
GError* conscience_push_service (const char *ns, struct service_info_s *si);
GError* conscience_remove_services (const char *ns, const char *type);

GError* register_namespace_service (const struct service_info_s *si);

/* -------------------------------------------------------------------------- */

gboolean namespace_in_worm_mode(namespace_info_t* ns_info);

gchar * namespace_get_state(namespace_info_t* ns_info);
	
gint64 namespace_container_max_size(namespace_info_t* ns_info);

gint64 namespace_chunk_size(const namespace_info_t* ns_info, const char *ns_name);

gchar* namespace_storage_policy(const namespace_info_t* ns_info, const char *ns_name);

gboolean namespace_is_storage_policy_valid(const namespace_info_t* ns_info, const gchar *storage_policy);

gchar* namespace_data_security_value(const namespace_info_t *ns_info, const gchar *wanted_policy);

gchar* namespace_storage_policy_value(const namespace_info_t *ns_info, const gchar *wanted_policy);

/* Extract mode compression state from namespace_info
 * @return TRUE if namespace is in mode compression, FALSE otherwise */
gboolean namespace_in_compression_mode(namespace_info_t* ns_info);

gsize namespace_get_autocontainer_src_offset(namespace_info_t* ns_info);

gsize namespace_get_autocontainer_src_size(namespace_info_t* ns_info);

gsize namespace_get_autocontainer_dst_bits(namespace_info_t* ns_info);

/* Returns the services update's configuration when the Load-Balancing
 * is performed by a servce of type srvtype for each namespace and VNS. */
gchar* gridcluster_get_service_update_policy(struct namespace_info_s *nsinfo);

gint64 gridcluster_get_container_max_versions(struct namespace_info_s *nsinfo);

/* Get the delay before actually removing contents marked as deleted. */
gint64 gridcluster_get_keep_deleted_delay(struct namespace_info_s *nsinfo);

gchar* gridcluster_get_nsinfo_strvalue(struct namespace_info_s *nsinfo,
		const gchar *key, const gchar *def);

gint64 gridcluster_get_nsinfo_int64(struct namespace_info_s *nsinfo,
		const gchar* key, gint64 def);

gchar* gridcluster_get_conscience(const char *ns);

#endif /*OIO_SDS__cluster__lib__gridcluster_h*/
