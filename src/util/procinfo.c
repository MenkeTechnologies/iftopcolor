/*
 * procinfo.c:
 *
 * Map network sockets to owning processes.
 * macOS: libproc (proc_pidinfo / proc_pidfdinfo)
 * Linux: /proc/net/tcp* + /proc/[pid]/fd/ symlink walking
 *
 * Thread safety: procinfo_refresh() is called from the tick thread,
 * procinfo_lookup() may be called from the packet capture thread.
 * We use a double-buffer: refresh builds a new table, then swaps
 * the live pointer atomically.
 */

#include "../include/procinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdatomic.h>

/* ── Internal lookup table ────────────────────────────────────────── */

typedef struct {
    int af;
    unsigned short protocol;
    unsigned short port;
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } addr;
    pid_t pid;
    char name[PROCINFO_NAME_MAX];
} sock_entry;

typedef struct {
    sock_entry *entries;
    int count;
    int cap;
} sock_table_t;

/* Double-buffer: live table is read by lookup, staging is built by refresh */
static _Atomic(sock_table_t *) live_table = NULL;
static sock_table_t *staging_table = NULL;

static sock_table_t *table_create(void) {
    sock_table_t *t = calloc(1, sizeof(sock_table_t));
    return t;
}

static void table_free(sock_table_t *t) {
    if (t) {
        free(t->entries);
        free(t);
    }
}

static void table_add(sock_table_t *t, int af, unsigned short protocol,
                      unsigned short port, const void *addr, pid_t pid,
                      const char *name) {
    if (t->count >= t->cap) {
        t->cap = t->cap ? t->cap * 2 : 512;
        t->entries = realloc(t->entries, t->cap * sizeof(sock_entry));
        if (!t->entries) {
            t->count = 0;
            t->cap = 0;
            return;
        }
    }
    sock_entry *e = &t->entries[t->count++];
    e->af = af;
    e->protocol = protocol;
    e->port = port;
    if (af == AF_INET) {
        memcpy(&e->addr.v4, addr, sizeof(struct in_addr));
    } else {
        memcpy(&e->addr.v6, addr, sizeof(struct in6_addr));
    }
    e->pid = pid;
    strncpy(e->name, name, PROCINFO_NAME_MAX - 1);
    e->name[PROCINFO_NAME_MAX - 1] = '\0';
}

static int table_find(sock_table_t *t, int af, const void *addr,
                      unsigned short port, unsigned short protocol,
                      proc_entry *out) {
    if (!t) return 0;
    for (int i = 0; i < t->count; i++) {
        sock_entry *e = &t->entries[i];
        if (e->port != port || e->protocol != protocol)
            continue;
        if (e->af == af) {
            if (af == AF_INET) {
                if (memcmp(&e->addr.v4, addr, sizeof(struct in_addr)) == 0 ||
                    e->addr.v4.s_addr == INADDR_ANY) {
                    out->pid = e->pid;
                    strncpy(out->name, e->name, PROCINFO_NAME_MAX);
                    return 1;
                }
            } else if (af == AF_INET6) {
                static const struct in6_addr any = IN6ADDR_ANY_INIT;
                if (memcmp(&e->addr.v6, addr, sizeof(struct in6_addr)) == 0 ||
                    memcmp(&e->addr.v6, &any, sizeof(struct in6_addr)) == 0) {
                    out->pid = e->pid;
                    strncpy(out->name, e->name, PROCINFO_NAME_MAX);
                    return 1;
                }
            }
        }
        /* Handle IPv4-mapped IPv6: socket bound as ::ffff:A.B.C.D, packet is AF_INET */
        if (af == AF_INET && e->af == AF_INET6) {
            const unsigned char *b = e->addr.v6.s6_addr;
            static const unsigned char v4mapped_prefix[12] = {
                0,0,0,0, 0,0,0,0, 0,0,0xff,0xff
            };
            if (memcmp(b, v4mapped_prefix, 12) == 0 &&
                memcmp(b + 12, addr, 4) == 0 &&
                e->port == port && e->protocol == protocol) {
                out->pid = e->pid;
                strncpy(out->name, e->name, PROCINFO_NAME_MAX);
                return 1;
            }
        }
    }
    return 0;
}

/* Swap staging into live, free the old live table */
static void table_publish(void) {
    sock_table_t *old = atomic_exchange(&live_table, staging_table);
    staging_table = NULL;
    table_free(old);
}

/* ── macOS implementation ─────────────────────────────────────────── */

#if defined(__APPLE__)

#include <libproc.h>
#include <sys/proc_info.h>

int procinfo_available(void) { return 1; }

