#pragma once
#include "icg_helper.h"
#include <glm/gtc/type_ptr.hpp>

class Water: public Material, public Light {

    private:
        GLuint vertex_array_id_;                // vertex array object
        GLuint vertex_buffer_object_position_;  // memory buffer for positions
        GLuint vertex_buffer_object_index_;     // memory buffer for indices
        GLuint program_id_;                     // GLSL shader program ID
        GLuint texture_reflection_id_;          // texture ID
        GLuint texture_normalmap_id_;           // texture ID
        GLuint texture_dudv_id_;                // texture ID
        GLuint tex_alpha_id_;
        float offset_x_ = 100;
        float offset_y_ = 100;

        GLuint num_indices_;                    // number of vertices to render
        GLuint MV_id_;
        GLuint P_id_;// model, view, proj matrix ID

        void loadTexture(string filename, GLuint *texture_id_, string uniform, GLuint glenumTexture) {
            int width;
            int height;
            int nb_component;
            // set stb_image to have the same coordinates as OpenGL
            stbi_set_flip_vertically_on_load(1);
            unsigned char* image = stbi_load(filename.c_str(), &width,
                                             &height, &nb_component, 0);

            if(image == nullptr) {
                throw(string("Failed to load texture : " + filename));
            }

            glGenTextures(1, texture_id_);

            glBindTexture(GL_TEXTURE_2D, *texture_id_);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            if(nb_component == 3) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                             GL_RGB, GL_UNSIGNED_BYTE, image);
            } else if(nb_component == 4) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, image);
            }

            GLuint tex_id = glGetUniformLocation(program_id_, uniform.c_str());
            glUniform1i(tex_id, glenumTexture /*GL_TEXTUREx*/);

            // cleanup
            glBindTexture(GL_TEXTURE_2D, 0);

            stbi_image_free(image);
        }

    public:
        void Init(GLuint water_tex) {
            // compile the shaders.
            check_error_gl();

            program_id_ = icg_helper::LoadShaders("water_vshader.glsl",
                                                  "water_fshader.glsl");
            check_error_gl();

            if(!program_id_) {
                exit(EXIT_FAILURE);
            }

            glUseProgram(program_id_);

            // vertex one vertex array
            glGenVertexArrays(1, &vertex_array_id_);
            glBindVertexArray(vertex_array_id_);

            // vertex coordinates and indices
            {
                std::vector<GLfloat> vertices;
                std::vector<GLuint> indices;
                // TODO 5: make a triangle grid with dimension 100x100.
                // always two subsequent entries in 'vertices' form a 2D vertex position.
                int grid_dim = 2048;
                float x_min = -TERRAIN_MAX_X * 2;
                float x_max = TERRAIN_MAX_X * 2;
                float y_min = -TERRAIN_MAX_X * 2;
                float y_max = TERRAIN_MAX_X * 2;
                float d_x = (x_max - x_min)/grid_dim;
                float d_y = (y_max - y_min)/grid_dim;

                // the given code below are the vertices for a simple quad.
                // your grid should have the same dimension as that quad, i.e.,
                // reach from [-1, -1] to [1, 1].
                //vertices.push_back(-1.0f); vertices.push_back( 1.0f);
                //vertices.push_back( 1.0f); vertices.push_back( 1.0f);
                //vertices.push_back( 1.0f); vertices.push_back(-1.0f);
                //vertices.push_back(-1.0f); vertices.push_back(-1.0f);

                // and indices.
                //indices.push_back(0);
                //indices.push_back(1);
                //indices.push_back(3);
                //indices.push_back(2);

                // vertex position of the triangles.
                int i = 0;
                for(float y = y_min; y <= y_max; y += d_y) {
                    for(float x = x_min; x <= x_max; x += d_x) {
                        vertices.push_back(x); vertices.push_back(-y);
                    }
                }

                grid_dim++;
                int j = 0;
                for(i = 0; i < grid_dim-1; i++) {
                    for(j = 0; j < grid_dim; j++) {
                        indices.push_back(j + i * grid_dim );
                        indices.push_back(j + (i+1) * grid_dim );
                    }
                    indices.push_back(grid_dim * grid_dim);
                }
                num_indices_ = indices.size();

                glEnable(GL_PRIMITIVE_RESTART);
                glPrimitiveRestartIndex(grid_dim * grid_dim);

                // position buffer
                glGenBuffers(1, &vertex_buffer_object_position_);
                glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_position_);
                glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                             &vertices[0], GL_STATIC_DRAW);

                // vertex indices
                glGenBuffers(1, &vertex_buffer_object_index_);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer_object_index_);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                             &indices[0], GL_STATIC_DRAW);

                // position shader attribute
                GLuint loc_position = glGetAttribLocation(program_id_, "position");
                glEnableVertexAttribArray(loc_position);
                glVertexAttribPointer(loc_position, 2, GL_FLOAT, DONT_NORMALIZE,
                                      ZERO_STRIDE, ZERO_BUFFER_OFFSET);
            }

            // load texture
            {
                texture_reflection_id_ = water_tex;
                glBindTexture(GL_TEXTURE_2D, texture_reflection_id_);

                GLuint heightmap_id = glGetUniformLocation(program_id_, "reflectionTexture");
                glUniform1i(heightmap_id, 0 /*GL_TEXTURE0*/);
                loadTexture("normalmap.png", &texture_normalmap_id_, "normalMapTexture", 1);
                loadTexture("dudv.png", &texture_dudv_id_, "dudvTexture", 2);

                loadTexture("alpha_water.tga", &tex_alpha_id_, "tex_alpha", 3 /*GL_TEXTURE6*/);
            }

            // other uniforms
            MV_id_ = glGetUniformLocation(program_id_, "MV");
            P_id_ = glGetUniformLocation(program_id_, "P");

            // to avoid the current object being polluted
            glBindVertexArray(0);
            glUseProgram(0);
        }

        void Cleanup() {
            glBindVertexArray(0);
            glUseProgram(0);
            glDeleteBuffers(1, &vertex_buffer_object_position_);
            glDeleteBuffers(1, &vertex_buffer_object_index_);
            glDeleteVertexArrays(1, &vertex_array_id_);
            glDeleteProgram(program_id_);

            glDeleteTextures(1, &texture_reflection_id_);
            glDeleteTextures(1, &texture_normalmap_id_);
            glDeleteTextures(1, &texture_dudv_id_);
            //glDeleteTextures(1, &texture_test_id_);
            glDeleteTextures(1, &tex_alpha_id_);
        }

        void Draw(float time, const glm::mat4 &model = IDENTITY_MATRIX,
                  const glm::mat4 &view = IDENTITY_MATRIX,
                  const glm::mat4 &projection = IDENTITY_MATRIX) {
            glUseProgram(program_id_);
            glBindVertexArray(vertex_array_id_);

            //pass the offset
            glUniform1f(glGetUniformLocation(program_id_, "offset_x"), offset_x_);
            glUniform1f(glGetUniformLocation(program_id_, "offset_y"), offset_y_);

            // bind textures
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_reflection_id_);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture_normalmap_id_);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texture_dudv_id_);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, tex_alpha_id_);
            }

            // setup MVP
            glm::mat4 MV = view*model;
            glm::mat4 P = projection;
            glUniformMatrix4fv(MV_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(MV));
            glUniformMatrix4fv(P_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(P));

            Material::Setup(program_id_);
            Light::Setup(program_id_);;

            // pass the current time stamp to the shader.
            glUniform1f(glGetUniformLocation(program_id_, "uniTime"), time);

            // draw
            // TODO 5: for debugging it can be helpful to draw only the wireframe.
            // You can do that by uncommenting the next line.
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // TODO 5: depending on how you set up your vertex index buffer, you
            // might have to change GL_TRIANGLE_STRIP to GL_TRIANGLES.
            glDrawElements(GL_TRIANGLE_STRIP, num_indices_, GL_UNSIGNED_INT, 0);
            // don't see any diff between the two "modes"

            glBindVertexArray(0);
            glUseProgram(0);
        }

        void move(float offset_x, float offset_y) {
            offset_x_ = fmax(offset_x_ + offset_x, 0.f);
            offset_y_ = fmax(offset_y_ + offset_x, 0.f);
        }

        void setOffsetXY(glm::vec2 off) {
            offset_x_ = off.x;
            offset_y_ = off.y;
        }
};
