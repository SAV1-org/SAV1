project('sav1', ['c', 'cpp'],
  version : '0.9.1',
  default_options : ['warning_level=3', 'buildtype=release', 'debug=false', 'cpp_std=c++11', 
  'strip=true']) # STRIP=TRUE strips the debug symbol table, a very significant size saving for builds.

inc_dirs = []
inc_dirs += include_directories('include')
inc_dirs += include_directories('dependencies/libwebm/webm_parser/include')
inc_dirs += include_directories('dependencies/libwebm/webm_parser') # so it can see .h files in that directory
inc_dirs += include_directories('dependencies/libyuv/include')
inc_dirs += include_directories('dependencies/thread')
if get_option('use_subprojects')
  inc_dirs += include_directories('subprojects/dav1d/include') #this could theoretically be a bug in dav1d buildconfig, they include a include in their dav1d_dep statement, maybe the rel path is wrong?
endif
if build_machine.system() == 'windows'
  inc_dirs += include_directories('dependencies/x64-windows/include')
endif

project_source_files = [
  'src/sav1.c',
  'src/custom_processing_audio.c',
  'src/custom_processing_video.c',
  'src/decode_av1.c',
  'src/decode_opus.c',
  'src/sav1_audio_frame.c',
  'src/sav1_internal.c',
  'src/sav1_settings.c',
  'src/sav1_video_frame.c',
  'src/thread_manager.c',
  'src/thread_queue.c',
  'src/webm_frame.c',
  'src/convert_av1.cpp',
  'src/parse.cpp'
]

webm_source_files = [
  'dependencies/libwebm/webm_parser/src/ancestory.cc',
  'dependencies/libwebm/webm_parser/src/bit_utils.cc',
  'dependencies/libwebm/webm_parser/src/block_header_parser.cc',
  'dependencies/libwebm/webm_parser/src/block_parser.cc',
  'dependencies/libwebm/webm_parser/src/buffer_reader.cc',
  'dependencies/libwebm/webm_parser/src/callback.cc',
  'dependencies/libwebm/webm_parser/src/date_parser.cc',
  'dependencies/libwebm/webm_parser/src/file_reader.cc',
  'dependencies/libwebm/webm_parser/src/float_parser.cc',
  'dependencies/libwebm/webm_parser/src/id_element_parser.cc',
  'dependencies/libwebm/webm_parser/src/id_parser.cc',
  'dependencies/libwebm/webm_parser/src/istream_reader.cc',
  'dependencies/libwebm/webm_parser/src/master_parser.cc',
  'dependencies/libwebm/webm_parser/src/parser_utils.cc',
  'dependencies/libwebm/webm_parser/src/segment_parser.cc',
  'dependencies/libwebm/webm_parser/src/size_parser.cc',
  'dependencies/libwebm/webm_parser/src/skip_parser.cc',
  'dependencies/libwebm/webm_parser/src/unknown_parser.cc',
  'dependencies/libwebm/webm_parser/src/var_int_parser.cc',
  'dependencies/libwebm/webm_parser/src/virtual_block_parser.cc',
  'dependencies/libwebm/webm_parser/src/void_parser.cc',
  'dependencies/libwebm/webm_parser/src/webm_parser.cc'
]

# Build webm as separate lib to silence warnings from it: since it's vendored
# the warnings are not useful.
webm_lib = static_library(
  'webm',
  webm_source_files,
  include_directories: inc_dirs,
  override_options: ['warning_level=0']
)

