
#include "rasterizer.hpp"

#include <math.h>

#include <algorithm>
#include <array>
#include <opencv2/opencv.hpp>
#include <vector>
#include "snow.hpp"

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
    // Use this function to draw a line from `begin` to `end` point on the image.
    auto x1 = begin.x();
    auto y1 = begin.y();
    auto x2 = end.x();
    auto y2 = end.y();

    Eigen::Vector3f line_color = {255, 255, 255};

    // transpose steep lines, leaving octants 1, 4, 5, 8
    int Dx = x2 - x1;
    int Dy = y2 - y1;
    int steepmask = (abs(Dx) - abs(Dy)) >> 31; // 0xffffffff if abs(Dx) < abd(Dy), 0 if abs(Dx) >= abs(Dy)
    if (steepmask) {
        std::swap(x1, y1);
        std::swap(x2, y2);
        std::swap(Dx, Dy);
    }

    // solve left to right, leaving octants 1, 8
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
        Dx = -Dx;
        Dy = -Dy;
    }

    // n = (-sy * Dy, sy * Dx) is the normal vector of the line with negative=must change y
    // n^Tx = f(dx, dy) = -sy * Dy * dx + sy * Dx * dy is the line function in delta coordinates
    // D = 2 * f(dx + 1, dy + sy * 0.5) is twice the line function at the midpoint of next two pixels
    int sy = (Dy > 0) - (Dy < 0); // sign of Dy
    int D = -2 * sy * Dy + Dx; // initial f(1, sy * 0.5) rounded down

    int x = x1;
    int y = y1;
    int count = 0;
    while (x <= (int) x2) {
        // draw current point, after transposing back
        int px = (y & steepmask) + (x & ~steepmask);
        int py = (x & steepmask) + (y & ~steepmask);
        set_pixel(Eigen::Vector2i(px, py), line_color);

        // if line function is negative, dy+=sy
        // f(dx, dy + sy) - f(dx, dy) = Dx
        int Dmask = (D >> 31); // 0xffffffff if D < 0, 0 if D >= 0
        // in the original code, tiebreaks depend on steepmask
        Dmask |= ~steepmask & ((D != 0) - 1);
        y += sy & Dmask;
        D += (2 * Dx) & Dmask;
        // always dx+=1
        // f(dx + 1, dy) - f(dx, dy) = -sy * Dy
        x++;
        D -= 2 * sy * Dy;
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

static std::array<std::array<float, 3>, 3> computeBarycentric(float x, float y, const Vector2f v[3]) {
    Vector2f a, b, c, p, ab, ac, ap;

    a = v[0]; b = v[1]; c = v[2];
    p = Eigen::Vector2f(x, y);
    ab = b - a; ac = c - a; ap = p - a;

    // Cramer's method for beta * ab + gamma * ac = ap
    float invD = 1 / (ab.x() * ac.y() - ac.x() * ab.y());

    float beta = (ap.x() * ac.y() - ac.x() * ap.y()) * invD;
    float betaDx = ac.y() * invD;
    float betaDy = -ac.x() * invD;

    float gamma = (ab.x() * ap.y() - ap.x() * ab.y()) * invD;
    float gammaDx = -ab.y() * invD;
    float gammaDy = ab.x() * invD;

    float alpha = 1 - beta - gamma;
    float alphaDx = -betaDx - gammaDx;
    float alphaDy = -betaDy - gammaDy;

    return {{{alpha, beta, gamma}, {alphaDx, betaDx, gammaDx}, {alphaDy, betaDy, gammaDy}}};
}

void rst::rasterizer::draw(std::vector<Triangle *> &TriangleList, bool culling, bool shadow, bool snow) {
    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

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

        if (culling) {

            Eigen::Vector3f A = viewspace_pos[0];
            Eigen::Vector3f B = viewspace_pos[1];
            Eigen::Vector3f C = viewspace_pos[2];

            Eigen::Vector3f AB = B - A;
            Eigen::Vector3f AC = C - A;
            Eigen::Vector3f N = AB.cross(AC);

            if(N.dot(A) >= 0){
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

        
        auto nnn = t->normal[0];
        rasterize_triangle(newtri, nnn, t->a(), viewspace_pos, viewspace_lights, shadow, snow);
    }
}

void rst::rasterizer::draw_occlusion_map(std::vector<Triangle *> &TriangleList, bool culling) {
    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = occlusion_view * model;
    for (const auto &t : TriangleList) {

        //std::cout << t->a().transpose() << std::endl;

        Triangle newtri = *t;

        std::array<Eigen::Vector4f, 3> mm{(occlusion_view * model * t->v[0]),
                                          (occlusion_view * model * t->v[1]),
                                          (occlusion_view * model * t->v[2])};

        std::array<Eigen::Vector3f, 3> occlusionspace_pos;

        std::transform(mm.begin(), mm.end(), occlusionspace_pos.begin(),
                       [](auto &v) { return v.template head<3>(); });

        /**/

        Eigen::Vector3f A = occlusionspace_pos[0];
        Eigen::Vector3f B = occlusionspace_pos[1];
        Eigen::Vector3f C = occlusionspace_pos[2];

        //std::cout << A.z() << " " << B.z() << " " << C.z() << std::endl;

        if (culling) {

            Eigen::Vector3f A = occlusionspace_pos[0];
            Eigen::Vector3f B = occlusionspace_pos[1];
            Eigen::Vector3f C = occlusionspace_pos[2];

            Eigen::Vector3f AB = B - A;
            Eigen::Vector3f AC = C - A;
            Eigen::Vector3f N = AB.cross(AC);

            if (N.dot(A) >= 0) {
                continue;
            }
        }

        Eigen::Vector4f v[] = {mvp * t->v[0], mvp * t->v[1], mvp * t->v[2]};

        // Homogeneous division
        for (auto &vec : v) {
            vec /= vec.w();
        }

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

        rasterize_occlusion_map_triangle(newtri);
    }
}

template <typename T>
static T interpolate(std::array<float, 3> barycentric_coords, const T values[3]) {
    return barycentric_coords[0] * values[0] + barycentric_coords[1] * values[1] + barycentric_coords[2] * values[2];
}

struct rast_px_info_all {
    std::array<float, 3> barycentric_coords;
    float z_numerator;
    float w_denominator;
    Eigen::Vector3f interpolated_color;
    Eigen::Vector3f interpolated_normal;
    Eigen::Vector2f interpolated_texcoords;
    Eigen::Vector3f interpolated_shadingcoords;

    static rast_px_info_all fromBarycentric(const std::array<float, 3> &barycentric_coords,
                                                               const float (&z_weighted)[3],
                                                               const float (&w_reciprocal)[3],
                                                               const Eigen::Vector3f (&colors)[3],
                                                               const Eigen::Vector3f (&normals)[3],
                                                               const Eigen::Vector2f (&texcoords)[3],
                                                               const Eigen::Vector3f (&shadingcoords)[3]) {
        return {{barycentric_coords[0], barycentric_coords[1], barycentric_coords[2]},
                interpolate(barycentric_coords, z_weighted),
                interpolate(barycentric_coords, w_reciprocal),
                interpolate(barycentric_coords, colors),
                interpolate(barycentric_coords, normals),
                interpolate(barycentric_coords, texcoords),
                interpolate(barycentric_coords, shadingcoords)};
    }

    rast_px_info_all operator*(const float scalar) const {
        return {{barycentric_coords[0] * scalar,
            barycentric_coords[1] * scalar,
            barycentric_coords[2] * scalar},
                z_numerator * scalar,
                w_denominator * scalar,
                interpolated_color * scalar,
                interpolated_normal * scalar,
                interpolated_texcoords * scalar,
                interpolated_shadingcoords * scalar};
    }

    rast_px_info_all operator+=(const rast_px_info_all &other) {
        barycentric_coords[0] += other.barycentric_coords[0];
        barycentric_coords[1] += other.barycentric_coords[1];
        barycentric_coords[2] += other.barycentric_coords[2];
        z_numerator += other.z_numerator;
        w_denominator += other.w_denominator;
        interpolated_color += other.interpolated_color;
        interpolated_normal += other.interpolated_normal;
        interpolated_texcoords += other.interpolated_texcoords;
        interpolated_shadingcoords += other.interpolated_shadingcoords;
        return *this;
    }
};

void rst::rasterizer::rasterize_triangle(const Triangle &t, 
                                            Eigen::Vector3f real_normal,
                                            Eigen::Vector4f real_coord,
                                            const std::array<Eigen::Vector3f, 3> &view_pos, 
                                            const std::vector<light> &view_lights, 
                                            bool shadow, bool snow) {
    
    // Find the texture of current object.
    Texture* currentTexture = nullptr;
    for (Texture* texture : textures) {
        if (texture != nullptr && texture->getObjectId() == t.objectId && texture->getHasTexture()) {
            currentTexture = texture;
        }
    }    
    
    const std::array<Vector4f, 3> v = t.toVector4();
    /* const Vector2f v2D[3] = {v[0].hnormalized().head<2>(), */
    /*                          v[1].hnormalized().head<2>(), */
    /*                          v[2].hnormalized().head<2>()}; */
    const Vector2f v2D[3] = {v[0].head<2>(), v[1].head<2>(), v[2].head<2>()};

    // Find the boundary box.
    auto [x_min_raw, x_max_raw] = std::minmax({v[0].x(), v[1].x(), v[2].x()});
    auto [y_min_raw, y_max_raw] = std::minmax({v[0].y(), v[1].y(), v[2].y()});
    int x_min = std::max(std::floor(x_min_raw), 0.0f);
    int y_min = std::max(std::floor(y_min_raw), 0.0f);
    int x_max = std::min(std::ceil(x_max_raw), (float) width);
    int y_max = std::min(std::ceil(y_max_raw), (float) height);

    /* const float z_weighted[3] = {v[0].z() / v[0].w(), v[1].z() / v[1].w(), v[2].z() / v[2].w()}; */
    /* const float w_reciprocal[3] = {1 / v[0].w(), 1 / v[1].w(), 1 / v[2].w()}; */
    const float z_weighted[3] = {v[0].z(), v[1].z(), v[2].z()};
    const float w_reciprocal[3] = {1, 1, 1};

    // Perform Phong shading
    const Eigen::Vector3f shading_coords[3] = {view_pos[0], view_pos[1], view_pos[2]};
    auto [barycentric, barycentricDx, barycentricDy] = computeBarycentric(x_min + 0.5, y_min + 0.5, v2D);

    auto px = rast_px_info_all::fromBarycentric;
    rast_px_info_all cur_px = px(barycentric, z_weighted, w_reciprocal,
                                 t.color, t.normal, t.tex_coords, shading_coords);
    const rast_px_info_all px_Dx = px(barycentricDx, z_weighted, w_reciprocal,
                                      t.color, t.normal, t.tex_coords, shading_coords);
    const rast_px_info_all px_Dy = px(barycentricDy, z_weighted, w_reciprocal,
                                      t.color, t.normal, t.tex_coords, shading_coords);
   
    // iterate through the bounding box
    for (int x = x_min; x <= x_max; x++){
        rast_px_info_all saved_start_of_row_pixel = cur_px;
        for (int y = y_min; y <= y_max; y++){     

            if (insideTriangle(x + 0.5, y + 0.5, t.v)){

                // Z-buffer
                float z_interpolated = cur_px.z_numerator / cur_px.w_denominator;
                int index = get_index(x, y);

                //if(!snow){
                //    if(real_3d_pos.y() > occlusion_buf[index]){
                //        //std::cout << occlusion_buf[index] << std::endl;
                //        //occlusion_buf[index] = real_3d_pos.y();
                //    }
                //}

                if(z_interpolated < depth_buf[index]){
                    depth_buf[index] = z_interpolated;

                    //std::cout << depth_buf[index] << std::endl;
                        
                    // pass them to the fragment_shader_payload
                    fragment_shader_payload payload(cur_px.interpolated_color,
                                                    cur_px.interpolated_normal.normalized(),
                                                    cur_px.interpolated_texcoords,
                                                        view_lights, currentTexture); 
                    payload.view_pos = cur_px.interpolated_shadingcoords;

                    // Call the fragment shader to get the pixel color
                    auto pixel_color = fragment_shader(payload);
                        
                    
                    if (shadow) {

                        // Find the relative position to the light source (by given shadow_projection and shadow_view)
                        Eigen::Vector4f view_pos {cur_px.interpolated_shadingcoords[0], cur_px.interpolated_shadingcoords[1], cur_px.interpolated_shadingcoords[2], 1.0f};
                        
                        view_pos = real_coord;
                        Eigen::Matrix4f mvp = shadow_projection * occlusion_view * model.inverse();
                        Eigen::Vector4f v = mvp * view_pos;

                        // To homogeneous form!
                        v = v / v[3];   

                        // Depth value of shadow depth map
                        float z_A = occlusion_buf[index];

                        // Depth value of the relative position 
                        float f1 = (50 - 0.1) / 2.0;
                        float f2 = (50 + 0.1) / 2.0;
                        float z_B = v.z() * f1 + f2;

                        //;std::cout << z_A << " " << z_B << std::endl;

                        // Draw shadow
                        if(z_B > z_A)   pixel_color *= 0.3;
                    }
                    
                    

                    if (snow) {
                        auto snow_color = snow_phong_fragment_shader(payload);

                        // transform the view space coordinates to upright coordinates
                        Eigen::Vector4f snow_coord = view_to_occlusion *
                            to_vec4(cur_px.interpolated_shadingcoords, 1.0f);
                        snow_coord = snow_coord / snow_coord.w();
                        Eigen::Vector4f snow_normal4 = view_to_occlusion *
                            to_vec4(cur_px.interpolated_normal, 1.0f);
                        static int iters = 0;
                        snow_normal4 = snow_normal4 / snow_normal4.w();
                        Eigen::Vector3f snow_normal = snow_normal4.head<3>();

                        // Viewport transformation
                        float f1 = (50 - 0.1) / 2.0;
                        float f2 = (50 + 0.1) / 2.0;
                        snow_coord.x() = 0.5 * width * (snow_coord.x() + 1.0);
                        snow_coord.y() = 0.5 * height * (snow_coord.y() + 1.0);
                        snow_coord.z() = snow_coord.z() * f1 + f2;

                        bool is_snow = false;
                        if (snow_coord.x() >= 0 && snow_coord.x() < width &&
                            snow_coord.y() >= 0 && snow_coord.y() < height) {
                            int snow_index = get_index(snow_coord.x(), snow_coord.y());
                            if (snow_coord.z() <= occlusion_map[snow_index] + 0.1) {
                                is_snow = true;
                            }
                        }

                        // calculate exposure fc
                        float fe = is_snow ? 1.0f : 0.0f;
                        //float finc = inclination(snow_normal);
                        //float fp = fe;

                        // We need to get the normal from the original triangle
                        float finc = inclination(real_normal);
                        float fp = finc;

                        if (iters % 1000000 == 1) {
                            /* std::cout << "fe: " << fe << ", finc: " << finc << ", fp: " << fp << std::endl; */
                            /* std::cout << "snow normal: " << cur_px.interpolated_normal << std::endl; */
                            /* std::cout << "snow norma: " << snow_normal << std::endl; */
                            /* std::cout << "snow_color: " << snow_color << std::endl << "pixel_color: " << pixel_color << std::endl; */
                        }
                        iters++;
                        pixel_color = fp * snow_color + (1 - fp) * pixel_color;
                        if (iters % 1000000 == 1) std::cout << "pixel_color: " << pixel_color << std::endl;
                    }

                    //std::cout << depth_buf[index] << " " << occlusion_buf[index] << std::endl;
                    /*
                    if(depth_buf[index] < occlusion_buf[index]){
                        //std::cout << real_3d_pos.y() << " " << occlusion_buf[index] << std::endl;
                        set_pixel(Vector2i(x, y), Eigen::Vector3f(255, 0, 0));
                        //occlusion_buf[index] = real_3d_pos.z();
                    }
                    */

                    // DO NOT REMOVE ME - start
                    // It can verify if the implementation of inclination part is correct
                    // If it's correct, you will see different colors based on the pitch angle, regardless of the eye position.

                    //Eigen::Vector3f N = real_normal.normalized();
                    //Eigen::Vector3f U = Eigen::Vector3f(0, 1, 0);
                    //float nu = N.dot(U);
                    //if(nu <= 0.0000) set_pixel(Vector2i(x, y), Eigen::Vector3f(255, 0, 0));             // [90, 180] -> Red
                    //else if(nu <= 0.2588)   set_pixel(Vector2i(x, y), Eigen::Vector3f(255, 128, 0));    // [75, 90)  -> Orange 
                    //else if(nu <= 0.5000)   set_pixel(Vector2i(x, y), Eigen::Vector3f(255, 255, 0));    // [60, 75)  -> Yellow 
                    //else if(nu <= 0.7071)   set_pixel(Vector2i(x, y), Eigen::Vector3f(0, 255, 0));      // [45, 60)  -> Green
                    //else if(nu <= 0.8660)   set_pixel(Vector2i(x, y), Eigen::Vector3f(0, 255, 255));    // [30, 45)  -> Cyan
                    //else if(nu <= 0.9659)   set_pixel(Vector2i(x, y), Eigen::Vector3f(0, 0, 255));      // [15, 30)  -> Blue
                    //else    set_pixel(Vector2i(x, y), Eigen::Vector3f(128, 0, 255));                    // [0, 15)  -> Purple  
                    // DO NOT REMOVE ME - end

                    set_pixel(Vector2i(x, y), pixel_color);
                }
            }
            // update interpolated values for next pixel
            cur_px += px_Dy;
        }
        // update interpolated values for next row
        saved_start_of_row_pixel += px_Dx;
        cur_px = saved_start_of_row_pixel;
    }
}

struct rast_px_info_nocolor {
    std::array<float, 3> barycentric_coords;
    float z_numerator;
    float w_denominator;

    static rast_px_info_nocolor fromBarycentric(const std::array<float, 3> &barycentric_coords,
                                                               const float (&z_weighted)[3],
                                                               const float (&w_reciprocal)[3]) {
        return {{barycentric_coords[0], barycentric_coords[1], barycentric_coords[2]},
                interpolate(barycentric_coords, z_weighted),
                interpolate(barycentric_coords, w_reciprocal)};
    }

    rast_px_info_nocolor operator*(const float scalar) const {
        return {{barycentric_coords[0] * scalar,
            barycentric_coords[1] * scalar,
            barycentric_coords[2] * scalar},
                z_numerator * scalar,
                w_denominator * scalar};
    }

    rast_px_info_nocolor operator+=(const rast_px_info_nocolor &other) {
        barycentric_coords[0] += other.barycentric_coords[0];
        barycentric_coords[1] += other.barycentric_coords[1];
        barycentric_coords[2] += other.barycentric_coords[2];
        z_numerator += other.z_numerator;
        w_denominator += other.w_denominator;
        return *this;
    }
};

void rst::rasterizer::rasterize_occlusion_map_triangle(const Triangle& t) {
    const std::array<Vector4f, 3> v = t.toVector4();
    /* const Vector2f v2D[3] = {v[0].hnormalized().head<2>(), */
    /*                          v[1].hnormalized().head<2>(), */
    /*                          v[2].hnormalized().head<2>()}; */
    const Vector2f v2D[3] = {v[0].head<2>(), v[1].head<2>(), v[2].head<2>()};

    // Find the boundary box.
    auto [x_min_raw, x_max_raw] = std::minmax({v[0].x(), v[1].x(), v[2].x()});
    auto [y_min_raw, y_max_raw] = std::minmax({v[0].y(), v[1].y(), v[2].y()});
    int x_min = std::max(std::floor(x_min_raw), 0.0f);
    int y_min = std::max(std::floor(y_min_raw), 0.0f);
    int x_max = std::min(std::ceil(x_max_raw), (float) width);
    int y_max = std::min(std::ceil(y_max_raw), (float) height);

    /* const float z_weighted[3] = {v[0].z() / v[0].w(), v[1].z() / v[1].w(), v[2].z() / v[2].w()}; */
    /* const float w_reciprocal[3] = {1 / v[0].w(), 1 / v[1].w(), 1 / v[2].w()}; */
    const float z_weighted[3] = {v[0].z(), v[1].z(), v[2].z()};
    const float w_reciprocal[3] = {1, 1, 1};

    auto [barycentric, barycentricDx, barycentricDy] = computeBarycentric(x_min + 0.5, y_min + 0.5, v2D);

    auto px = rast_px_info_nocolor::fromBarycentric;
    rast_px_info_nocolor cur_px = px(barycentric, z_weighted, w_reciprocal);
    const rast_px_info_nocolor px_Dx = px(barycentricDx, z_weighted, w_reciprocal);
    const rast_px_info_nocolor px_Dy = px(barycentricDy, z_weighted, w_reciprocal);

    // iterate through the bounding box
    for (int x = x_min; x <= x_max; x++) {
        rast_px_info_nocolor saved_start_of_row_pixel = cur_px;
        for (int y = y_min; y <= y_max; y++) {
            if (insideTriangle(x + 0.5, y + 0.5, t.v)) {
                // Z-buffer
                float z_interpolated = cur_px.z_numerator / cur_px.w_denominator;
                int index = get_index(x, y);
                if (z_interpolated > occlusion_map[index]) {
                    occlusion_map[index] = z_interpolated;
                }
            }
            // update interpolated values for next pixel
            cur_px += px_Dy;
        }
        // update interpolated values for next row
        saved_start_of_row_pixel += px_Dx;
        cur_px = saved_start_of_row_pixel;
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f &m) {
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f &v) {
    view = v;
    view_to_occlusion = occlusion_view * view.inverse();
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f &p) {
    projection = p;
}

void rst::rasterizer::set_lights(const std::vector<light> &l) {
    lights = l;
}

void rst::rasterizer::set_occlusion_view(const Eigen::Matrix4f &v) {
    occlusion_view = v;
}

void rst::rasterizer::set_shadow_view(const Eigen::Matrix4f &v) {
    shadow_view = v;
}

void rst::rasterizer::set_shadow_buffer(const std::vector<float> &shadow_buffer) {
    std::copy(shadow_buffer.begin(), shadow_buffer.end(), this->occlusion_map.begin());
}

void rst::rasterizer::set_occlusion_buffer(const std::vector<float> &occlusion_buffer) {
    std::fill(occlusion_buf.begin(), occlusion_buf.end(), std::numeric_limits<float>::infinity());
    std::copy(occlusion_buffer.begin(), occlusion_buffer.end(), this->occlusion_buf.begin());
    //occlusion_buf.resize(w * h);

    //std::cout << occlusion_buf[0] << "1111111111111" << std::endl;
}

void rst::rasterizer::clear(rst::Buffers buff) {
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color) {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f(BACKGROUND_COLOR));
        std::fill(ssaa_frame_buf.begin(), ssaa_frame_buf.end(), Eigen::Vector3f(BACKGROUND_COLOR));
    }

    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth) {

        //std::fill(occlusion_buf.begin(), occlusion_buf.end(), std::numeric_limits<float>::infinity());

        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(ssaa_depth_buf.begin(), ssaa_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h) {
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    occlusion_buf.resize(w * h);

    occlusion_map.resize(w * h);
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
