
#include "rasterizer.hpp"

#include <math.h>

#include <algorithm>
#include <array>
#include <opencv2/opencv.hpp>
#include <vector>

#include "Shader.hpp"
#include "Triangle.hpp"

rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions) {
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices) {
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols) {
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_normals(const std::vector<Eigen::Vector3f> &normals) {
    auto id = get_next_id();
    nor_buf.emplace(id, normals);

    normal_id = id;

    return {id};
}

void rst::rasterizer::post_process_buffer() {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int index = get_index(x, y);
            for (int i = 0; i < 4; i++) {
                frame_buf[index] += ssaa_frame_buf[4 * index + i];
            }
            frame_buf[index] /= 4;
        }
    }
}

// Bresenham's line drawing algorithm
void rst::rasterizer::draw_line(Eigen::Vector3f begin, Eigen::Vector3f end) {
    auto x1 = begin.x();
    auto y1 = begin.y();
    auto x2 = end.x();
    auto y2 = end.y();

    Eigen::Vector3f line_color = {255, 255, 255};

    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

    dx = x2 - x1;
    dy = y2 - y1;
    dx1 = fabs(dx);
    dy1 = fabs(dy);
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;

    if (dy1 <= dx1) {
        if (dx >= 0) {
            x = x1;
            y = y1;
            xe = x2;
        } else {
            x = x2;
            y = y2;
            xe = x1;
        }
        Eigen::Vector2i point = Eigen::Vector2i(x, y);
        set_pixel(point, line_color);
        for (i = 0; x < xe; i++) {
            x = x + 1;
            if (px < 0) {
                px = px + 2 * dy1;
            } else {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
                    y = y + 1;
                } else {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            //            delay(0);
            Eigen::Vector2i point = Eigen::Vector2i(x, y);
            set_pixel(point, line_color);
        }
    } else {
        if (dy >= 0) {
            x = x1;
            y = y1;
            ye = y2;
        } else {
            x = x2;
            y = y2;
            ye = y1;
        }
        Eigen::Vector2i point = Eigen::Vector2i(x, y);
        set_pixel(point, line_color);
        for (i = 0; y < ye; i++) {
            y = y + 1;
            if (py <= 0) {
                py = py + 2 * dx1;
            } else {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
                    x = x + 1;
                } else {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            //            delay(0);
            Eigen::Vector2i point = Eigen::Vector2i(x, y);
            set_pixel(point, line_color);
        }
    }
}

auto to_vec4(const Eigen::Vector3f &v3, float w = 1.0f) {
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

static bool insideTriangle(float x, float y, const Vector4f *_v) {
    Vector3f v[3];
    for (int i = 0; i < 3; i++)
        v[i] = {_v[i].x(), _v[i].y(), 1.0};
    Vector3f p(x, y, 1.);
    Vector3f f0, f1, f2;
    f0 = (p - v[0]).cross(v[1] - v[0]);
    f1 = (p - v[1]).cross(v[2] - v[1]);
    f2 = (p - v[2]).cross(v[0] - v[2]);
    if (f0.dot(f1) > 0 && f1.dot(f2) > 0)
        return true;
    return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector4f *v) {
    float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) /
               (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() -
                v[2].x() * v[1].y());
    float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) /
               (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() -
                v[0].x() * v[2].y());
    float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y()) /
               (v[2].x() * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * v[2].y() + v[0].x() * v[1].y() -
                v[1].x() * v[0].y());
    return {c1, c2, c3};
}

