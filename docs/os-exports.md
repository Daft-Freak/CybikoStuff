# Info about CyOS exports that nobody asked for

## asserts

Base class for a bunch of things, also used for mostly disabled logging.

```c++
struct asserts
{
    void *vtable;
};
```

`__cyos 322 void *asserts_vtable`

`__cyos 432 char *asserts_class_name(asserts *)`

Returns "asserts::".

## Globals

`__cyos 260 delete`

`__cyos 261 new`

`__cyos 262 delete[]`

`__cyos 263 new[]`

Wrappers around `malloc`/`free`, the way they're used suggests that one pair is for arrays.

`__cyos 264 long sdiv32(long, long)`

Signed 32-bit divide.

`__cyos 267 os_entry`

The OS entrypoint.

`__cyos 269 unsigned long umul32(unsigned long, unsigned long)`

32-bit multiply.

`__cyos 272 unsigned long udiv32(unsigned long, unsigned long)`

Unsigned 32-bit divide.

`__cyos 273 unsigned long umod32(unsigned long, unsigned long)`



## More...


```
__cyos 303 File_vtable
__cyos 304 Flag_vtable
__cyos 308 Mutex_vtable
__cyos 312 Buffer_vtable
__cyos 313 Flagex_vtable

__cyos 86 BlockMapper_dtor
__cyos 177 BlockMapper_ctor
__cyos 279 BlockMapper_vtable
__cyos 421 BlockMapper_class_name
__cyos 437 BlockMapper_clear
__cyos 706 BlockMapper_lookup
__cyos 775 BlockMapper_init
__cyos 819 BlockMapper_is_reserved
__cyos 877 BlockMapper_set_mapping

__cyos 97 Console_dtor
__cyos 194 Console_ctor

__cyos 100 CommandStringParser_dtor
__cyos 203 CommandStringParser_ctor
__cyos 936 CommandStringParser_parse

__cyos 48 FAT_lock_fat
__cyos 82 FAT_unlock_fat
__cyos 105 FAT_dtor
__cyos 216 FAT_ctor
__cyos 378 FAT_get_bad_blocks
__cyos 382 FAT_clear_other_cache
__cyos 383 FAT_is_valid_block_ind
__cyos 394 FAT_build
__cyos 429 T_FAT_class_name
__cyos 455 FAT_is_remapped_block
__cyos 457 FAT_is_block_free
__cyos 462 FAT_get_block_size
__cyos 463 FAT_get_total_blocks
__cyos 464 FAT_get_used_blocks
__cyos 504 FAT_create_file
__cyos 565 FAT_find_free_block
__cyos 583 FAT_get_total_files
__cyos 615 FAT_get_free_blocks
__cyos 806 FAT_is_correct
__cyos 878 FAT_add_remapped_block
__cyos 991 FAT_readblock
__cyos 993 FAT_readblockshort
__cyos 1253 FAT_get_usable_disk_blocks
__cyos 1286 FAT_writeblock

__cyos 197 Hourglass_ctor

__cyos 191 MemMgr_ctor

__cyos 121 Messenger_dtor
__cyos 254 Messenger_ctor
__cyos 375 Messenger_add_to_pool
__cyos 888 messenger_ptr
__cyos 900 Messenger_get_from_pool
__cyos 944 Messenger_post
__cyos 1370 Messenger_get_pool_count

__cyos 108 Module_dtor
__cyos 224 Module_ctor
__cyos 225 Module_ctor_Ex2
__cyos 226 Module_ctor_Ex
__cyos 314 Module_vtable
__cyos 494 Module_decrement_usage
__cyos 554 Module_execute
__cyos 667 Module_lookup_export
__cyos 715 Module_get_retcode
__cyos 747 Module_get_usage
__cyos 769 Module_increment_usage
__cyos 824 Module_is_valid
__cyos 847 Module_load
__cyos 848 Module_load_input
__cyos 859 Module_load_executable
__cyos 917 Module_open_resource_name
__cyos 918 Module_open_resource
__cyos 1024 Module_apply_relocations
__cyos 1174 Module_create_process

__cyos 187 MutexCounter_ctor
__cyos 288 MutexCounter_vtable
__cyos 864 MutexCounter_lock
__cyos 1259 MutexCounter_release

__cyos 255 NameBlock_ctor
__cyos 340 NameBlock_vtable

__cyos 122 Process_dtor
__cyos 259 Process_ctor
__cyos 343 Process_vtable
__cyos 666 Process_get_display
__cyos 1050 Process_run

__cyos 118 Receiver_dtor
__cyos 248 Receiver_ctor

__cyos 93 SystemThread_dtor
__cyos 188 SystemThread_ctor
__cyos 289 SystemThread_vtable
__cyos 802 SystemThread_is_active
__cyos 820 SystemThread_is_started

__cyos 89 TCacheBlock_dtor
__cyos 184 TCacheBlock_ctor
__cyos 283 TCacheBlock_vtable
__cyos 392 TCacheBlock_get_size
__cyos 423 TCacheBlock_class_name
__cyos 603 TCacheBlock_clear
__cyos 656 TCacheBlock_get_clust
__cyos 658 TCacheBlock_get_clust_head
__cyos 780 TCacheBlock_init
__cyos 970 TCacheBlock_put_clust
__cyos 983 TCacheBlock_read
__cyos 1266 TCacheBlock_clear_if_current
__cyos 1246 TCacheBlock_test_mutex

__cyos 230 Thread_ctor_Ex
__cyos 231 Thread_ctor
__cyos 906 Thread_update_times

__cyos 246 Queue_ctor
__cyos 364 Queue_add_message
__cyos 625 Queue_pop_message
__cyos 663 Queue_get_count
__cyos 979 Queue_put_message_internal
```
