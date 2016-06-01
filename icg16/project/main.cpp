// glew must be before glfw
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// contains helper functions such as shader compiler
#include "icg_helper.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

float TERRAIN_MAX_X = 64.f;
float CUBE_SIZE = 256.f;

#include "screenquad/screenquad.h"
#include "grid/grid.h"
#include "water/water.h"
#include "framebuffer.h"

#include "trackball.h"
#include "cube/cube.h"

Grid grid;
Water water;
ScreenQuad height_map;
FrameBuffer fb_height_map;
FrameBuffer water_map;

int window_width = 1024;
int window_height = 768;
double last_pos = 0.0;

using namespace glm;

mat4 projection_matrix;
mat4 view_matrix;

size_t TEXTURE_SIZE = 4096;


//------CAM------
// cam speed controls
float cam_mov_speed = 0;  // "amount/speed"
float cam_rotate_speed = 0;
float cam_jetpack_speed = 0;

float cam_mov_d_speed = 0.00005;//1.05f;
float cam_rotate_d_speed = 0.005f;
float cam_jetpack_d_speed = 0.001f;


//float cam_mov_backward_d_speed = 0.001;  // extra....not really useful
//float cam_rotate_negative_d_speed = 0.0005; // extra  ''    ''     ''

// cam move controls
bool cam_mov_forward;
bool up_down_rotation = false;
bool cam_mov_engine_on = false;  // true when W/S pressed => must increase speed, false when W/S released => decrease speed
bool cam_rotate_yaw; // true if yaw, false if pitch, no roll for now
bool cam_rotate_positive;   // "direction" of rotation, positive if true negative else
bool cam_rotate_engine_on = false;

bool cam_jetpack_engine_on = false;
bool cam_jetpack_up;

bool fps = true;
bool bezier = false;
float t_bezier = 0.0f;
float dt_bezier = 0.001;

// cam position controls
vec3 cam_pos, cam_look /**used only to init, maybe simplify*/, cam_up, cam_direction;
vec3 mirror_cam_direction, mirror_cam_look, mirror_cam_pos, mirror_cam_up;
mat4 mirror_view;

vec3 a,b,c,d,look_at_bezier;
//----------------

mat4 trackball_matrix;
mat4 old_trackball_matrix;

mat4 terrain_model_matrix, cube_model_matrix;
mat4 water_model_matrix;
float waterheight = 0.f;

Trackball trackball;
Cube cube;

bool enableKeyCallback = true;

void refresh() {
    fb_height_map.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    height_map.Draw();
    fb_height_map.Unbind();
    glViewport(0, 0, window_width, window_height);

    float time = glfwGetTime();

    mirror_view = lookAt(mirror_cam_pos,
                         mirror_cam_look,
                         cam_up);

    water_map.Bind();  // TODO move into refresh ?
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        grid.Draw(time,terrain_model_matrix, mirror_view, projection_matrix, 1, 0.f);
        cube.Draw(cube_model_matrix*translate(IDENTITY_MATRIX, vec3(cam_pos.x, 0, cam_pos.z)), mirror_view, projection_matrix);
    water_map.Unbind();
    glViewport(0, 0, window_width, window_height);

    enableKeyCallback = true;
}

float getTerrainHeight(float x, float y) {
    //cout << "x " << x << endl;
    float translated_x = TEXTURE_SIZE * (x + TERRAIN_MAX_X)/ (TERRAIN_MAX_X *2.f);
    float translated_y = TEXTURE_SIZE *(y + TERRAIN_MAX_X)/ (TERRAIN_MAX_X *2.f);

    fb_height_map.glBind();
    GLfloat data;
    glReadPixels(translated_x, TEXTURE_SIZE - translated_y, 1, 1, GL_RED, GL_FLOAT, &data);
    fb_height_map.Unbind();
    glViewport(0, 0, window_width, window_height);
    data = (data > 0) ? data + 0.5f : 0.5f;
    return data;
}

