tools/db_sanity_test.d tools/db_sanity_test.o: tools/db_sanity_test.cc \
  include/rocksdb/db.h include/rocksdb/metadata.h \
  include/rocksdb/types.h include/rocksdb/version.h \
  include/rocksdb/iterator.h include/rocksdb/slice.h \
  include/rocksdb/status.h include/rocksdb/options.h \
  include/rocksdb/listener.h include/rocksdb/universal_compaction.h \
  include/rocksdb/transaction_log.h include/rocksdb/write_batch.h \
  include/rocksdb/write_batch_base.h include/rocksdb/thread_status.h \
  include/rocksdb/env.h include/rocksdb/comparator.h \
  include/rocksdb/table.h include/rocksdb/immutable_options.h \
  include/rocksdb/slice_transform.h include/rocksdb/filter_policy.h \
  port/port.h port/port_posix.h util/string_util.h
