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


#define DEBUG_DISTORSION


/*! Смещение изображения к центру в размерности изображения для компенсации
глазного расстояния. Такое же смещение внесено в modelview_projection_uni */
uniform float vertex_x_disp;

// in vec3 vertex_attr;
in vec4 vertex_attr;

out vec4 position_var;
out float blue_x_disp;
out float blue_y_disp;
out float green_x_disp;
out float green_y_disp;

out vec4 info_red_position;
out vec4 info_green_position;
out vec4 info_blue_position;


vec4 GetGreenPosition(vec4 pos) {
  pos.y += 0.0015 * pos.w;
  pos.x /= 0.995;
  pos.y /= 0.994;
  return pos;
}


vec4 GetBluePosition(vec4 pos) {
  pos.x -= 0.0015 * pos.w;
  pos.y += 0.002 * pos.w;
  pos.x /= 0.990;
  pos.y /= 0.990;
  return pos;
}


#ifdef DEBUG_DISTORSION

float kDistorsion0 = 1.0; // Len = 0. It's center of screen. k = 1.0
float kDistorsion1 = 1.0; // Len = 0.250. It's first square center of edge. k near 1.0
float kDistorsion2 = 1.0; // Len = 0.354. It's first square corner
float kDistorsion3 = 1.0; // Len = 0.500. Second square edge center
float kDistorsion4 = 1.0; // Len = 0.750. Second square corner and third square edge center
float kDistorsion5 = 1.0; // Len = 1.000. Forth square edge center
float kDistorsion6 = 1.0; // Len = 1.400. Forth square corner
float kDistorsionMaxDist = 1.5;

vec4 FixDistorsion(vec4 pos) {
  // Отладочная версия
  vec2 cpos;
  cpos.x = pos.x / pos.z / pos.w;
  cpos.y = pos.y / pos.z / pos.w;
  float len = length(cpos);
  if (len > kDistorsionMaxDist) { return pos; }
  float k = 1.0;
  if (len <= 0.250) {
    k = kDistorsion0 + (kDistorsion1 - kDistorsion0) / 0.250 * len;
  } else if (len <= 0.354) {
    k = kDistorsion1 + (kDistorsion2 - kDistorsion1) / 0.104 * (len - 0.250);
  } else if (len <= 0.500) {
    k = kDistorsion2 + (kDistorsion3 - kDistorsion2) / 0.146 * (len - 0.354);
  } else if (len <= 0.750) {
    k = kDistorsion3 + (kDistorsion4 - kDistorsion3) / 0.250 * (len - 0.500);
  } else if (len <= 1.000) {
    k = kDistorsion4 + (kDistorsion5 - kDistorsion4) / 0.250 * (len - 0.750);
  } else {
    k = kDistorsion5 + (kDistorsion6 - kDistorsion5) / 0.400 * (len - 1.000);
  }

  pos.x *= k;
  pos.y *= k;
  return pos;
}

#else

vec4 FixDistorsion(vec4 pos) {

  return pos;
}

#endif

void main(void)
{
  vec4 screen_scale = vec4(1.15, 1.0, 1.0, 1.0);
  float disp_scale = 0.85;

  position_var = vertex_attr;
  // scr_pos это позиция точки на экране с правильными пропорциями:
  // горизонтальный и вертикальный отрезки одинаковой длины выглядят одинаково
  // для глаза
  vec4 scr_pos = /* modelview_projection_uni * */ vertex_attr; // vec4(vertex_attr, 1.0);

  gl_Position = FixDistorsion(scr_pos);

  // Vertial/Horizontal ratio compensation
  gl_Position *= screen_scale;
  gl_Position.z = 0.0;

  // Compensate chromatic abberation
  float blue_y = -0.015; // 1.0192
  float blue_x = -0.01; // 1.0224
  float green_y = -0.005; // 1.0078
  float green_x = -0.004; // 1.0091
  blue_x_disp = scr_pos.x / scr_pos.w * blue_x;
  blue_y_disp = scr_pos.y / scr_pos.w * blue_y;
  green_x_disp = scr_pos.x / scr_pos.w * green_x;
  green_y_disp = scr_pos.y / scr_pos.w * green_y;


  float info_scale = 1.0;
  info_red_position = scr_pos;
//  info_red_position.x /= info_red_position.z;
  info_red_position.x /= info_scale;
  info_red_position.y /= info_scale * -1.0;
//  info_red_position.z = 1.0;
  info_green_position = GetGreenPosition(info_red_position);
  info_blue_position = GetBluePosition(info_red_position);

//  info_red_position.x -= vertex_x_disp;
//  info_green_position.x -= vertex_x_disp;
//  info_blue_position.x -= vertex_x_disp;
}