void procinfo_refresh(void) {
    staging_table = table_create();
    if (!staging_table) return;

    int npids = proc_listallpids(NULL, 0);
    if (npids <= 0) { table_publish(); return; }

    pid_t *pids = malloc(npids * sizeof(pid_t));
    if (!pids) { table_publish(); return; }

    npids = proc_listallpids(pids, npids * sizeof(pid_t));
    if (npids <= 0) {
        free(pids);
        table_publish();
        return;
    }

    for (int p = 0; p < npids; p++) {
        pid_t pid = pids[p];

        int bufsize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, NULL, 0);
        if (bufsize <= 0) continue;

        struct proc_fdinfo *fds = malloc(bufsize);
        if (!fds) continue;

        int actual = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds, bufsize);
        if (actual <= 0) {
            free(fds);
            continue;
        }
        int nfds = actual / (int)sizeof(struct proc_fdinfo);

        int has_socket = 0;
        for (int f = 0; f < nfds; f++) {
            if (fds[f].proc_fdtype == PROX_FDTYPE_SOCKET) {
                has_socket = 1;
                break;
            }
        }
        if (!has_socket) {
            free(fds);
            continue;
        }

        char pname[PROCINFO_NAME_MAX];
        int namelen = proc_name(pid, pname, sizeof(pname));
        if (namelen <= 0) {
            snprintf(pname, sizeof(pname), "pid:%d", pid);
        }

        for (int f = 0; f < nfds; f++) {
            if (fds[f].proc_fdtype != PROX_FDTYPE_SOCKET)
                continue;

            struct socket_fdinfo si;
            int ret = proc_pidfdinfo(pid, fds[f].proc_fd, PROC_PIDFDSOCKETINFO,
                                     &si, sizeof(si));
            if (ret <= 0) continue;

            int family = si.psi.soi_family;
            if (family != AF_INET && family != AF_INET6) continue;

            int proto = si.psi.soi_protocol;
            if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) continue;

            unsigned short lport = ntohs((unsigned short)si.psi.soi_proto.pri_in.insi_lport);
            if (lport == 0) continue;

            if (family == AF_INET) {
                struct in_addr laddr = si.psi.soi_proto.pri_in.insi_laddr.ina_46.i46a_addr4;
                table_add(staging_table, AF_INET, proto, lport, &laddr, pid, pname);
            } else {
                struct in6_addr laddr = si.psi.soi_proto.pri_in.insi_laddr.ina_6;
                table_add(staging_table, AF_INET6, proto, lport, &laddr, pid, pname);
            }
        }
        free(fds);
    }
    free(pids);
    table_publish();
}

/* ── Linux implementation ─────────────────────────────────────────── */

#elif defined(__linux__)

#include <dirent.h>

int procinfo_available(void) { return 1; }

typedef struct {
    unsigned long inode;
    int af;
    unsigned short protocol;
    unsigned short port;
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } addr;
} inode_entry;

static inode_entry *inode_table = NULL;
static int inode_count = 0;
static int inode_cap = 0;

static void inode_table_clear(void) {
    inode_count = 0;
}

static void inode_table_add(unsigned long inode, int af, unsigned short protocol,
                            unsigned short port, const void *addr) {
    if (inode_count >= inode_cap) {
        inode_cap = inode_cap ? inode_cap * 2 : 1024;
        inode_table = realloc(inode_table, inode_cap * sizeof(inode_entry));
        if (!inode_table) {
            inode_count = 0;
            inode_cap = 0;
            return;
        }
    }
    inode_entry *e = &inode_table[inode_count++];
    e->inode = inode;
    e->af = af;
    e->protocol = protocol;
    e->port = port;
    if (af == AF_INET) {
        memcpy(&e->addr.v4, addr, sizeof(struct in_addr));
    } else {
        memcpy(&e->addr.v6, addr, sizeof(struct in6_addr));
    }
}

static inode_entry *inode_table_find(unsigned long inode) {
    for (int i = 0; i < inode_count; i++) {
        if (inode_table[i].inode == inode)
            return &inode_table[i];
    }
    return NULL;
}

