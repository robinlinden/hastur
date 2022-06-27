// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

uniform vec2 resolution;
uniform vec3 inner_top_left;
uniform vec3 inner_top_right;
uniform vec3 inner_bottom_left;
uniform vec3 inner_bottom_right;
uniform vec3 outer_top_left;
uniform vec3 outer_top_right;
uniform vec3 outer_bottom_left;
uniform vec3 outer_bottom_right;
uniform vec4 left_border_color;
uniform vec4 right_border_color;
uniform vec4 top_border_color;
uniform vec4 bottom_border_color;

// Gets the position of the fragment in screen coordinates
vec3 get_frag_pos() {
    vec2 p = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
    return vec3(p.x, p.y, 0.0);
}

// Checks if a point is inside a quadrilateral
bool is_point_inside(vec3 p, vec3 a, vec3 b, vec3 c, vec3 d) {
    return dot(cross(p - a, b - a), cross(p - d, c - d)) <= 0.0 &&
            dot(cross(p - a, d - a), cross(p - b, c - b)) <= 0.0;
}

void main() {
    vec3 p = get_frag_pos();
    if (is_point_inside(p, inner_top_left, inner_top_right, inner_bottom_right, inner_bottom_left)) {
        // The fragment is not on a border, set fully transparent color
        gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
    } else {
        if (is_point_inside(p, outer_top_left, outer_top_right, inner_top_right, inner_top_left)) {
            gl_FragColor = top_border_color;
        } else if (is_point_inside(p, outer_top_right, outer_bottom_right, inner_bottom_right, inner_top_right)) {
            gl_FragColor = right_border_color;
        } else if (is_point_inside(p, outer_bottom_right, outer_bottom_left, inner_bottom_left, inner_bottom_right)) {
            gl_FragColor = bottom_border_color;
        } else if (is_point_inside(p, outer_bottom_left, outer_top_left, inner_top_left, inner_bottom_left)) {
            gl_FragColor = left_border_color;
        } else {
            // The fragment is not on a border, set fully transparent color
            gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
        }
    }
}