void cam_rotate() {
    vec3 axis = cam_rotate_yaw ? cam_up : cross(cam_up, cam_direction);
    float sens = cam_rotate_positive ? 1.f : -1.f;
    cam_direction = glm::rotate(cam_direction, sens * cam_rotate_speed, axis);

    if(!cam_rotate_yaw) { // pitch 'up/down'
        mirror_cam_direction = glm::rotate(mirror_cam_direction, -sens * cam_rotate_speed, axis);
    } else {
        mirror_cam_direction = glm::rotate(mirror_cam_direction, sens * cam_rotate_speed, axis);
    }

//    if((up_down_rotation || cam_rotate_speed > 0) && !cam_rotate_yaw) {
//        mirror_cam_direction = glm::rotate(mirror_cam_direction, -sens * cam_rotate_speed, axis);
//    } else {
//        mirror_cam_direction = glm::rotate(mirror_cam_direction, sens * cam_rotate_speed, axis);;
//    }
}

void cam_move() {
    float offset_x = (cam_direction.x > 0 && cam_pos.y <= 0.2) ? 0.2 : 0;
    float offset_y = (cam_direction.z > 0 && cam_pos.y <= 0.2) ? 0.2 : 0;
    float sens = cam_mov_forward ? -1.f : 1.f;

    // TODO maybe move this logic into display() like bezier ?
    if (fps) {
        cam_pos.y = getTerrainHeight(cam_pos.x + offset_x, cam_pos.z + offset_y);
    } else {
        float sens = cam_jetpack_up ? 1.f : -1.f;
        cam_pos.y += sens * cam_jetpack_speed;
    }

    mirror_cam_pos.y = -cam_pos.y + waterheight;
    cam_look = cam_pos + cam_direction;
    mirror_cam_look = mirror_cam_pos + mirror_cam_direction;

    vec3 direction = normalize(cam_direction) * sens * cam_mov_speed;
    vec3 mirror_direction = normalize(mirror_cam_direction) * sens * cam_mov_speed;

    //cout << mirror_direction.x << "," << mirror_direction.y << endl;

    grid.move(-direction.x, direction.z);
    height_map.move(-direction.x, direction.z);
    water.move(-direction.x, direction.z);
    refresh();
}


void cam_bezier(float time, vec3 point1, vec3 point2, vec3 point3, vec3 point4, vec3 look_at_settings) {

    float t2 = time * time;
    float t3 = t2 * time;
    float mt = 1-time;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;

    float offset_x_= (point1*mt3 + 3.0f*point2*mt2*time + 3.0f*point3*mt*t2 + point4*t3).x;
    float offset_y_ = (point1*mt3 + 3.0f*point2*mt2*time + 3.0f*point3*mt*t2 + point4*t3).z;

    //cout << offset_x_ << ", " << offset_y_ << endl;

    vec2 tmp = vec2(look_at_settings.x, look_at_settings.z) - grid.getOffsetXY();
    tmp *= TERRAIN_MAX_X * 2;

    cam_look = vec3(tmp.x, 0, -tmp.y);
    mirror_cam_look = cam_look;

    grid.setOffsetXY(vec2(offset_x_, offset_y_));
    height_map.setOffsetXY(vec2(offset_x_, offset_y_));
    water.setOffsetXY(vec2(offset_x_, offset_y_));
    refresh();

    if(t_bezier >= 1.0f) {
        bezier = false;
        t_bezier = 0.0f;
        dt_bezier = 0.001f;
    }
}