yuv_source_files = [
  'dependencies/libyuv/source/compare.cc',
  'dependencies/libyuv/source/compare_common.cc',
  'dependencies/libyuv/source/compare_gcc.cc',
  'dependencies/libyuv/source/compare_mmi.cc', 
  'dependencies/libyuv/source/compare_msa.cc', 
  'dependencies/libyuv/source/compare_neon.cc', 
  'dependencies/libyuv/source/compare_neon64.cc',
  'dependencies/libyuv/source/compare_win.cc',
  'dependencies/libyuv/source/convert.cc',
  'dependencies/libyuv/source/convert_argb.cc',
  'dependencies/libyuv/source/convert_from.cc',
  'dependencies/libyuv/source/convert_from_argb.cc',
  'dependencies/libyuv/source/convert_jpeg.cc',
  'dependencies/libyuv/source/convert_to_argb.cc',
  'dependencies/libyuv/source/convert_to_i420.cc',
  'dependencies/libyuv/source/cpu_id.cc',
  'dependencies/libyuv/source/mjpeg_decoder.cc',
  'dependencies/libyuv/source/mjpeg_validate.cc',
  'dependencies/libyuv/source/planar_functions.cc',
  'dependencies/libyuv/source/rotate.cc',
  'dependencies/libyuv/source/rotate_any.cc',
  'dependencies/libyuv/source/rotate_argb.cc',
  'dependencies/libyuv/source/rotate_common.cc',
  'dependencies/libyuv/source/rotate_gcc.cc',
  'dependencies/libyuv/source/rotate_lsx.cc',
  'dependencies/libyuv/source/rotate_mmi.cc',
  'dependencies/libyuv/source/rotate_msa.cc',
  'dependencies/libyuv/source/rotate_neon.cc',
  'dependencies/libyuv/source/rotate_neon64.cc',
  'dependencies/libyuv/source/rotate_win.cc',
  'dependencies/libyuv/source/row_any.cc',
  'dependencies/libyuv/source/row_common.cc',
  'dependencies/libyuv/source/row_gcc.cc',
  'dependencies/libyuv/source/row_lasx.cc',
  'dependencies/libyuv/source/row_lsx.cc',
  'dependencies/libyuv/source/row_mmi.cc',
  'dependencies/libyuv/source/row_msa.cc',
  'dependencies/libyuv/source/row_neon.cc',
  'dependencies/libyuv/source/row_neon64.cc',
  'dependencies/libyuv/source/row_win.cc',
  'dependencies/libyuv/source/scale.cc',
  'dependencies/libyuv/source/scale_any.cc',
  'dependencies/libyuv/source/scale_argb.cc',
  'dependencies/libyuv/source/scale_common.cc',
  'dependencies/libyuv/source/scale_gcc.cc',
  'dependencies/libyuv/source/scale_lsx.cc',
  'dependencies/libyuv/source/scale_mmi.cc',
  'dependencies/libyuv/source/scale_msa.cc',
  'dependencies/libyuv/source/scale_neon.cc',
  'dependencies/libyuv/source/scale_neon64.cc',
  'dependencies/libyuv/source/scale_rgb.cc',
  'dependencies/libyuv/source/scale_uv.cc',
  'dependencies/libyuv/source/scale_win.cc',
  'dependencies/libyuv/source/video_common.cc'
]

# Build yuv as separate lib to silence warnings from it: since it's vendored
# the warnings are not useful.
yuv_lib = static_library(
  'yuv',
  yuv_source_files,
  include_directories: inc_dirs,
  override_options: ['warning_level=0']
)

c = meson.get_compiler('c')
dir_base = meson.current_source_dir()
deps = []

if get_option('use_subprojects')
  opus_proj = subproject('opus', default_options: 'default_library=static')
  deps += opus_proj.get_variable('opus_dep')

  dav1d_proj = subproject('dav1d', default_options: ['default_library=static', 'enable_tools=False'])
  deps += dav1d_proj.get_variable('dav1d_dep')

elif build_machine.system() == 'windows'
  deps += c.find_library('opus', required: true, dirs: join_paths(dir_base, 'dependencies/x64-windows/bin'))
  deps += c.find_library('dav1d', required: true, dirs: join_paths(dir_base, 'dependencies/x64-windows/bin'))

else
  deps += c.find_library('opus', required: true)
  deps += c.find_library('dav1d', required: true)
endif

# Add thread library dependencies for Windows and Linux
if build_machine.system() == 'windows'
  deps += c.find_library('winmm', required: true) # for thread.h on windows
endif
if build_machine.system() == 'linux'
  deps += c.find_library('pthread', required: true)
endif

if build_machine.system() == 'windows'
  sav1_name_prefix = ''
else
  sav1_name_prefix = 'lib'
endif

sav1_lib = library('sav1',
                   project_source_files,
                   include_directories: inc_dirs,
                   dependencies: deps,
                   link_with: [webm_lib, yuv_lib],
                   name_prefix: sav1_name_prefix,
                   install: true,
                   install_dir: dir_base)

sav1play_includes = [include_directories('include')]
if build_machine.system() == 'windows'
  sav1play_includes += include_directories('dependencies/x64-windows/include')
endif

sav1play_deps = []
if build_machine.system() == 'windows'
  sav1play_deps += c.find_library('SDL2', required: true, dirs: join_paths(dir_base, 'dependencies/x64-windows/bin'))
else
  sav1play_deps += c.find_library('SDL2', required: true)
endif

executable('sav1play', 'examples/sav1play.c', 
            include_directories: sav1play_includes,
            dependencies: sav1play_deps,
            link_with: sav1_lib,
            install: true, install_dir: dir_base)

executable('sav1slideshow', 'examples/sav1slideshow.c', 
            include_directories: sav1play_includes,
            dependencies: sav1play_deps,
            link_with: sav1_lib,
            install: true, install_dir: dir_base)