static void parse_proc_net(const char *path, int af, unsigned short protocol) {
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    char line[512];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        unsigned int local_addr_hex, remote_addr_hex;
        unsigned int local_port, remote_port;
        unsigned int state;
        unsigned long inode;

        if (af == AF_INET) {
            int matched = sscanf(line, " %*d: %x:%x %x:%x %x %*x:%*x %*x:%*x %*x %*u %*d %lu",
                                 &local_addr_hex, &local_port,
                                 &remote_addr_hex, &remote_port,
                                 &state, &inode);
            if (matched < 6) continue;
            if (inode == 0) continue;

            struct in_addr addr;
            addr.s_addr = local_addr_hex;
            inode_table_add(inode, AF_INET, protocol, (unsigned short)local_port, &addr);
        } else {
            char local_addr_str[33], remote_addr_str[33];
            int matched = sscanf(line, " %*d: %32[0-9A-Fa-f]:%x %32[0-9A-Fa-f]:%x %x %*x:%*x %*x:%*x %*x %*u %*d %lu",
                                 local_addr_str, &local_port,
                                 remote_addr_str, &remote_port,
                                 &state, &inode);
            if (matched < 6) continue;
            if (inode == 0) continue;

            struct in6_addr addr;
            for (int i = 0; i < 4; i++) {
                unsigned int word;
                char chunk[9];
                memcpy(chunk, local_addr_str + i * 8, 8);
                chunk[8] = '\0';
                sscanf(chunk, "%x", &word);
                memcpy(addr.s6_addr + i * 4, &word, 4);
            }
            inode_table_add(inode, AF_INET6, protocol, (unsigned short)local_port, &addr);
        }
    }
    fclose(fp);
}

static void get_proc_name(pid_t pid, char *buf, int bufsize) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (fgets(buf, bufsize, fp)) {
            char *nl = strchr(buf, '\n');
            if (nl) *nl = '\0';
        } else {
            snprintf(buf, bufsize, "pid:%d", pid);
        }
        fclose(fp);
    } else {
        snprintf(buf, bufsize, "pid:%d", pid);
    }
}

void procinfo_refresh(void) {
    staging_table = table_create();
    if (!staging_table) return;

    inode_table_clear();

    parse_proc_net("/proc/net/tcp",  AF_INET,  IPPROTO_TCP);
    parse_proc_net("/proc/net/tcp6", AF_INET6, IPPROTO_TCP);
    parse_proc_net("/proc/net/udp",  AF_INET,  IPPROTO_UDP);
    parse_proc_net("/proc/net/udp6", AF_INET6, IPPROTO_UDP);

    if (inode_count == 0) { table_publish(); return; }

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) { table_publish(); return; }

    struct dirent *proc_ent;
    while ((proc_ent = readdir(proc_dir)) != NULL) {
        char *endp;
        long pid = strtol(proc_ent->d_name, &endp, 10);
        if (*endp != '\0' || pid <= 0) continue;

        char fd_path[64];
        snprintf(fd_path, sizeof(fd_path), "/proc/%ld/fd", pid);
        DIR *fd_dir = opendir(fd_path);
        if (!fd_dir) continue;

        char pname[PROCINFO_NAME_MAX];
        int name_resolved = 0;

        struct dirent *fd_ent;
        while ((fd_ent = readdir(fd_dir)) != NULL) {
            char link_path[128];
            char link_target[128];
            snprintf(link_path, sizeof(link_path), "%s/%s", fd_path, fd_ent->d_name);

            ssize_t len = readlink(link_path, link_target, sizeof(link_target) - 1);
            if (len <= 0) continue;
            link_target[len] = '\0';

            unsigned long inode;
            if (sscanf(link_target, "socket:[%lu]", &inode) != 1) continue;

            inode_entry *ie = inode_table_find(inode);
            if (!ie) continue;

            if (!name_resolved) {
                get_proc_name((pid_t)pid, pname, sizeof(pname));
                name_resolved = 1;
            }

            table_add(staging_table, ie->af, ie->protocol, ie->port,
                      &ie->addr, (pid_t)pid, pname);
        }
        closedir(fd_dir);
    }
    closedir(proc_dir);
    table_publish();
}

/* ── Unsupported platform ─────────────────────────────────────────── */

#else

int procinfo_available(void) { return 0; }

void procinfo_refresh(void) {
    /* no-op */
}

#endif /* platform selection */

/* ── Shared (platform-independent) ────────────────────────────────── */

int procinfo_lookup(int address_family, const void *local_addr,
                    unsigned short local_port, unsigned short protocol,
                    proc_entry *out) {
    sock_table_t *t = atomic_load(&live_table);
    return table_find(t, address_family, local_addr, local_port, protocol, out);
}

void procinfo_destroy(void) {
    sock_table_t *t = atomic_exchange(&live_table, NULL);
    table_free(t);
    table_free(staging_table);
    staging_table = NULL;
#if defined(__linux__)
    free(inode_table);
    inode_table = NULL;
    inode_count = 0;
    inode_cap = 0;
#endif
}
