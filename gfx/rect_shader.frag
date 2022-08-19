// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

uniform vec2 resolution;

// Corner position, inner rectangle
uniform vec2 inner_top_left;
uniform vec2 inner_top_right;
uniform vec2 inner_bottom_left;
uniform vec2 inner_bottom_right;

// Corner position, outer rectangle
uniform vec2 outer_top_left;
uniform vec2 outer_top_right;
uniform vec2 outer_bottom_left;
uniform vec2 outer_bottom_right;

// Corner radii of outer rectangle
uniform vec2 top_left_radii;
uniform vec2 top_right_radii;
uniform vec2 bottom_left_radii;
uniform vec2 bottom_right_radii;

// Colors
uniform vec4 left_border_color;
uniform vec4 right_border_color;
uniform vec4 top_border_color;
uniform vec4 bottom_border_color;
uniform vec4 inner_rect_color;

// Gets the position of the fragment in screen coordinates
vec2 get_frag_pos() {
    return vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
}

vec2 top_left_inward_pos(vec2 origin, vec2 offset) {
    return vec2(origin.x + offset.x, origin.y + offset.y);
}

vec2 top_right_inward_pos(vec2 origin, vec2 offset) {
    return vec2(origin.x - offset.x, origin.y + offset.y);
}

vec2 bottom_left_inward_pos(vec2 origin, vec2 offset) {
    return vec2(origin.x + offset.x, origin.y - offset.y);
}

vec2 bottom_right_inward_pos(vec2 origin, vec2 offset) {
    return vec2(origin.x - offset.x, origin.y - offset.y);
}

bool is_point_inside_quadrilateral(vec2 p2, vec2 a2, vec2 b2, vec2 c2, vec2 d2) {
    vec3 p = vec3(p2.xy, 0.0);
    vec3 a = vec3(a2.xy, 0.0);
    vec3 b = vec3(b2.xy, 0.0);
    vec3 c = vec3(c2.xy, 0.0);
    vec3 d = vec3(d2.xy, 0.0);
    return dot(cross(p - a, b - a), cross(p - d, c - d)) <= 0.0 &&
           dot(cross(p - a, d - a), cross(p - b, c - b)) <= 0.0;
}

bool is_point_inside_ellipse(vec2 p, vec2 origin, vec2 radii) {
    return (pow(p.x - origin.x, 2.0) / pow(radii.x, 2.0) + pow(p.y - origin.y, 2.0) / pow(radii.y, 2.0)) <= 1.0;
}

