#ifndef _FS_CEPH_MON_CLIENT_H
#define _FS_CEPH_MON_CLIENT_H

#include <linux/completion.h>
#include <linux/radix-tree.h>

#include "messenger.h"
#include "msgpool.h"

struct ceph_client;
struct ceph_mount_args;

/*
 * The monitor map enumerates the set of all monitors.
 */
struct ceph_monmap {
	struct ceph_fsid fsid;
	u32 epoch;
	u32 num_mon;
	struct ceph_entity_inst mon_inst[0];
};

struct ceph_mon_client;
struct ceph_mon_statfs_request;


/*
 * Generic mechanism for resending monitor requests.
 */
typedef void (*ceph_monc_request_func_t)(struct ceph_mon_client *monc,
					 int newmon);

/* a pending monitor request */
struct ceph_mon_request {
	struct ceph_mon_client *monc;
	struct delayed_work delayed_work;
	unsigned long delay;
	ceph_monc_request_func_t do_request;
};

/*
 * statfs() is done a bit differently because we need to get data back
 * to the caller
 */
struct ceph_mon_statfs_request {
	u64 tid;
	int result;
	struct ceph_statfs *buf;
	struct completion completion;
	unsigned long last_attempt, delay; /* jiffies */
	struct ceph_msg *request;  /* original request */
};

struct ceph_mon_client {
	struct ceph_client *client;
	struct ceph_monmap *monmap;

	struct mutex mutex;
	struct delayed_work delayed_work;

	bool hunting;
	int cur_mon;                       /* last monitor i contacted */
	unsigned long sub_sent, sub_renew_after;
	struct ceph_connection *con;

	/* msg pools */
	struct ceph_msgpool msgpool_mount_ack;
	struct ceph_msgpool msgpool_subscribe_ack;
	struct ceph_msgpool msgpool_statfs_reply;

	/* pending statfs requests */
	struct radix_tree_root statfs_request_tree;
	int num_statfs_requests;
	u64 last_tid;

	/* mds/osd map or mount requests */
	bool want_mount;
	int want_next_osdmap; /* 1 = want, 2 = want+asked */
	u32 have_osdmap, have_mdsmap;

	struct dentry *debugfs_file;
};

extern struct ceph_monmap *ceph_monmap_decode(void *p, void *end);
extern int ceph_monmap_contains(struct ceph_monmap *m,
				struct ceph_entity_addr *addr);

extern int ceph_monc_init(struct ceph_mon_client *monc, struct ceph_client *cl);
extern void ceph_monc_stop(struct ceph_mon_client *monc);

/*
 * The model here is to indicate that we need a new map of at least
 * epoch @want, and also call in when we receive a map.  We will
 * periodically rerequest the map from the monitor cluster until we
 * get what we want.
 */
extern int ceph_monc_got_mdsmap(struct ceph_mon_client *monc, u32 have);
extern int ceph_monc_got_osdmap(struct ceph_mon_client *monc, u32 have);

extern void ceph_monc_request_next_osdmap(struct ceph_mon_client *monc);

extern int ceph_monc_request_mount(struct ceph_mon_client *monc);

extern int ceph_monc_do_statfs(struct ceph_mon_client *monc,
			       struct ceph_statfs *buf);



#endif