void rst::rasterizer::draw(std::vector<Triangle *> &TriangleList, bool culling, bool shadow) {
    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;
    //culling = false;

    Eigen::Matrix4f mvp = projection * view * model;

    std::vector<light> viewspace_lights;
    for (const auto &l : lights) {
        light view_space_light;
        view_space_light.position = (view * to_vec4(l.position, 1.0f)).head<3>();
        view_space_light.intensity = l.intensity;
        viewspace_lights.push_back(view_space_light);
    }

    for (const auto &t : TriangleList) {
        Triangle newtri = *t;

        std::array<Eigen::Vector4f, 3> mm{(view * model * t->v[0]), (view * model * t->v[1]), (view * model * t->v[2])};

        std::array<Eigen::Vector3f, 3> viewspace_pos;

        std::transform(mm.begin(), mm.end(), viewspace_pos.begin(), [](auto &v) { return v.template head<3>(); });

        // FIXME: The following Back-face culling code is partially incorrect.
        // I can't disable it, otherwise the FPS is too low.
        if (culling) {

            Eigen::Vector3f A = viewspace_pos[0];
            Eigen::Vector3f B = viewspace_pos[1];
            Eigen::Vector3f C = viewspace_pos[2];

            Eigen::Vector3f AB = B - A;
            Eigen::Vector3f AC = C - A;
            Eigen::Vector3f N = AB.cross(AC);

            Eigen::Vector3f P = Eigen::Vector3f {0, 0, -1};
            if(N.dot(P) >= 0){
                continue;
            }
        }

        Eigen::Vector4f v[] = {mvp * t->v[0], mvp * t->v[1], mvp * t->v[2]};

        // Homogeneous division
        for (auto &vec : v) {
            vec.x() /= vec.w();
            vec.y() /= vec.w();
            vec.z() /= vec.w();
        }

        Eigen::Matrix4f inv_trans = (view * model).inverse().transpose();
        Eigen::Vector4f n[] = {
            inv_trans * to_vec4(t->normal[0], 0.0f), 
            inv_trans * to_vec4(t->normal[1], 0.0f),
            inv_trans * to_vec4(t->normal[2], 0.0f)
        };

        // Viewport transformation
        for (auto &vert : v) {
            vert.x() = 0.5 * width * (vert.x() + 1.0);
            vert.y() = 0.5 * height * (vert.y() + 1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i) {
            // screen space coordinates
            newtri.setVertex(i, v[i]);
        }

        for (int i = 0; i < 3; ++i) {
            // view space normal
            newtri.setNormal(i, n[i].head<3>());
        }
        
        rasterize_triangle(newtri, viewspace_pos, viewspace_lights, shadow);
    }
}

static Eigen::Vector3f interpolate(float alpha, float beta, float gamma, const Eigen::Vector3f &vert1,
                                   const Eigen::Vector3f &vert2, const Eigen::Vector3f &vert3, float weight) {
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

static Eigen::Vector2f interpolate(float alpha, float beta, float gamma, const Eigen::Vector2f &vert1,
                                   const Eigen::Vector2f &vert2, const Eigen::Vector2f &vert3, float weight) {
    auto u = (alpha * vert1[0] + beta * vert2[0] + gamma * vert3[0]);
    auto v = (alpha * vert1[1] + beta * vert2[1] + gamma * vert3[1]);

    u /= weight;
    v /= weight;

    return Eigen::Vector2f(u, v);
}

void rst::rasterizer::rasterize_triangle(const Triangle &t, const std::array<Eigen::Vector3f, 3> &view_pos, const std::vector<light> &view_lights, bool shadow) {
    
    // Find the texture of current object.
    Texture* currentTexture = nullptr;
    for (Texture* texture : textures) {
        if (texture != nullptr && texture->getObjectId() == t.objectId && texture->getHasTexture()) {
            currentTexture = texture;
        }
    }    
    
    auto v = t.toVector4();

    // Find the boundary box.
    int x_min = std::numeric_limits<int>::max();
    int x_max = std::numeric_limits<int>::min();
    int y_min = std::numeric_limits<int>::max();
    int y_max = std::numeric_limits<int>::min();

    for (const auto& record : v){
        float x = record[0];
        float y = record[1];

        if(x < x_min)   x_min = x;
        if(x > x_max)   x_max = x;
        if(y < y_min)   y_min = y;
        if(y > y_max)   y_max = y;
    }

    int padding = 5;
    x_min = std::max(x_min-padding, 0);
    y_min = std::max(y_min-padding, 0);
    x_max = std::min(x_max+padding, width);
    y_max = std::min(y_max+padding, height);
   
    // Perform Phong shading           
    for(int x=x_min; x<x_max; x++){
        for(int y=y_min; y<y_max; y++){     

            if(insideTriangle(x, y, t.v)){

                auto[alpha, beta, gamma] = computeBarycentric2D(x+0.5, y+0.5, t.v);
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma /v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w(); 
                z_interpolated *= w_reciprocal;

                // Interpolate everything for each pixel.
                Eigen::Vector3f color = alpha * t.color[0] + beta * t.color[1] + gamma * t.color[2];
                Eigen::Vector3f normal = alpha * t.normal[0] + beta * t.normal[1] + gamma * t.normal[2];
                Eigen::Vector2f texcoords = alpha * t.tex_coords[0] + beta * t.tex_coords[1] + gamma * t.tex_coords[2];
                Eigen::Vector3f shadingcoords = alpha * view_pos[0] + beta * view_pos[1] + gamma * view_pos[2];

                // Z-buffer
                int index = get_index(x, y);
                if(z_interpolated < depth_buf[index]){
                    depth_buf[index] = z_interpolated;
                        
                    // pass them to the fragment_shader_payload
                    fragment_shader_payload payload(color, normal.normalized(), texcoords, view_lights, currentTexture); 
                    payload.view_pos = shadingcoords;

                    // Call the fragment shader to get the pixel color
                    auto pixel_color = fragment_shader(payload);
                        
                    /*
                    if (shadow) {

                        // Find the relative position to the light source (by given shadow_projection and shadow_view)
                        Eigen::Vector4f view_pos {shadingcoords[0], shadingcoords[1], shadingcoords[2], 1.0f};
                        Eigen::Matrix4f mvp = shadow_projection * shadow_view * model.inverse();
                        Eigen::Vector4f v = mvp * view_pos;

                        // To homogeneous form!
                        v = v / v[3];   

                        // Depth value of shadow depth map
                        float z_A = shadow_buf[index];

                        // Depth value of the relative position 
                        float f1 = (50 - 0.1) / 2.0;
                        float f2 = (50 + 0.1) / 2.0;
                        float z_B = v.z() * f1 + f2;

                        // Draw shadow
                        if(z_B > z_A)   pixel_color *= 0.3;
                    }
                    */
                        
                    set_pixel(Vector2i(x, y), pixel_color);
                }

            }
        }
    }
        
    
}

void rst::rasterizer::set_model(const Eigen::Matrix4f &m) {
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f &v) {
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f &p) {
    projection = p;
}

void rst::rasterizer::set_lights(const std::vector<light> &l) {
    lights = l;
}

void rst::rasterizer::set_shadow_view(const Eigen::Matrix4f &v) {
    shadow_view = v;
}

void rst::rasterizer::set_shadow_buffer(const std::vector<float> &shadow_buffer) {
    std::copy(shadow_buffer.begin(), shadow_buffer.end(), this->shadow_buf.begin());
}

void rst::rasterizer::clear(rst::Buffers buff) {
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color) {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        std::fill(ssaa_frame_buf.begin(), ssaa_frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }

    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth) {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(ssaa_depth_buf.begin(), ssaa_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h) {
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    shadow_buf.resize(w * h);
    ssaa_frame_buf.resize(4 * w * h);
    ssaa_depth_buf.resize(4 * w * h);
    //texture = std::nullopt;
}

int rst::rasterizer::get_index(int x, int y) {
    return (height - y - 1) * width + x;
}

void rst::rasterizer::set_pixel(const Vector2i &point, const Eigen::Vector3f &color) {
    // old index: auto ind = point.y() + point.x() * width;
    int ind = (height - point.y() - 1) * width + point.x();
    frame_buf[ind] = color;
}

void rst::rasterizer::set_vertex_shader(std::function<Eigen::Vector3f(vertex_shader_payload)> vert_shader) {
    vertex_shader = vert_shader;
}

void rst::rasterizer::set_fragment_shader(std::function<Eigen::Vector3f(fragment_shader_payload)> frag_shader) {
    fragment_shader = frag_shader;
}
