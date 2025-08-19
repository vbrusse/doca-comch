#
# Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of NVIDIA CORPORATION &
# AFFILIATES (the "Company") and all right, title, and interest in and to the
# software product, including all associated intellectual property rights, are
# and shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#

#
# Filename: meson.build - configuration file for "The Meson Build System"
#
# Customized by: Vlademir Brusse
#
# Date: 2024/12/10
#

project('vDU_LDPC_CODES', 'C', 'CPP',
        # Get version number from file.
        version: run_command(find_program('cat'),
                files('/opt/mellanox/doca/applications/VERSION'), check: true).stdout().strip(),
        license: 'Proprietary',
        default_options: ['buildtype=debug'],
        meson_version: '>= 0.61.2'
)

TEST_NAME = 'vdu_high_phy_ldpc_codes'

# Comment this line to restore warnings of experimental DOCA features
add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c', 'cpp'])

# Define the shared library path
LIBRARY_PATH = '/tmp/build/libldpc_armral.so'

# Define the external library as a dependency
ldpc_armral_dep = declare_dependency(
    link_args: ['-L/tmp/build', '-lldpc_armral'],
    include_directories: include_directories('/tmp/build')
)

test_dependencies = []
# Required for all DOCA programs
#sample_dependencies += dependency('doca-common')
# The DOCA library of the sample itself
#sample_dependencies += dependency('doca-comch')
# Utility DOCA library for executables
#sample_dependencies += dependency('doca-argp')
test_dependencies += dependency('threads')

test_srcs = [
        # The sample itself
        TEST_NAME + '.c',
        # Main function for the sample's executable
        #SAMPLE_NAME + '.c',
        # Common code for the DOCA library samples
        #'../comch_ctrl_path_common.c',
        #'../nrLDPC_common.c',
        # Common code for all DOCA samples
        #'../../common.c',
]

test_inc_dirs  = []
# Common DOCA library logic
test_inc_dirs += include_directories('..')
# Common DOCA logic (samples)
#sample_inc_dirs += include_directories('../..')
# Common DOCA logic
#sample_inc_dirs += include_directories('../../..')
# Common DOCA logic (applications)
#sample_inc_dirs += include_directories('../../../applications/common/')
# libldpc_armral.so
test_inc_dirs += include_directories('/tmp/build')

executable(TEST_NAME, test_srcs,
    c_args : '-Wno-missing-braces',
    dependencies : [test_dependencies, ldpc_armral_dep],
    include_directories : test_inc_dirs,
    install : false,
    install_rpath : '/tmp/build',
)
