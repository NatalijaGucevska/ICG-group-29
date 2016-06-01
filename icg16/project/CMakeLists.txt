# the exercise name is nothing else than the directory
get_filename_component(EXERCISENAME ${CMAKE_CURRENT_LIST_DIR} NAME)
file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

# these files are copied only once
file(GLOB TEXTURES
    grid/grid_texture.tga
    water/normalmap.png
    water/dudv.png
    water/alpha_water.tga
    grid/rock.tga
    grid/sand.tga
    grid/snow.tga
    grid/grass.tga
    grid/alpha.tga
    cube/down.jpg
    cube/up.jpg
    cube/front.jpg
    cube/back.jpg
    cube/right.jpg
    cube/left.jpg)
copy_files_once(${TEXTURES})

# list all the files you want to copy everytime
# you build (ie, you want the shaders there)
file(GLOB SHADERS
  screenquad/screenquad_vshader.glsl
  screenquad/screenquad_fshader.glsl
  water/water_vshader.glsl
  water/water_fshader.glsl
  grid/grid_vshader.glsl
  grid/grid_fshader.glsl
  cube/cube_vshader.glsl
  cube/cube_fshader.glsl)
deploy_shaders_to_build_dir(${SHADERS})

add_executable(${EXERCISENAME} ${SOURCES} ${HEADERS} ${SHADERS})
target_link_libraries(${EXERCISENAME} ${COMMON_LIBS})