void Init() {
    // sets background color
    glClearColor(0.937, 0.937, 0.937 /*gray*/, 1.0 /*solid*/);

    // enable depth test.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // enable blending/transparency
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint fb_tex = fb_height_map.Init(TEXTURE_SIZE, TEXTURE_SIZE, true, GL_R32F);
    GLuint water_tex = water_map.Init(window_width, window_height, true, GL_RGB);

 //   glEnable(GL_CULL_FACE);

    height_map.Init(TEXTURE_SIZE, TEXTURE_SIZE);
    grid.Init(fb_tex);
    water.Init(water_tex);
    cube.Init();


    // VIEW MATRICES
    cam_pos = vec3(0.5f, 2.0f, 0.5f);
    cam_look = vec3(16.0f, 0.0f, 0.0f);
    cam_up = vec3(0.0f, 1.0f, 0.0f);
    cam_direction = cam_look - cam_pos;

    mirror_cam_pos = vec3(0.5f, -2.0f, 0.5f);
    mirror_cam_look = cam_look;
    mirror_cam_direction = mirror_cam_look - mirror_cam_pos;
    mirror_cam_up = -cam_up;

    // update view
    view_matrix = lookAt(cam_pos,
                         cam_look,
                         cam_up);
    mirror_view = lookAt(mirror_cam_pos,
                         mirror_cam_look,
                         mirror_cam_up);

    trackball_matrix = IDENTITY_MATRIX;

    projection_matrix = perspective(45.0f,
                                              (GLfloat)window_width / window_height,
                                              0.1f, 8000.0f);

    // MODEL MATRICES
    terrain_model_matrix = translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f));
    cube_model_matrix = terrain_model_matrix;
    water_model_matrix = translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f));

    refresh();
}

vec3 d1 = vec3(-TERRAIN_MAX_X*2, 0.f, TERRAIN_MAX_X*4)       ;
vec3 d2 = vec3(0, 0.f, TERRAIN_MAX_X*4)                      ;
vec3 d3 = vec3(TERRAIN_MAX_X*2, 0.f, TERRAIN_MAX_X*4)        ;
vec3 d4 = vec3(-TERRAIN_MAX_X*2, 0, 0.0f)                    ;
vec3 d5 = vec3(0, 0, 0.0f)                                   ;
vec3 d6 = vec3(TERRAIN_MAX_X*2, 0, 0.0f)                     ;
vec3 d7 = vec3(-TERRAIN_MAX_X*2, 0.f, TERRAIN_MAX_X*2)       ;
vec3 d8 = vec3(0, 0.f, TERRAIN_MAX_X*2)                      ;
vec3 d9 = vec3(TERRAIN_MAX_X*2, 0.f, TERRAIN_MAX_X*2)        ;

mat4 m1 = translate(mat4(1.0f), d1);
mat4 m2 = translate(mat4(1.0f), d2);
mat4 m3 = translate(mat4(1.0f), d3);
mat4 m4 = translate(mat4(1.0f), d4);
mat4 m5 = translate(mat4(1.0f), d5);
mat4 m6 = translate(mat4(1.0f), d6);
mat4 m7 = translate(mat4(1.0f), d7);
mat4 m8 = translate(mat4(1.0f), d8);
mat4 m9 = translate(mat4(1.0f), d9);

