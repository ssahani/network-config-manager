/* Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "alloc-util.h"
#include "config-file.h"
#include "config-parser.h"
#include "dbus.h"
#include "dracut-parser.h"
#include "file-util.h"
#include "log.h"
#include "macros.h"
#include "network-manager.h"
#include "network.h"
#include "networkd-api.h"
#include "parse-util.h"
#include "string-util.h"

static const char *const dhcp_client_mode_table[_DHCP_CLIENT_MAX] = {
        [DHCP_CLIENT_NO]   = "no",
        [DHCP_CLIENT_YES]  = "yes",
        [DHCP_CLIENT_IPV4] = "ipv4",
        [DHCP_CLIENT_IPV6] = "ipv6",
};

const char *dhcp_client_modes_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(dhcp_client_mode_table))
                return NULL;

        return dhcp_client_mode_table[id];
}

int dhcp_client_name_to_mode(char *name) {
        assert(name);

        for (size_t i = DHCP_CLIENT_NO; i < (size_t) ELEMENTSOF(dhcp_client_mode_table); i++)
                if (string_equal_fold(name, dhcp_client_mode_table[i]))
                        return i;

        return _DHCP_CLIENT_INVALID;
}

static const char *const dhcp_client_kind_table[_DHCP_CLIENT_MAX] = {
        [DHCP_CLIENT_IPV4]   = "ipv4",
        [DHCP_CLIENT_IPV6]   = "ipv6",
};

const char *dhcp_client_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(dhcp_client_kind_table))
                return NULL;

        return dhcp_client_kind_table[id];
}

int dhcp_name_to_client(char *name) {
        assert(name);

        for (size_t i = DHCP_CLIENT_IPV4; i < (size_t) ELEMENTSOF(dhcp_client_kind_table); i++)
                if (string_equal_fold(name, dhcp_client_kind_table[i]))
                        return i;

        if (string_equal(name, "4"))
                return DHCP_CLIENT_IPV4;
        else if (string_equal(name, "6"))
                return DHCP_CLIENT_IPV6;

        return _DHCP_CLIENT_INVALID;
}

static const char *const dhcp_client_identifier[_DHCP_CLIENT_IDENTIFIER_MAX] = {
        [DHCP_CLIENT_IDENTIFIER_MAC]       = "mac",
        [DHCP_CLIENT_IDENTIFIER_DUID]      = "duid",
        [DHCP_CLIENT_IDENTIFIER_DUID_ONLY] = "duid-ony",
};

const char *dhcp_client_identifier_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(dhcp_client_identifier))
                return NULL;

        return dhcp_client_identifier[id];
}

int dhcp_client_identifier_to_mode(char *name) {
        assert(name);

        for (size_t i = DHCP_CLIENT_IDENTIFIER_MAC; i < (size_t) ELEMENTSOF(dhcp_client_identifier); i++)
                if (string_equal_fold(name, dhcp_client_identifier[i]))
                        return i;

        return _DHCP_CLIENT_IDENTIFIER_INVALID;
}

static const char *const dhcp_client_duid_type [_DHCP_CLIENT_DUID_TYPE_MAX] =  {
        [DHCP_CLIENT_DUID_TYPE_LINK_LAYER_TIME] = "link-layer-time",
        [DHCP_CLIENT_DUID_TYPE_VENDOR]          = "vendor",
        [DHCP_CLIENT_DUID_TYPE_LINK_LAYER]      = "link-layer",
        [DHCP_CLIENT_DUID_TYPE_UUID]            = "uuid",
};

const char *dhcp_client_duid_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(dhcp_client_duid_type))
                return NULL;

        return dhcp_client_duid_type[id];
}

int dhcp_client_duid_name_to_type(char *name) {
        assert(name);

        for (size_t i = DHCP_CLIENT_DUID_TYPE_LINK_LAYER_TIME; i < (size_t) ELEMENTSOF(dhcp_client_duid_type); i++)
                if (string_equal_fold(name, dhcp_client_duid_type[i]))
                        return i;

        return _DHCP_CLIENT_DUID_TYPE_INVALID;
}

static const char *const link_local_address_type[_LINK_LOCAL_ADDRESS_MAX] =  {
        [LINK_LOCAL_ADDRESS_YES]           = "yes",
        [LINK_LOCAL_ADDRESS_NO]            = "no",
        [LINK_LOCAL_ADDRESS_IPV4]          = "ipv4",
        [LINK_LOCAL_ADDRESS_IPV6]          = "ipv6",
        [LINK_LOCAL_ADDRESS_FALLBACK]      = "fallback",
        [LINK_LOCAL_ADDRESS_IPV4_FALLBACK] = "ipv4-fallback",
};

const char *link_local_address_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(link_local_address_type))
                return NULL;

        return link_local_address_type[id];
}

int link_local_address_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = LINK_LOCAL_ADDRESS_YES; i < (size_t) ELEMENTSOF(link_local_address_type); i++)
                if (string_equal_fold(name, link_local_address_type[i]))
                        return i;

        return _LINK_LOCAL_ADDRESS_INVALID;
}

static const char *const ipv6_ra_preference_type[_IPV6_RA_PREFERENCE_MAX] =  {
        [IPV6_RA_PREFERENCE_LOW]    = "low",
        [IPV6_RA_PREFERENCE_MEDIUM] = "medium",
        [IPV6_RA_PREFERENCE_HIGH]   = "high",
};

const char *ipv6_ra_preference_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(ipv6_ra_preference_type))
                return NULL;

        return ipv6_ra_preference_type[id];
}

int ipv6_ra_preference_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = IPV6_RA_PREFERENCE_LOW; i < (size_t) ELEMENTSOF(ipv6_ra_preference_type); i++)
                if (string_equal_fold(name, ipv6_ra_preference_type[i]))
                        return i;

        return _IPV6_RA_PREFERENCE_INVALID;
}

static const char *const ip_duplicate_address_detection_type[_IP_DUPLICATE_ADDRESS_DETECTION_MAX] =  {
        [IP_DUPLICATE_ADDRESS_DETECTION_NONE] = "none",
        [IP_DUPLICATE_ADDRESS_DETECTION_IPV4] = "ipv4",
        [IP_DUPLICATE_ADDRESS_DETECTION_IPV6] = "ipv6",
        [IP_DUPLICATE_ADDRESS_DETECTION_BOTH] = "both",
};

const char *ip_duplicate_address_detection_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(ip_duplicate_address_detection_type))
                return NULL;

        return ip_duplicate_address_detection_type[id];
}

int ip_duplicate_address_detection_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = IP_DUPLICATE_ADDRESS_DETECTION_NONE; i < (size_t) ELEMENTSOF(ip_duplicate_address_detection_type); i++)
                if (string_equal_fold(name, ip_duplicate_address_detection_type[i]))
                        return i;

        return _IP_DUPLICATE_ADDRESS_DETECTION_INVALID;
}

static const char *const route_scope_type[_ROUTE_SCOPE_MAX] =  {
        [ROUTE_SCOPE_UNIVERSE] = "global",
        [ROUTE_SCOPE_SITE]     = "site",
        [ROUTE_SCOPE_LINK]     = "link",
        [ROUTE_SCOPE_HOST]     = "host",
        [ROUTE_SCOPE_NOWHERE]  = "nowhere",
};

const char *route_scope_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(route_scope_type))
                return NULL;

        return route_scope_type[id];
}

int route_scope_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = ROUTE_SCOPE_UNIVERSE; i < (size_t) ELEMENTSOF(route_scope_type); i++)
                if (string_equal_fold(name, route_scope_type[i]))
                        return i;

        return _ROUTE_SCOPE_INVALID;
}

static const char * const route_type[_ROUTE_TYPE_MAX] = {
        [ROUTE_TYPE_UNICAST]     = "unicast",
        [ROUTE_TYPE_LOCAL]       = "local",
        [ROUTE_TYPE_BROADCAST]   = "broadcast",
        [ROUTE_TYPE_ANYCAST]     = "anycast",
        [ROUTE_TYPE_MULTICAST]   = "multicast",
        [ROUTE_TYPE_BLACKHOLE]   = "blackhole",
        [ROUTE_TYPE_UNREACHABLE] = "unreachable",
        [ROUTE_TYPE_PROHIBIT]    = "prohibit",
        [ROUTE_TYPE_THROW]       = "throw",
        [ROUTE_TYPE_NAT]         = "nat",
        [ROUTE_TYPE_XRESOLVE]    = "xresolve",
};

const char *route_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(route_type))
                return NULL;

        return route_type[id];
}

int route_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = ROUTE_TYPE_UNICAST; i < (size_t) ELEMENTSOF(route_type); i++)
                if (string_equal_fold(name, route_type[i]))
                        return i;

        return _ROUTE_TYPE_INVALID;
}

static const char * const ipv6_route_preference_type[_IPV6_ROUTE_PREFERENCE_MAX] = {
        [IPV6_ROUTE_PREFERENCE_LOW]     = "low",
        [IPV6_ROUTE_PREFERENCE_MEDIUM]  = "medium",
        [IPV6_ROUTE_PREFERENCE_HIGH]    = "high",
};

const char *ipv6_route_preference_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(ipv6_route_preference_type))
                return NULL;

        return ipv6_route_preference_type[id];
}

int ipv6_route_preference_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = IPV6_ROUTE_PREFERENCE_LOW; i < (size_t) ELEMENTSOF(ipv6_route_preference_type); i++)
                if (string_equal_fold(name, ipv6_route_preference_type[i]))
                        return i;

        return _IPV6_ROUTE_PREFERENCE_INVALID;
}

static const char * const route_protocol_type[_ROUTE_PROTOCOL_MAX] = {
       [ROUTE_PROTOCOL_KERNEL]  = "kernel",
       [ROUTE_PROTOCOL_BOOT]    = "boot",
       [ROUTE_PROTOCOL_STATIC]  = "static",
       [ROUTE_PRTOCOL_DHCP]     = "dhcp",
};

const char *route_protocol_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(route_protocol_type))
                return NULL;

        return route_protocol_type[id];
}

int route_protocol_to_mode(const char *name) {
        assert(name);

        for (size_t i = IPV6_ROUTE_PREFERENCE_LOW; i < (size_t) ELEMENTSOF(route_protocol_type); i++)
                if (string_equal_fold(name, route_protocol_type[i]))
                        return i;

        return _ROUTE_PROTOCOL_INVALID;
}

static const char * const route_table_type[_ROUTE_TABLE_MAX] = {
       [ROUTE_TABLE_LOCAL]    = "local",
       [ROUTE_TABLE_MAIN]     = "main",
       [ROUTE_TABLE_DEFAULT]  = "default",
};

const char *route_table_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(route_table_type))
                return NULL;

        return route_table_type[id];
}

int route_table_to_mode(const char *name) {
        assert(name);

        for (size_t i = ROUTE_TABLE_DEFAULT; i < (size_t) ELEMENTSOF(route_table_type); i++)
                if (string_equal_fold(name, route_table_type[i]))
                        return i;

        return _ROUTE_TABLE_INVALID;
}

static const char *const auth_key_management_type[_AUTH_KEY_MANAGEMENT_MAX] =  {
        [AUTH_KEY_MANAGEMENT_NONE]    = "password",
        [AUTH_KEY_MANAGEMENT_WPA_PSK] = "psk",
        [AUTH_KEY_MANAGEMENT_WPA_EAP] = "eap",
        [AUTH_KEY_MANAGEMENT_8021X]   = "8021x",
};

const char *auth_key_management_type_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(auth_key_management_type))
                return NULL;

        return auth_key_management_type[id];
}

int auth_key_management_type_to_mode(const char *name) {
        assert(name);

        for (size_t i = AUTH_KEY_MANAGEMENT_NONE; i < (size_t) ELEMENTSOF(auth_key_management_type); i++)
                if (string_equal_fold(name, auth_key_management_type[i]))
                        return i;

        return _AUTH_KEY_MANAGEMENT_INVALID;
}

static const char* const auth_eap_method_type[_AUTH_EAP_METHOD_MAX] =  {
        [AUTH_EAP_METHOD_NONE] = "none",
        [AUTH_EAP_METHOD_TLS]  = "tls",
        [AUTH_EAP_METHOD_PEAP] = "peap",
        [AUTH_EAP_METHOD_TTLS] = "ttls",
};

const char *auth_eap_method_to_name(int id) {
        if (id < 0)
                return NULL;

        if ((size_t) id >= ELEMENTSOF(auth_eap_method_type))
                return NULL;

        return auth_eap_method_type[id];
}

int auth_eap_method_to_mode(const char *name) {
        assert(name);

        for (size_t i = AUTH_EAP_METHOD_NONE; i < (size_t) ELEMENTSOF(auth_eap_method_type); i++)
                if (string_equal_fold(name, auth_eap_method_type[i]))
                        return i;

        return _AUTH_EAP_METHOD_INVALID;
}

int create_network_conf_file(const char *ifname, char **ret) {
        _auto_cleanup_ char *file = NULL, *network = NULL;
        int r;

        assert(ifname);

        file = string_join("-", "10", ifname, NULL);
        if (!file)
                return log_oom();

        r = create_conf_file("/etc/systemd/network", file, "network", &network);
        if (r < 0)
                return r;

        r = set_config_file_string(network, "Match", "Name", ifname);
        if (r < 0)
                return r;

        r = set_file_permisssion(network, "systemd-network");
        if (r < 0)
                 return r;

        if (ret)
                *ret = steal_pointer(network);


        return dbus_network_reload();
}

int create_or_parse_network_file(const IfNameIndex *ifidx, char **ret) {
        _auto_cleanup_ char *setup = NULL, *network = NULL;
        int r;

        assert(ifidx);

        r = network_parse_link_setup_state(ifidx->ifindex, &setup);
        if (r < 0) {
                r = create_network_conf_file(ifidx->ifname, &network);
                if (r < 0)
                        return r;
        } else {
                r = network_parse_link_network_file(ifidx->ifindex, &network);
                if (r < 0) {
                        r = create_network_conf_file(ifidx->ifname, &network);
                        if (r < 0)
                                return r;
                }
        }

        if (!g_file_test(network, G_FILE_TEST_EXISTS)) {
                r = create_network_conf_file(ifidx->ifname, &network);
                if (r < 0)
                        return r;
        }

        *ret = steal_pointer(network);
        return 0;
}

int routing_policy_rule_new(RoutingPolicyRule **ret) {
        RoutingPolicyRule *rule;

        rule = new0(RoutingPolicyRule, 1);
        if (!rule)
                return -ENOMEM;

        *ret = rule;
        return 0;
}

void routing_policy_rule_free(RoutingPolicyRule *rule) {
        if (!rule)
                return;

        free(rule->ipproto);
        free(rule->sport);
        free(rule->dport);
        free(rule);
}

int network_new(Network **ret) {
        _auto_cleanup_ Network *n = NULL;
        int r;

        n = new0(Network, 1);
        if (!n)
                return log_oom();

        *n = (Network) {
                .unmanaged = -1,
                .arp = -1,
                .multicast = -1,
                .all_multicast = -1,
                .promiscuous = -1,
                .req_for_online = -1,
                .dhcp_type = _DHCP_CLIENT_INVALID,
                .dhcp4_use_mtu = -1,
                .dhcp4_use_dns = -1,
                .dhcp4_use_domains = -1,
                .dhcp4_use_ntp = -1,
                .dhcp6_use_dns = -1,
                .dhcp6_use_ntp = -1,
                .gateway_onlink = -1,
                .lldp = -1,
                .ipv6_accept_ra = -1,
                .dhcp_client_identifier_type = _DHCP_CLIENT_IDENTIFIER_INVALID,
                .link_local = _LINK_LOCAL_ADDRESS_INVALID,
                .parser_type = _PARSER_TYPE_INVALID,
        };

        r = set_new(&n->addresses, NULL, NULL);
        if (r < 0)
                return r;

        r = set_new(&n->nameservers, NULL, NULL);
        if (r < 0)
                return r;

        r = set_new(&n->ntps, NULL, NULL);
        if (r < 0)
                return r;

        *ret = steal_pointer(n);
        return 0;
}

static int wifi_access_point_unref (void *key, void *value, void *user_data) {
        WiFiAccessPoint *ap = value;

        if (!ap)
                return 0;

        free(ap->ssid);
        free(ap->auth->identity);
        free(ap->auth->anonymous_identity);
        free(ap->auth->password);
        free(ap->auth->ca_certificate);
        free(ap->auth->client_certificate);
        free(ap->auth->client_key);
        free(ap->auth->client_key_password);

        free(ap->auth);
        free(ap);

        return 0;
}

void network_unref(Network *n) {
        if (!n)
                return;

        set_unrefp(&n->addresses);
        set_unrefp(&n->ntps);
        set_unrefp(&n->nameservers);

        if (n->access_points) {
                g_hash_table_foreach_steal(n->access_points, wifi_access_point_unref, NULL);
                g_hash_table_destroy(n->access_points);
        }

        free(n->ifname);
        free(n->mac);
        free(n->match_mac);
        free(n->hostname);
        free(n->gateway);
        free(n->req_family_for_online);
        free(n);
}

void g_network_free (gpointer data) {
        Network *n;

        n = data;
        network_unrefp(&n);
}

int parse_address_from_string_and_add(const char *s, Set *a) {
        _auto_cleanup_ IPAddress *address = NULL;
        _auto_cleanup_ char *p = NULL;
        int r;

        if (set_contains(a, (void *) s))
                return -EEXIST;

        r = parse_ip_from_string(s, &address);
        if (r < 0)
                return r;

        p = g_strdup(s);
        if (!p)
                return log_oom();

        (void) set_add(a, p);

        steal_pointer(p);
        return 0;
}

static void append_wpa_auth_conf(const WIFIAuthentication *auth, GString *s) {
        assert(s);
        assert(auth);

        switch (auth->key_management) {
        case AUTH_KEY_MANAGEMENT_NONE:
                break;
        case AUTH_KEY_MANAGEMENT_WPA_PSK:
                g_string_append(s, "        key_mgmt=WPA-PSK\n");
                break;
        case AUTH_KEY_MANAGEMENT_WPA_EAP:
                g_string_append(s, "        key_mgmt=WPA-EAP\n");
                break;
        case AUTH_KEY_MANAGEMENT_8021X:
                g_string_append(s, "        key_mgmt=IEEE8021X\n");
                break;
        default:
                break;
        }

        switch (auth->eap_method) {
        case AUTH_EAP_METHOD_NONE:
                break;
        case AUTH_EAP_METHOD_TLS:
                g_string_append(s, "        eap=TLS\n");
                break;
        case AUTH_EAP_METHOD_PEAP:
                g_string_append(s, "        eap=PEAP\n");
                break;
        case AUTH_EAP_METHOD_TTLS:
                g_string_append(s, "        eap=TTLS\n");
                break;
        default:
                break;
        }

        if (auth->identity)
                g_string_append_printf(s, "        identity=\"%s\"\n", auth->identity);

        if (auth->anonymous_identity)
                g_string_append_printf(s, "        anonymous_identity=\"%s\"\n", auth->anonymous_identity);

        if (auth->password) {
                if (auth->key_management == AUTH_KEY_MANAGEMENT_WPA_PSK)
                        g_string_append_printf(s, "        psk=\"%s\"\n", auth->password);
                else
                        g_string_append_printf(s, "        password=\"%s\"\n", auth->password);
        }

        if (auth->ca_certificate)
                g_string_append_printf(s, "        ca_cert=\"%s\"\n", auth->ca_certificate);

        if (auth->client_certificate)
                g_string_append_printf(s, "        client_cert=\"%s\"\n", auth->client_certificate);

        if (auth->client_key)
                g_string_append_printf(s, "        private_key=\"%s\"\n", auth->client_key);

        if (auth->client_key_password)
                g_string_append_printf(s, "        private_key_passwd=\"%s\"\n", auth->client_key_password);
}

static void append_access_points(gpointer key, gpointer value, gpointer userdata) {
        GString *config = userdata;
        WiFiAccessPoint *ap = value;

        g_string_append(config, "network={\n");
        g_string_append_printf(config, "        ssid=\"%s\"\n", ap->ssid);

        append_wpa_auth_conf(ap->auth, config);

        g_string_append(config, "}\n\n");
}

int generate_wifi_config(Network *n, GString **ret) {
        _cleanup_(g_string_unrefp) GString *config = NULL;

        assert(n);

        config = g_string_new(NULL);
        if (!config)
                return log_oom();

        g_string_append(config, "# WPA Supplicant Configuration\n"
                                "# this goes in /etc/network-config-manager/wpa_supplicant.conf on Photon OS\n"
                                "# chown root, chmod 600 \n\n");
        g_string_append(config, "# allow frontend (e.g., wpa_cli) to be used by all users in 'wheel' group\n"
                                "ctrl_interface=DIR=/run/wpa_supplicant GROUP=wheel\n"
                                "update_config=1\n\n");

        g_hash_table_foreach(n->access_points, append_access_points, config);

        *ret = steal_pointer(config);
        return 0;
}

static void append_routes(gpointer key, gpointer value, gpointer userdata) {
        _auto_cleanup_ char *gateway = NULL, *destination = NULL;
        _cleanup_(section_freep) Section *section = NULL;
        KeyFile *key_file = userdata;
        Route *route = value;
        int r;

        if (ip_is_null(&route->destination) && ip_is_null(&route->gw))
                return;

        r = section_new("Route", &section);
        if (r < 0)
                return;

        if (!ip_is_null(&route->destination)) {
                (void) ip_to_string(AF_INET, &route->destination, &destination);
                (void ) add_key_to_section(section, "Destination", destination);
        }

        if (!ip_is_null(&route->gw)) {
                (void) ip_to_string(AF_INET, &route->gw, &gateway);
                (void) add_key_to_section(section, "Gateway", gateway);
        }

        r = add_section_to_key_file(key_file, section);
        if (r < 0)
                return;

        steal_pointer(section);
}

static void append_nameservers(gpointer key, gpointer value, gpointer userdata) {
        GString *config = userdata;

        g_string_append_printf(config, "%s ", (char *) key);
}

static void append_ntp(gpointer key, gpointer value, gpointer userdata) {
        GString *config = userdata;

        g_string_append_printf(config, "%s ", (char *) key);
}

static void append_addresses(gpointer key, gpointer value, gpointer userdata) {
        _cleanup_(section_freep) Section *section = NULL;
        KeyFile *key_file = userdata;
        int r;

        r = section_new("Address", &section);
        if (r < 0)
                return;

        (void) add_key_to_section(section, "Address", (char *) key);

        r = add_section_to_key_file(key_file, section);
        if (r < 0)
                return;

        steal_pointer(section);
}

int generate_network_config(Network *n) {
        _auto_cleanup_ char *gateway = NULL, *network = NULL;
        _cleanup_(key_file_freep) KeyFile *key_file = NULL;
        int r;

        assert(n);

        r = create_network_conf_file(n->ifname, &network);
        if (r < 0)
                return r;

        r = parse_key_file(network, &key_file);
        if (r < 0)
                return r;

        r = set_config(key_file, "Match", "Name", n->ifname);
        if (r < 0)
                return r;

        if (n->match_mac) {
                r = set_config(key_file, "Match", "MACAddress", n->match_mac);
                if (r < 0)
                        return r;
        }

        if (n->unmanaged >= 0 || n->arp >= 0 || n->multicast >= 0 || n->all_multicast >= 0 || n->promiscuous >= 0 ||
            n->req_for_online >= 0 || n->mtu > 0 || n->mac || n->req_family_for_online) {

                if (n->unmanaged >= 0) {
                        r = set_config(key_file, "Link", "Unmanaged", bool_to_string(!n->unmanaged));
                        if (r < 0)
                                return r;
                }

                if (n->arp >= 0) {
                        r = set_config(key_file, "Link", "ARP", bool_to_string(n->arp));
                        if (r < 0)
                                return r;
                }

                if (n->multicast >= 0) {
                        r = set_config(key_file, "Link", "Multicast", bool_to_string(n->multicast));
                        if (r < 0)
                                return r;
                }

                if (n->all_multicast >= 0) {
                        r = set_config(key_file, "Link", "AllMulticast", bool_to_string(n->all_multicast));
                        if (r < 0)
                                return r;
                }

                if (n->promiscuous >= 0) {
                        r = set_config(key_file, "Link", "Promiscuous", bool_to_string(n->promiscuous));
                        if (r < 0)
                                return r;
                }

                if (n->req_for_online >= 0) {
                        r = set_config(key_file, "Link", "RequiredForOnline", bool_to_string(n->req_for_online));
                        if (r < 0)
                                return r;
                }

                if (n->mtu > 0) {
                        r = key_file_set_uint(key_file, "Link", "MTUBytes", n->mtu);
                        if (r < 0)
                                return r;
                }

                if (n->mac) {
                        r = set_config(key_file, "Link", "MACAddress", n->mac);
                        if (r < 0)
                                return r;
                }

                if (n->req_family_for_online) {
                        r = set_config(key_file, "Link", "RequiredFamilyForOnline", n->req_family_for_online);
                        if (r < 0)
                                return r;
                }
        }

        if (n->dhcp_type != _DHCP_CLIENT_INVALID) {
                if (n->parser_type == PARSER_TYPE_YAML)
                        r = set_config(key_file, "Network", "DHCP", dhcp_client_modes_to_name(n->dhcp_type));
                else
                        r = set_config(key_file, "Network", "DHCP", dracut_to_networkd_dhcp_mode_to_name(n->dhcp_type));
        }

        if (n->lldp >= 0) {
                r = set_config(key_file, "Network", "LLDP", bool_to_string(n->lldp));
                if (r < 0)
                        return r;
        }

        if (n->link_local != _LINK_LOCAL_ADDRESS_INVALID) {
                r = set_config(key_file, "Network", "LinkLocalAddressing", link_local_address_type_to_name(n->link_local));
                if (r < 0)
                        return r;

        }

        if (n->ipv6_accept_ra >= 0) {
                r = set_config(key_file, "Network", "IPv6AcceptRA", bool_to_string(n->ipv6_accept_ra));
                if (r < 0)
                        return r;
        }

        if (n->nameservers && set_size(n->nameservers) > 0) {
                _cleanup_(g_string_unrefp) GString *c = NULL;

                c = g_string_new(NULL);
                if (!c)
                        return log_oom();

                set_foreach(n->nameservers, append_nameservers, c);
                r = set_config(key_file, "Network", "DNS", c->str);
                if (r < 0)
                        return r;
        }

        if (n->ntps && set_size(n->ntps) > 0) {
                _cleanup_(g_string_unrefp) GString *c = NULL;

                c = g_string_new(NULL);
                if (!c)
                        return log_oom();

                set_foreach(n->ntps, append_ntp, c);
                r = set_config(key_file, "Network", "NTP", c->str);
                if (r < 0)
                        return r;
        }

        if (n->netdev) {
                r = set_config(key_file, "Network", netdev_kind_to_name(n->netdev->kind), n->netdev->ifname);
                if (r < 0)
                        return r;
        }

        if (n->dhcp_client_identifier_type != _DHCP_CLIENT_IDENTIFIER_INVALID || n->dhcp4_use_dns >= 0 || n->dhcp4_use_domains >= 0 ||
            n->dhcp4_use_ntp >= 0 || n->dhcp4_use_mtu >= 0) {

                if (n->dhcp_client_identifier_type != _DHCP_CLIENT_IDENTIFIER_INVALID) {
                        r = set_config(key_file, "DHCPv4", "ClientIdentifier", dhcp_client_identifier_to_name(n->dhcp_client_identifier_type));
                        if (r < 0)
                                return r;
                }

                if (n->dhcp4_use_dns >= 0) {
                        r = set_config(key_file, "DHCPv4", "UseDNS", bool_to_string(n->dhcp4_use_dns));
                        if (r < 0)
                                return r;
                }

                if (n->dhcp4_use_domains >= 0) {
                        r = set_config(key_file, "DHCPv4", "UseDomains", bool_to_string(n->dhcp4_use_domains));
                        if (r < 0)
                                return r;
                }

                if (n->dhcp4_use_ntp >= 0) {
                        r = set_config(key_file, "DHCPv4", "UseNTP", bool_to_string(n->dhcp4_use_ntp));
                        if (r < 0)
                                return r;
                }

                if (n->dhcp4_use_mtu >= 0) {
                        r = set_config(key_file, "DHCPv4", "UseMTU", bool_to_string(n->dhcp4_use_mtu));
                        if (r < 0)
                                return r;
                }
        }

        if ( n->dhcp6_use_dns >= 0 || n->dhcp6_use_ntp >= 0) {
                if (n->dhcp6_use_dns >= 0) {
                        r = set_config(key_file, "DHCPv6", "UseDNS", bool_to_string(n->dhcp4_use_dns));
                         if (r < 0)
                                return r;
                }

                if (n->dhcp6_use_ntp >= 0) {
                        r = set_config(key_file, "DHCPv6", "UseNTP", bool_to_string(n->dhcp6_use_ntp));
                        if (r < 0)
                                return r;
                }
        }

        if (n->addresses && set_size(n->addresses) > 0)
                set_foreach(n->addresses, append_addresses, key_file);

        if (n->gateway && !ip_is_null(n->gateway)) {
                _cleanup_(section_freep) Section *section = NULL;

                (void) ip_to_string_prefix(AF_INET, n->gateway, &gateway);

                r = section_new("Route", &section);
                if (r < 0)
                        return r;

                add_key_to_section(section, "Gateway", gateway);
                add_key_to_section(section, "GatewayOnlink", bool_to_string(n->gateway_onlink));

                r = add_section_to_key_file(key_file, section);
                if (r < 0)
                        return r;

                steal_pointer(section);
        }

        if (n->routes)
                g_hash_table_foreach(n->routes, append_routes, key_file);

        r = key_file_save (key_file);
        if (r < 0) {
                log_warning("Failed to write to '%s': %s", key_file->name, strerror(-r));
                return r;
        }

        r = set_file_permisssion(network, "systemd-network");
        if (r < 0)
                return r;

        (void) dbus_network_reload();
        return 0;
}
