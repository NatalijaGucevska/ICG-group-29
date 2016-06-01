#pragma once
#include "icg_helper.h"
#include <glm/gtc/type_ptr.hpp>

class Cube {

    private:
        GLuint vertex_array_id_;        // vertex array object
        GLuint program_id_;             // GLSL shader program ID
        GLuint vertex_buffer_object_;   // memory buffer

        GLuint texture_id_;             // texture ID
        GLuint MVP_id_;                 // Model, view, projection matrix ID

    public:
        void Init() {
            // compile the shaders
            program_id_ = icg_helper::LoadShaders("cube_vshader.glsl",
                                                  "cube_fshader.glsl");
            if(!program_id_) {
                exit(EXIT_FAILURE);
            }

            glUseProgram(program_id_);

            // vertex one vertex Array
            glGenVertexArrays(1, &vertex_array_id_);
            glBindVertexArray(vertex_array_id_);

            // vertex coordinates
            {
                const GLfloat vertex_point[] = {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,

                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,

                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,

                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,

                                                -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,
                                                -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,

                                                -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,
                                                -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,
                                                 CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE};
                // buffer
                glGenBuffers(1, &vertex_buffer_object_);
                glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_point),
                             vertex_point, GL_STATIC_DRAW);

                // attribute
                GLuint vertex_point_id = glGetAttribLocation(program_id_, "vpoint");
                glEnableVertexAttribArray(vertex_point_id);
                glVertexAttribPointer(vertex_point_id, 3, GL_FLOAT, DONT_NORMALIZE,
                                      ZERO_STRIDE, ZERO_BUFFER_OFFSET);
            }

           // load texture
            /*Correct order
             "left.jpg",
            "right.jpg",
            "down.jpg",
            "up.jpg",
            "back.jpg",
            "front.jpg",*/

           {
             create_cube_map (
                         "left.jpg",
                         "right.jpg",
                         "down.jpg",
                         "up.jpg",
                         "back.jpg",
                         "front.jpg",
                         &texture_id_
                    );
           }

            GLuint tex_id = glGetUniformLocation(program_id_, "tex");
            glUniform1i(tex_id, 0 /*GL_TEXTURE0*/);

            // other uniforms
            {
                MVP_id_ = glGetUniformLocation(program_id_, "MVP");
            }

            // to avoid the current object being polluted
            glBindVertexArray(0);
            glUseProgram(0);
        }

        void Cleanup() {
            glBindVertexArray(0);
            glUseProgram(0);
            glDeleteBuffers(1, &vertex_buffer_object_);
            glDeleteProgram(program_id_);
            glDeleteVertexArrays(1, &vertex_array_id_);
            glDeleteTextures(1, &texture_id_);
        }

        void Draw(const glm::mat4 &model = IDENTITY_MATRIX,
                  const glm::mat4 &view = IDENTITY_MATRIX,
                  const glm::mat4 &projection = IDENTITY_MATRIX) {
            glDepthMask (GL_FALSE);
            glUseProgram(program_id_);
            glBindVertexArray(vertex_array_id_);

            // bind textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture (GL_TEXTURE_CUBE_MAP, texture_id_);

            // setup MVP
            glm::mat4 MVP = projection*view*model;
            glUniformMatrix4fv(MVP_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(MVP));

            // draw
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthMask (GL_TRUE);

            glBindVertexArray(0);
            glUseProgram(0);
        }

        bool load_cube_map_side (GLuint texture, GLenum side_target, const char* file_name) {
          glBindTexture (GL_TEXTURE_CUBE_MAP, texture);

          int x, y, n;
          int force_channels = 4;
          unsigned char*  image_data = stbi_load (
            file_name, &x, &y, &n, force_channels);
          if (!image_data) {
            fprintf (stderr, "ERROR: could not load %s\n", file_name);
            return false;
          }
          // non-power-of-2 dimensions check
          if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
            fprintf (
              stderr, "WARNING: image %s is not power-of-2 dimensions\n", file_name
            );
          }

          // copy image data into 'target' side of cube map
          glTexImage2D (
            side_target,
            0,
            GL_RGBA,
            x,
            y,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            image_data
          );
          free (image_data);
          return true;
        }

        void create_cube_map (
          const char* front,
          const char* back,
          const char* top,
          const char* bottom,
          const char* left,
          const char* right,
          GLuint* tex_cube
        ) {
          // generate a cube-map texture to hold all the sides
          glActiveTexture (GL_TEXTURE0);
          glGenTextures (1, tex_cube);

          // load each image and copy into a side of the cube-map texture
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front));
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back));
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top));
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom));
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left));
          assert (
            load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right));
          // format cube map texture
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


        }
};