// gets called for every frame.
void Display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float time = glfwGetTime();

   // update view
    if (cam_mov_engine_on) {
        cam_mov_speed += cam_mov_d_speed;
    } else if (cam_mov_speed > 0) {
        cam_mov_speed = fmax(cam_mov_speed - cam_mov_d_speed, 0);
    }
    if (cam_rotate_engine_on) {
        cam_rotate_speed += cam_rotate_d_speed;
    } else if (cam_rotate_speed > 0) {
        cam_rotate_speed = fmax(cam_rotate_speed - cam_rotate_d_speed, 0);
    }
    if (cam_jetpack_engine_on) {
        cam_jetpack_speed += cam_jetpack_d_speed;
    } else if (cam_jetpack_speed > 0) {
        cam_jetpack_speed = fmax(cam_jetpack_speed - cam_jetpack_d_speed, 0);
    }
    if(!bezier) {
        cam_move();
        cam_rotate();
    } else {
      t_bezier += dt_bezier;


      cam_bezier(t_bezier, a,b,c,d, look_at_bezier);
    }


    view_matrix = lookAt(cam_pos,
                         cam_look,
                         cam_up);
    // draw a quad on the ground.

    //grid.Draw(time, m5, view_matrix, projection_matrix, 0, 0.f);

    /*grid.moveX(d1.x);
    grid.moveY(d1.z);
    height_map.moveX(d1.x);
    height_map.moveY(d1.z);
    refresh();
    grid.Draw(time, m1, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d2.x);
    grid.moveY(d2.z);
    height_map.moveX(d2.x);
    height_map.moveY(d2.z);
    refresh();
    grid.Draw(time, m2, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d3.x);
    grid.moveY(d3.z);
    height_map.moveX(d3.x);
    height_map.moveY(d3.z);
    refresh();
    grid.Draw(time, m3, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d4.x);
    grid.moveY(d4.z);
    height_map.moveX(d4.x);
    height_map.moveY(d4.z);
    refresh();
    grid.Draw(time, m4, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d6.x);
    grid.moveY(d6.z);
    height_map.moveX(d6.x);
    height_map.moveY(d6.z);
    refresh();
    grid.Draw(time, m6, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d7.x);
    grid.moveY(d7.z);
    height_map.moveX(d7.x);
    height_map.moveY(d7.z);
    refresh();
    grid.Draw(time, m7, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d8.x);
    grid.moveY(d8.z);
    height_map.moveX(d8.x);
    height_map.moveY(d8.z);
    refresh();
    grid.Draw(time, m8, view_matrix, projection_matrix, 0, 0.f);

    grid.moveX(d9.x);
    grid.moveY(d9.z);
    height_map.moveX(d9.x);
    height_map.moveY(d9.z);
    refresh();
    grid.Draw(time, m9, view_matrix, projection_matrix, 0, 0.f);*/

    cube.Draw(cube_model_matrix*translate(IDENTITY_MATRIX, vec3(cam_pos.x, 0, cam_pos.z)), view_matrix, projection_matrix);
    grid.Draw(time, terrain_model_matrix, view_matrix, projection_matrix, 0, 0) ;
    water.Draw(time, water_model_matrix, view_matrix, projection_matrix);

}

// transforms glfw screen coordinates into normalized OpenGL coordinates.
vec2 TransformScreenCoords(GLFWwindow* window, int x, int y) {
    // the framebuffer and the window doesn't necessarily have the same size
    // i.e. hidpi screens. so we need to get the correct one
    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);
    return vec2(2.0f * (float)x / width - 1.0f,
                1.0f - 2.0f * (float)y / height);
}

void MouseButton(GLFWwindow* window, int button, int action, int mod) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x_i, y_i;
        glfwGetCursorPos(window, &x_i, &y_i);
        vec2 p = TransformScreenCoords(window, x_i, y_i);
        trackball.BeingDrag(p.x, p.y);
        old_trackball_matrix = trackball_matrix;
        // Store the current state of the model matrix.
    }
}

void MousePos(GLFWwindow* window, double x, double y) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        vec2 p = TransformScreenCoords(window, x, y);
        // TODO 3: Calculate 'trackball_matrix' given the return value of
        // trackball.Drag(...) and the value stored in 'old_trackball_matrix'.
        // See also the mouse_button(...) function.
        // trackball_matrix = ...
        //old_trackball_matrix = trackball_matrix;
        trackball_matrix = trackball.Drag(p.x, p.y) * old_trackball_matrix;
    }
}

// Gets called when the windows/framebuffer is resized.
void SetupProjection(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;

    cout << "Window has been resized to "
         << window_width << "x" << window_height << "." << endl;

    glViewport(0, 0, window_width, window_height);

    // TODO 1: Use a perspective projection instead;
    projection_matrix = perspective(45.0f,
                                              (GLfloat)window_width / window_height,
                                              0.1f, 80000.0f);

    //fb_height_map.UpdateSize(window_width, window_height);
    //height_map.UpdateSize(window_width, window_height);
    water_map.UpdateSize(window_width, window_height);

}

