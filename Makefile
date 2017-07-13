# 
# 1. General Compiler Settings
#
CXX       = g++
CXXFLAGS  = -std=c++14 -Wall -Wextra -Wcast-qual -Wno-unused-function -Wno-sign-compare -Wno-unused-value -Wno-unused-label -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -fno-rtti \
            -pedantic -Wno-long-long -msse4.2 -mbmi -mbmi2 -mavx2 -D__STDC_CONSTANT_MACROS -fopenmp
INCLUDES  =
LIBRARIES = -lpthread

#
# 2. Target Specific Settings
#
ifeq ($(TARGET),match)
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM -DMATCH
        output_dir := out/match/
endif
ifeq ($(TARGET),release)
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM
        output_dir := out/release/
endif
ifeq ($(TARGET),debug)
	CXXFLAGS += -O0 -g -ggdb -DDEBUG -DBROADCAST -D_GLIBCXX_DEBUG
        output_dir := out/debug/
endif
ifeq ($(TARGET),default)
	CXXFLAGS += -Ofast -g -ggdb -fno-fast-math
        output_dir := out/default/
endif

#
# 2. Default Settings (applied if there is no target-specific settings)
#
sources      ?= $(shell ls -R src/*.cc)
sources_dir  ?= src/
objects      ?=
directories  ?= $(output_dir)

#
# 4. Public Targets
#
default release debug:
	$(MAKE) TARGET=$@ preparation image_log_maker linear_policy_learner count_alpha_3x3

match:
	$(MAKE) TARGET=$@ preparation gtp_client nngs_client

clean:
	rm -rf out/*

#
# 5. Private Targets
#
preparation $(directories):
	mkdir -p $(directories)

image_log_maker :
	$(CXX) $(CXXFLAGS) -o $(output_dir)image_log_maker $(sources_dir)test/image_log_maker.cc $(LIBRARIES)

linear_policy_learner :
	$(CXX) $(CXXFLAGS) -o $(output_dir)linear_policy_learner $(sources_dir)julie/linear_policy_learner.cc $(LIBRARIES)

gtp_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)gtp_client $(sources_dir)main_gtp.cc $(LIBRARIES)

nngs_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)nngs_client $(sources_dir)main_nngs.cc $(LIBRARIES)

nngs_server :
	$(CXX) $(CXXFLAGS) -o $(output_dir)nngs_server $(sources_dir)server/server_nngs.cc $(LIBRARIES)

count_alpha_3x3 :
	$(CXX) $(CXXFLAGS) -o $(output_dir)count_alpha_3x3 $(sources_dir)test/count_alpha_3x3.cc $(LIBRARIES)

-include $(dependencies)