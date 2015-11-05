/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/*
 * Copyright 2015 Lanet Network
 * Programmed by Oleksandr Natalenko <o.natalenko@lanet.ua>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <libcephfs.h>
#include <linux/limits.h>
#include <pfcq.h>
#include <pfcfsq.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t cfs_mkdir_safe_lock = PTHREAD_MUTEX_INITIALIZER;

struct ceph_mount_info* cfs_mount(const char* _mons, const char* _id, const char* _keyring_file, const char* _root)
{
	struct ceph_mount_info* ret = NULL;

	if (unlikely(ceph_create(&ret, _id) != 0))
		goto out;

	if (unlikely(ceph_conf_set(ret, "mon_host", _mons) != 0))
		goto err_release;

	if (unlikely(ceph_conf_set(ret, "keyring", _keyring_file) != 0))
		goto err_release;

	if (unlikely(ceph_init(ret) != 0))
		goto err_release;

	if (unlikely(ceph_mount(ret, _root) != 0))
		goto err_release;

	goto out;

err_release:
	ceph_release(ret);
	ret = NULL;

out:
	return ret;
}

int cfs_unmount(struct ceph_mount_info* _fs)
{
	int ret = -1;

	if (unlikely(ceph_unmount(_fs) != 0))
		return ret;

	if (unlikely(ceph_release(_fs) != 0))
		return ret;

	return 0;
}

int cfs_mkdir_safe(struct ceph_mount_info* _fs, const char* _path, mode_t _mode)
{
	int ret = 0;
	int ceph_opendir_res = -1;
	struct ceph_dir_result* cfs_dir = NULL;

	if (unlikely(pthread_mutex_lock(&cfs_mkdir_safe_lock)))
		panic("pthread_mutex_lock");
	ceph_opendir_res = ceph_opendir(_fs, _path, &cfs_dir);
	if (unlikely(ceph_opendir_res != 0))
		ret = ceph_mkdir(_fs, _path, _mode);
	else
		ceph_closedir(_fs, cfs_dir);
	if (unlikely(pthread_mutex_unlock(&cfs_mkdir_safe_lock)))
		panic("pthread_mutex_unlock");

	return ret;
}

void walk_dir_generic(struct ceph_mount_info* _fs, const char* _entry_point, dentry_handler_t _handler, dentry_comparator_t _comparator, void* _data, unsigned int _level)
{
	int ceph_opendir_res = -1;
	int ceph_readdirplus_r_res = 0;
	int sb_mask = 0;
	char path[PATH_MAX];
	struct ceph_dir_result* cfs_dir = NULL;
	struct dirent entry;
	struct stat sb;

	pfcq_zero(&entry, sizeof(struct dirent));
	pfcq_zero(&sb, sizeof(struct stat));
	pfcq_zero(path, PATH_MAX);

	debug("Walking into %s entry...\n", _entry_point);

	ceph_opendir_res = ceph_opendir(_fs, _entry_point, &cfs_dir);
	if (unlikely(ceph_opendir_res != 0))
	{
		warning("ceph_opendir");
		goto out;
	}

	while (likely((ceph_readdirplus_r_res = ceph_readdirplus_r(_fs, cfs_dir, &entry, &sb, &sb_mask)) == 1))
	{
		if (unlikely(strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0))
			continue;

		if (likely(_comparator))
			if (unlikely(!_comparator(_entry_point, &entry, &sb, _level)))
				continue;

		if (likely(strcmp(_entry_point, "/") == 0))
		{
			if (unlikely(snprintf(path, PATH_MAX, "/%s", entry.d_name) < 0))
				continue;
		} else
		{
			if (unlikely(snprintf(path, PATH_MAX, "%s/%s", _entry_point, entry.d_name) < 0))
				continue;
		}
		if (likely(_handler))
			_handler(_fs, path, &entry, &sb, _data, _level);
	}

	if (unlikely(ceph_closedir(_fs, cfs_dir) != 0))
		warning("ceph_closedir");

out:
	debug("Walking out of %s entry...\n", _entry_point);

	return;
}