void ErrorCallback(int error, const char* description) {
    fputs(description, stderr);
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //static double startTime = 0;
    if(action != GLFW_PRESS && key != GLFW_KEY_UP && key != GLFW_KEY_DOWN &&
                               key != GLFW_KEY_LEFT && key != GLFW_KEY_RIGHT &&
                                key != GLFW_KEY_W && key != GLFW_KEY_S &&
                                key != GLFW_KEY_A && key != GLFW_KEY_D &&
                                key != GLFW_KEY_Q && key != GLFW_KEY_E &&
                                key != GLFW_KEY_R && key != GLFW_KEY_F)
    {
        return;
    }

    //if(action == GLFW_PRESS && key == GLFW_KEY_W && key == GLFW_KEY_S &&
    //                          key == GLFW_KEY_A && key == GLFW_KEY_D &&
    //                          key == GLFW_KEY_Q && key == GLFW_KEY_E)
    //{
    //    printf("press");
    //    startTime = glfwGetTime();
    //    //return;
    //}

    if(!enableKeyCallback) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key == GLFW_KEY_N) {
       // grid.enableTerrain();
       mirror_view = translate(mirror_view, vec3(0, 0.01, 0));
    }
    else if (key == GLFW_KEY_M) {
        //grid.disableTerrain();//
        mirror_view = translate(mirror_view, vec3(0, -0.01, 0));

    }
    else if (key == GLFW_KEY_1) {
        height_map.changeSubdivision(-1);
        refresh();
    }
    else if (key == GLFW_KEY_2) {
        enableKeyCallback = false;
        height_map.changeSubdivision(1);
        refresh();
    }
    else if (key == GLFW_KEY_3) {
        enableKeyCallback = false;
        height_map.decreaseAmplitude();
        refresh();
    }
    else if (key == GLFW_KEY_4) {
        enableKeyCallback = false;
        height_map.increaseAmplitude();
        refresh();
    }
    else if (key == GLFW_KEY_5) {
        enableKeyCallback = false;
        height_map.changeLacunarity(-1/16.f);
        refresh();
    }
    else if (key == GLFW_KEY_6) {
        enableKeyCallback = false;
        height_map.changeLacunarity(1/16.f);
        refresh();
    }
    else if (key == GLFW_KEY_7) {
        enableKeyCallback = false;
        height_map.changeOctaves(-1);
        refresh();
    }
    else if (key == GLFW_KEY_8) {
        enableKeyCallback = false;
        height_map.changeOctaves(1);
        refresh();
    }
    else if (key == GLFW_KEY_9) {
        enableKeyCallback = false;
        height_map.decreaseSubdivisionMultiplier(1.025);
        refresh();
    }
    else if (key == GLFW_KEY_0) {
        enableKeyCallback = false;
        height_map.increaseSubdivisionMultiplier(1.025);
        refresh();
    }
    else if (key == GLFW_KEY_X) {
        //enableKeyCallback = false;
        waterheight -= 0.01f;
        water_model_matrix = translate(water_model_matrix, vec3(0, -0.01, 0));
        refresh();
    }
    else if (key == GLFW_KEY_Y) {
        //enableKeyCallback = false;
        waterheight += 0.01f;
        water_model_matrix = translate(water_model_matrix, vec3(0, 0.01, 0));
        refresh();
    }
    else if (key == GLFW_KEY_U) {
        cube_model_matrix = translate(cube_model_matrix, vec3(0, 0.5, 0));
    }
    else if (key == GLFW_KEY_I) {
        cube_model_matrix = translate(cube_model_matrix, vec3(0, -0.5, 0));
    }
    else if (key == GLFW_KEY_Z) {
        fps = !fps;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_R) {
        if (!cam_jetpack_up) {
            cam_jetpack_speed = 0;
        }
        cam_jetpack_engine_on = true;
        cam_jetpack_up = true;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
        cam_jetpack_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_F) {
        if (cam_jetpack_up) {
            cam_jetpack_speed = 0;
        }
        cam_jetpack_engine_on = true;
        cam_jetpack_up = false;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_F) {
        cam_jetpack_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_W) {
        // removes the " inertia " if user change direction (brake..)
        if (!cam_mov_forward) {
            cam_mov_speed = 0;
        }
        cam_mov_engine_on = true;
        cam_mov_forward = true;
        if(bezier) {
            dt_bezier = dt_bezier*1.3f;
        }
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_W) {
        cam_mov_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_S) {
        // removes the " inertia " if user change direction (brake..)
        if (cam_mov_forward) {
            cam_mov_speed = 0;
        }
        cam_mov_engine_on = true;
        cam_mov_forward = false;
        if(bezier) {
           dt_bezier = dt_bezier/1.3f;
        }
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_S) {
        cam_mov_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_A) {
        if (!cam_rotate_positive) {
            cam_rotate_speed = 0;
        }
        cam_rotate_engine_on = true;
        cam_rotate_positive = true;
        cam_rotate_yaw = true;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_A) {
        cam_rotate_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_D) {
        if (cam_rotate_positive) {
            cam_rotate_speed = 0;
        }
        cam_rotate_engine_on = true;
        cam_rotate_positive = false;
        cam_rotate_yaw = true;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_D) {
        cam_rotate_engine_on = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_Q) {
        if (cam_rotate_positive) {
            cam_rotate_speed = 0;
        }
        cam_rotate_engine_on = true;
        cam_rotate_positive = false;
        cam_rotate_yaw = false;  // pitch
//        up_down_rotation = true;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_Q) {
        cam_rotate_engine_on = false;
//        up_down_rotation = false;
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_E) {
        if (!cam_rotate_positive) {
            cam_rotate_speed = 0;
        }
        cam_rotate_engine_on = true;
        cam_rotate_positive = true;
        cam_rotate_yaw = false;  // pitch
//        up_down_rotation = true;
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_E) {
        cam_rotate_engine_on = false;
//        up_down_rotation = false;
    }
    else if (key == GLFW_KEY_O) {
        cam_pos += vec3(0.0f, -0.1f, 0.0f);
    } else if (key == GLFW_KEY_G) {
        bezier = !bezier;
        if(bezier) {
            vec2 point = grid.getOffsetXY();

            a = vec3(point.x, 0.0f, point.y);
            b = vec3(point.x, 0.0f, point.y + 0.25);
            c = vec3(point.x + 0.25, 0.0f, point.y + 0.25);
            d = vec3(point.x + 0.25, 0.0f, point.y);
            look_at_bezier = vec3(point.x + 0.125, 0.0f, point.y);
        }
    }
}


