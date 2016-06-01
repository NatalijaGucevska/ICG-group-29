#pragma once
#include "icg_helper.h"
#include <glm/gtc/type_ptr.hpp>

struct Light {
    glm::vec3 La = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 Ld = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 Ls = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec3 light_pos = glm::vec3(0.0f, 32.0f, 0.f);

    // pass light properties to the shader
    void Setup(GLuint program_id) {
        glUseProgram(program_id);

        // given in camera space
        GLuint light_pos_id = glGetUniformLocation(program_id, "light_pos");

        GLuint La_id = glGetUniformLocation(program_id, "La");
        GLuint Ld_id = glGetUniformLocation(program_id, "Ld");
        GLuint Ls_id = glGetUniformLocation(program_id, "Ls");

        glUniform3fv(light_pos_id, ONE, glm::value_ptr(light_pos));
        glUniform3fv(La_id, ONE, glm::value_ptr(La));
        glUniform3fv(Ld_id, ONE, glm::value_ptr(Ld));
        glUniform3fv(Ls_id, ONE, glm::value_ptr(Ls));
    }
};

struct Material {
    glm::vec3 ka = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 kd = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 ks = glm::vec3(0.3f, 0.3f, 0.3f);
    float alpha = 120.0f;
    
    // pass material properties to the shaders
    void Setup(GLuint program_id) {
        glUseProgram(program_id);

        GLuint ka_id = glGetUniformLocation(program_id, "ka");
        GLuint kd_id = glGetUniformLocation(program_id, "kd");
        GLuint ks_id = glGetUniformLocation(program_id, "ks");
        GLuint alpha_id = glGetUniformLocation(program_id, "alpha");

        glUniform3fv(ka_id, ONE, glm::value_ptr(ka));
        glUniform3fv(kd_id, ONE, glm::value_ptr(kd));
        glUniform3fv(ks_id, ONE, glm::value_ptr(ks));
        glUniform1f(alpha_id, alpha);
    }
};

class Grid : public Material, public Light {

    private:
        GLuint vertex_array_id_;                // vertex array object
        GLuint vertex_buffer_object_position_;  // memory buffer for positions
        GLuint vertex_buffer_object_index_;     // memory buffer for indices
        GLuint program_id_;                     // GLSL shader program ID


        GLuint height_map_id_;                  // height map
        GLuint colormap_texture_id_;            // texture ID

        GLuint tex_rock_id_;                    // rock texture ID
        GLuint tex_grass_id_;                   // grass texture ID
        GLuint tex_sand_id_;                    // sand texture ID
        GLuint tex_snow_id_;                    // snow texture ID

        GLuint tex_alpha_id_;

        GLuint num_indices_;                    // number of vertices to render
        GLuint MV_id_;                          // model, view matrix ID
        GLuint P_id_;                           // projection matrix ID
        GLuint enableTerrain_ = 1;              // does enable terrain

        float offset_x_ = 100;
        float offset_y_ = 100;

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

