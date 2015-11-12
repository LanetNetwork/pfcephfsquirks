/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/*
 * Copyright 2015 Lanet Network
 * Programmed by Oleksandr Natalenko <o.natalenko@lanet.ua>
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

#pragma once

#ifndef __PFCFSQ_H__
#define __PFCFSQ_H__

typedef void (*cfs_dentry_handler_t)(struct ceph_mount_info*, const char*, struct dirent*, struct stat*, void*, unsigned int);
typedef int (*cfs_dentry_comparator_t)(const char*, struct dirent*, struct stat*, unsigned int);

struct ceph_mount_info* cfs_mount(const char* _mons, const char* _id, const char* _keyring_file, const char* _root) __attribute__((nonnull(1, 2, 3, 4)));
int cfs_unmount(struct ceph_mount_info* _fs) __attribute__((nonnull(1)));
int cfs_mkdir_safe(struct ceph_mount_info* _fs, const char* _path, mode_t _mode);
void cfs_walk_dir_generic(struct ceph_mount_info* _fs, const char* _entry_point, cfs_dentry_handler_t _handler, cfs_dentry_comparator_t _comparator, void* _data, unsigned int _level) __attribute__((nonnull(1, 2, 3)));

#endif /* __PFGFQ_H__ */