int main(int argc, char *argv[]) {
    // GLFW Initialization
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(ErrorCallback);

    // hint GLFW that we would like an OpenGL 3 context (at least)
    // http://www.glfw.org/faq.html#how-do-i-create-an-opengl-30-context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // attempt to open the window: fails if required version unavailable
    // note some Intel GPUs do not support OpenGL 3.2
    // note update the driver of your graphic card
    GLFWwindow* window = glfwCreateWindow(window_width, window_height,
                                          "Trackball", NULL, NULL);
    if(!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // makes the OpenGL context of window current on the calling thread
    glfwMakeContextCurrent(window);

    // set the callback for escape key
    glfwSetKeyCallback(window, KeyCallback);

    // set the framebuffer resize callback
    glfwSetFramebufferSizeCallback(window, SetupProjection);

    // set the mouse press and position callback
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetCursorPosCallback(window, MousePos);

    // GLEW Initialization (must have a context)
    // https://www.opengl.org/wiki/OpenGL_Loading_Library
    glewExperimental = GL_TRUE; // fixes glew error (see above link)
    if(glewInit() != GLEW_NO_ERROR) {
        fprintf( stderr, "Failed to initialize GLEW\n");
        return EXIT_FAILURE;
    }

    cout << "OpenGL" << glGetString(GL_VERSION) << endl;

    // initialize our OpenGL program
    Init();

    // update the window size with the framebuffer size (on hidpi screens the
    // framebuffer is bigger)
    glfwGetFramebufferSize(window, &window_width, &window_height);
    SetupProjection(window, window_width, window_height);

    // render loop
    while(!glfwWindowShouldClose(window)){
        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cube.Cleanup();
    grid.Cleanup();
    water.Cleanup();

    // close OpenGL window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
