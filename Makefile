TOP ?= ${HOME}

CXXFLAGS = -std=c++17 -g -Og -I ${TOP}/rapidjson/include -I ${TOP}/abseil-cpp
LDFLAGS = -L ${TOP}/abseil-cpp/build/absl/flags  \
  -L ${TOP}/abseil-cpp/build/absl/container \
  -L ${TOP}/abseil-cpp/build/absl/strings \
  -L ${TOP}/abseil-cpp/build/absl/synchronization \
  -L ${TOP}/abseil-cpp/build/absl/time \
  -L ${TOP}/abseil-cpp/build/absl/base  \
  -L ${TOP}/abseil-cpp/build/absl/debugging  \
  -L ${TOP}/abseil-cpp/build/absl/hash  \
  -L ${TOP}/abseil-cpp/build/absl/numeric  \

LDLIBS = -labsl_flags -labsl_flags_internal -labsl_flags_reflection \
  -labsl_flags_config \
  -labsl_flags_commandlineflag \
  -labsl_flags_commandlineflag_internal \
  -labsl_flags_marshalling \
  -labsl_flags_program_name \
  -labsl_flags_parse \
  -labsl_flags_usage \
  -labsl_flags_usage_internal \
  -labsl_str_format_internal \
  -labsl_hash \
  -labsl_city \
  -labsl_low_level_hash \
  -labsl_raw_hash_set -labsl_strings \
  -labsl_raw_logging_internal -labsl_throw_delegate \
  -labsl_synchronization -labsl_graphcycles_internal \
  -labsl_time -labsl_time_zone \
  -labsl_symbolize \
  -labsl_stacktrace \
  -labsl_debugging_internal \
  -labsl_demangle_internal \
  -labsl_base -labsl_spinlock_wait -labsl_malloc_internal \
  -labsl_flags_private_handle_accessor \
  -labsl_int128 \

PROGS = gtorusgen gcycle genstates shannon shannon2

.PHONY: all clean

all: ${PROGS}
clean:
	rm -f *.o ${PROGS}


${PROGS} : glife.h