bool is_inside_rounded_rect(vec2 p,
                            vec2 top_left_pos,
                            vec2 top_right_pos,
                            vec2 bottom_left_pos,
                            vec2 bottom_right_pos,
                            vec2 top_left_radii,
                            vec2 top_right_radii,
                            vec2 bottom_left_radii,
                            vec2 bottom_right_radii) {
    if (is_point_inside_quadrilateral(p,
                                      top_left_pos,
                                      top_right_pos,
                                      bottom_right_pos,
                                      bottom_left_pos)) {
        if (top_left_radii.x > 0.0 || top_left_radii.y > 0.0) {
            vec2 c = top_left_inward_pos(top_left_pos, top_left_radii);
            if (p.x <= c.x && p.y <= c.y && !is_point_inside_ellipse(p, c, top_left_radii)) {
                return false;
            }
        }
        if (top_right_radii.x > 0.0 || top_right_radii.y > 0.0) {
            vec2 c = top_right_inward_pos(top_right_pos, top_right_radii);
            if (p.x >= c.x && p.y <= c.y && !is_point_inside_ellipse(p, c, top_right_radii)) {
                return false;
            }
        }
        if (bottom_left_radii.x > 0.0 || bottom_left_radii.y > 0.0) {
            vec2 c = bottom_left_inward_pos(bottom_left_pos, bottom_left_radii);
            if (p.x <= c.x && p.y >= c.y && !is_point_inside_ellipse(p, c, bottom_left_radii)) {
                return false;
            }
        }
        if (bottom_right_radii.x > 0.0 || bottom_right_radii.y > 0.0) {
            vec2 c = bottom_right_inward_pos(bottom_right_pos, bottom_right_radii);
            if (p.x >= c.x && p.y >= c.y && !is_point_inside_ellipse(p, c, bottom_right_radii)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

vec2 max_radius(vec2 r) {
    float lr = max(r.x, r.y);
    return vec2(lr, lr);
}

void main() {
    vec4 no_color = vec4(1.0, 1.0, 1.0, 0.0);
    vec2 p = get_frag_pos();

    // Calculate radius for all corners of the inner rectangle
    vec2 inner_top_left_radii = vec2(max(top_left_radii.x - (inner_top_left.x - outer_top_left.x), 0.0),
                                     max(top_left_radii.y - (inner_top_left.y - outer_top_left.y), 0.0));
    vec2 inner_top_right_radii = vec2(max(top_right_radii.x - (outer_top_right.x - inner_top_right.x), 0.0),
                                      max(top_right_radii.y - (inner_top_right.y - outer_top_right.y), 0.0));
    vec2 inner_bottom_left_radii = vec2(max(bottom_left_radii.x - (inner_bottom_left.x - outer_bottom_left.x), 0.0),
                                        max(bottom_left_radii.y - (outer_bottom_left.y - inner_bottom_left.y), 0.0));
    vec2 inner_bottom_right_radii = vec2(max(bottom_right_radii.x - (outer_bottom_right.x - inner_bottom_right.x), 0.0),
                                         max(bottom_right_radii.y - (outer_bottom_right.y - inner_bottom_right.y), 0.0));

    // Check if the point is inside the inner rectangle
    if (is_inside_rounded_rect(p,
                               inner_top_left,
                               inner_top_right,
                               inner_bottom_left,
                               inner_bottom_right,
                               inner_top_left_radii,
                               inner_top_right_radii,
                               inner_bottom_left_radii,
                               inner_bottom_right_radii)) {
        gl_FragColor = inner_rect_color;
    // Otherwise, check if the point is inside the outer rectangle
    } else if (is_inside_rounded_rect(p,
                                      outer_top_left,
                                      outer_top_right,
                                      outer_bottom_left,
                                      outer_bottom_right,
                                      top_left_radii,
                                      top_right_radii,
                                      bottom_left_radii,
                                      bottom_right_radii)) {
        // Extract the largest inner radius for each corner
        vec2 max_inner_top_left_radii = max_radius(inner_top_left_radii);
        vec2 max_inner_top_right_radii = max_radius(inner_top_right_radii);
        vec2 max_inner_bottom_left_radii = max_radius(inner_bottom_left_radii);
        vec2 max_inner_bottom_right_radii = max_radius(inner_bottom_right_radii);

        // Adjust position for corners of inner rectangle with respect to the max radius
        // This is needed to get a good transition between colors
        vec2 inner_top_left_inward = top_left_inward_pos(inner_top_left, max_inner_bottom_left_radii);
        vec2 inner_top_right_inward = top_right_inward_pos(inner_top_right, max_inner_top_right_radii);
        vec2 inner_bottom_left_inward = bottom_left_inward_pos(inner_bottom_left, max_inner_bottom_left_radii);
        vec2 inner_bottom_right_inward = bottom_right_inward_pos(inner_bottom_right, max_inner_bottom_right_radii);

        // Check if point is on the top border
        if (is_point_inside_quadrilateral(p,
                                          outer_top_left,
                                          outer_top_right,
                                          inner_top_right_inward,
                                          inner_top_left_inward)) {
            gl_FragColor = top_border_color;
        // Otherwise, check if point is on the right border
        } else if (is_point_inside_quadrilateral(p,
                                                 outer_top_right,
                                                 outer_bottom_right,
                                                 inner_bottom_right_inward,
                                                 inner_top_right_inward)) {
            gl_FragColor = right_border_color;
        // Otherwise, check if point is on the bottom border
        } else if (is_point_inside_quadrilateral(p,
                                                 outer_bottom_right,
                                                 outer_bottom_left,
                                                 inner_bottom_left_inward,
                                                 inner_bottom_right_inward)) {
            gl_FragColor = bottom_border_color;
        // Otherwise, check if point is on the left border
        } else if (is_point_inside_quadrilateral(p,
                                                 outer_bottom_left,
                                                 outer_top_left,
                                                 inner_top_left_inward,
                                                 inner_bottom_left_inward)) {
            gl_FragColor = left_border_color;
        // Otherwise, set transparent color
        } else {
            gl_FragColor = no_color;
        }
    }
    // Otherwise, set transparent color
    else {
        gl_FragColor = no_color;
    }
}
