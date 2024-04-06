#version 330

/*
 * Created by Florian Märkl <info@florianmaerkl.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

uniform mat4 modelview_projection_uni;

//!< Смещение изображения к центру в размерности изображения на один глаз. Типовое значение 0.01
uniform float vertex_x_disp;

in vec3 vertex_attr;

out vec3 position_var;
out float blue_x_disp;
out float blue_y_disp;
out float green_x_disp;
out float green_y_disp;

out vec3 info_red_position;
// out vec3 info_green_position;
// out vec3 info_blue_position;

void main(void)
{
  position_var = vertex_attr;
  vec4 scr_pos = vec4(vertex_attr, 1.0); // modelview_projection_uni * vec4(vertex_attr, 1.0);
  gl_Position = scr_pos;

  // Compensate distortion
  if (false) {
    const float scr_radius = 1.8;
    const float dis_k1 = -0.32; // Manual calibration
    const float dis_k2 = 0.015;
    const float min_radius = 0.01;
    const float max_radius = 3.0;
    vec2 dispos;
    vec4 distorsion = vec4(1.0, 1.0, 1.0, 1.0);
    dispos.x = scr_pos.x / scr_pos.z;
    dispos.y = scr_pos.y / scr_pos.z;
    float dislen = length(dispos) / scr_radius;
    if (dislen > min_radius && dislen < max_radius) {
      float dislen2 = dislen * dislen;
      float dislen4 = dislen2 * dislen2;
      float newlen = dislen + dis_k1 * dislen2 + dis_k2 * dislen4;
      float dis_koef = newlen / dislen;
      distorsion = vec4(dis_koef, dis_koef, 1.0, 1.0);
    }

    gl_Position = scr_pos * distorsion;
  }

  // Compensate chromatic abberation
  float blue_y = -0.015; // 1.0192
  float blue_x = -0.01; // 1.0224
  float green_y = -0.005; // 1.0078
  float green_x = -0.004; // 1.0091
  blue_x_disp = gl_Position.x / gl_Position.z * blue_x;
  blue_y_disp = gl_Position.y / gl_Position.z * blue_y;
  green_x_disp = gl_Position.x / gl_Position.z * green_x;
  green_y_disp = gl_Position.y / gl_Position.z * green_y;


  float info_x_scale = -0.8;
  float info_y_scale = -0.6;
  info_red_position = vertex_attr;
  info_red_position.x /= info_red_position.z * info_x_scale;
  info_red_position.y /= info_red_position.z * info_y_scale;
  info_red_position.z = 1.0;

  info_red_position.x -= vertex_x_disp;
  position_var.x -= vertex_x_disp;
}