            if(nb_component == 3) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                             GL_RGB, GL_UNSIGNED_BYTE, image);
            } else if(nb_component == 4) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, image);
            } else if(nb_component == 1) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
            }

            glGenerateMipmap(GL_TEXTURE_2D);  //Generate mipmaps now!!!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

            GLuint tex_id = glGetUniformLocation(program_id_, uniform.c_str());
            glUniform1i(tex_id, glenumTexture /*GL_TEXTUREx*/);

            // cleanup
            glBindTexture(GL_TEXTURE_2D, 0);

            stbi_image_free(image);
        }

    public:
        void Init(GLuint tex_grid) {
            // compile the shaders.
            program_id_ = icg_helper::LoadShaders("grid_vshader.glsl",
                                                  "grid_fshader.glsl");

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
                float x_min = -TERRAIN_MAX_X;
                float x_max = TERRAIN_MAX_X;
                float y_min = -TERRAIN_MAX_X;
                float y_max = TERRAIN_MAX_X;
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

            // load textures
            {
                height_map_id_ = tex_grid;
                glBindTexture(GL_TEXTURE_2D, height_map_id_);

                GLuint heightmap_id = glGetUniformLocation(program_id_, "heightmap");
                glUniform1i(heightmap_id, 0 /*GL_TEXTURE0*/);

                // create 1D texture (colormap)
                {
                    const int ColormapSize=5;
                    GLfloat tex[5*ColormapSize] = {/*red*/    .949019608, .890196078, .22745098,
                                                   /*yellow*/ 53/255.0, 206/255.0, 11/255.0,
                                                   /*yellow*/ 39/255.0, 77/255.0, 29/255.0,
                                                   /*yellow*/ 197/255.0, 197/255.0, 197/255.0,
                                                   /*green*/  1.0, 1.0, 1.0};
                    glGenTextures(1, &colormap_texture_id_);
                    glBindTexture(GL_TEXTURE_1D, colormap_texture_id_);
                    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, ColormapSize, 0, GL_RGB, GL_FLOAT, tex);
                    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    GLuint tex_id = glGetUniformLocation(program_id_, "colormap");
                    glUniform1i(tex_id, 1 /*GL_TEXTURE1*/);
                }

                loadTexture("rock.tga", &tex_rock_id_, "tex_rock", 2 /*GL_TEXTURE2*/);
                loadTexture("grass.tga", &tex_grass_id_, "tex_grass", 3 /*GL_TEXTURE3*/);
                loadTexture("sand.tga", &tex_sand_id_, "tex_sand", 4 /*GL_TEXTURE4*/);
                loadTexture("snow.tga", &tex_snow_id_, "tex_snow", 5 /*GL_TEXTURE5*/);
                loadTexture("alpha.tga", &tex_alpha_id_, "tex_alpha", 6 /*GL_TEXTURE6*/);
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


            // textures
            glDeleteTextures(1, &height_map_id_);
            glDeleteTextures(1, &colormap_texture_id_);

            glDeleteTextures(1, &tex_rock_id_);
            glDeleteTextures(1, &tex_grass_id_);
            glDeleteTextures(1, &tex_sand_id_);
            glDeleteTextures(1, &tex_snow_id_);
            glDeleteTextures(1, &tex_alpha_id_);
        }

        void Draw(float time, const glm::mat4 &model = IDENTITY_MATRIX,
                  const glm::mat4 &view = IDENTITY_MATRIX,
                  const glm::mat4 &projection = IDENTITY_MATRIX,
                  int isReflected=0, float waterHeight=0) {
            glUseProgram(program_id_);
            glBindVertexArray(vertex_array_id_);

            // setup MVP
            glm::mat4 MV = view*model;
            glm::mat4 P = projection;
            glUniformMatrix4fv(MV_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(MV));
            glUniformMatrix4fv(P_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(P));

            Material::Setup(program_id_);
            Light::Setup(program_id_);

            // pass the current time stamp to the shader.
            glUniform1f(glGetUniformLocation(program_id_, "time"), time);

            // pass the enable terrain
            glUniform1i(glGetUniformLocation(program_id_, "enableTerrain"), enableTerrain_);

            //pass the offset
            glUniform1f(glGetUniformLocation(program_id_, "offset_x"), offset_x_);
            glUniform1f(glGetUniformLocation(program_id_, "offset_y"), offset_y_);
            glUniform1f(glGetUniformLocation(program_id_, "water_height"), waterHeight);
            glUniform1f(glGetUniformLocation(program_id_, "is_reflected"), isReflected);

            // bind textures
            {
                // height map
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, height_map_id_);

                // color map
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_1D, colormap_texture_id_);

                glActiveTexture(GL_TEXTURE0 + 2);
                glBindTexture(GL_TEXTURE_2D, tex_rock_id_);
                glActiveTexture(GL_TEXTURE0 + 3);
                glBindTexture(GL_TEXTURE_2D, tex_grass_id_);
                glActiveTexture(GL_TEXTURE0 + 4);
                glBindTexture(GL_TEXTURE_2D, tex_sand_id_);
                glActiveTexture(GL_TEXTURE0 + 5);
                glBindTexture(GL_TEXTURE_2D, tex_snow_id_);

                glActiveTexture(GL_TEXTURE0 + 6);
                glBindTexture(GL_TEXTURE_2D, tex_alpha_id_);
            }

            // draw
            // for debugging it can be helpful to draw only the wireframe.
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // TODO 5: depending on how you set up your vertex index buffer, you
            // might have to change GL_TRIANGLE_STRIP to GL_TRIANGLES.
            glDrawElements(GL_TRIANGLE_STRIP, num_indices_, GL_UNSIGNED_INT, 0);
            // don't see any diff between the two "modes"

            glBindVertexArray(0);
            glUseProgram(0);
        }

        void enableTerrain() {
            enableTerrain_ = 1;
            cout << "enableTerrain(): " << enableTerrain_ << endl;
        }

        void disableTerrain() {
            enableTerrain_ = 0;
            cout << "disableTerrain(): " << enableTerrain_ << endl;
        }

        void move(float offset_x, float offset_y) {
            offset_x_ = fmax(offset_x_ + offset_x, 0.f);
            offset_y_ = fmax(offset_y_ + offset_y, 0.f);
        }

        glm::vec2 getOffsetXY() {
            return glm::vec2(offset_x_, offset_y_);
        }

        void setOffsetXY(glm::vec2 off) {
            offset_x_ = off.x;
            offset_y_ = off.y;
        }
};
