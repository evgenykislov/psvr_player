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

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D tex_uni;
uniform sampler2D tex_info;
uniform vec4 min_max_uv_uni;
uniform float projection_angle_factor_uni;
uniform bool cylinder_type;
uniform mat4 modelview_projection_uni;

in vec3 position_var;
in float blue_x_disp;
in float blue_y_disp;
in float green_x_disp;
in float green_y_disp;

in vec3 info_red_position;
in vec3 info_green_position;
in vec3 info_blue_position;


out vec4 color_out;


vec4 GetCylinderColor(vec3 position) {
  const float cylinder_radius = 1.0;
  const float plane_distance = 1.0;
  const float plane_arc = 1.3;
  const float xarc = plane_arc * plane_distance / cylinder_radius; // in radians
  const float yarc = xarc / 16.0 * 9.0;
  float relx = position.x / (position.z / plane_distance);
  float rely = position.y / (position.z / plane_distance);
  float anglex = atan(relx, cylinder_radius);
  float angley = atan(-rely, cylinder_radius);

  if (anglex < -xarc/2 || anglex > xarc/2) {
    return vec4(0.0);
  }

  if (angley < -yarc/2 || angley > yarc/2) {
    return vec4(0.0);
  }

  vec2 cyl_coor;
  cyl_coor.x = 0.5 * anglex / (xarc / 2.0) + 0.5;
  cyl_coor.y = 0.5 * angley / (yarc / 2.0) + 0.5;

  vec2 uv = min_max_uv_uni.xy + (min_max_uv_uni.zw - min_max_uv_uni.xy) * cyl_coor;
  return vec4(texture(tex_uni, uv).rgb, 1.0);
}

vec4 GetSphereColor(vec3 position) {
  vec2 dir_h = position.xz;
  float length_h = length(dir_h);
  dir_h /= length_h;
  vec2 dir_v = normalize(vec2(length_h, position.y));

  vec2 sphere_coord = vec2(acos(dir_h.y), acos(dir_v.y)) * vec2(0.5 / M_PI, 1.0 / M_PI);
  if(dir_h.x > 0.0) {
    sphere_coord.x = 1.0 - sphere_coord.x;
  }

  sphere_coord.x -= 0.5;
  sphere_coord.x *= projection_angle_factor_uni;
  sphere_coord.x += 0.5;

  vec3 color;

  if(sphere_coord.x < 0.0 || sphere_coord.x > 1.0)
  {
    color = vec3(0.0);
  }
  else
  {
    vec2 uv = min_max_uv_uni.xy + (min_max_uv_uni.zw - min_max_uv_uni.xy) * sphere_coord;
    color = texture(tex_uni, uv).rgb;
  }

  return vec4(color, 1.0);
}


vec4 GetInfoColor(vec3 pos) {
  vec4 info_color = vec4(0, 0, 0, 0);
  vec2 pos2 = pos.xy + vec2(0.5, 0.5);
  if (pos2.x > 0 && pos2.x < 1 && pos2.y > 0 && pos2.y < 1) {
    info_color = texture(tex_info, pos2);
  }
  return info_color;
}



void main(void)
{
  // Calculates position of colored point
  vec4 view_pos = modelview_projection_uni * vec4(position_var, 1.0);

  vec3 pos_red = view_pos.xyz; // Red without correction
  vec3 pos_blue = pos_red;
  pos_blue.x += pos_blue.z * blue_x_disp;
  pos_blue.y += pos_blue.z * blue_y_disp;
  vec3 pos_green = pos_red;
  pos_green.x += pos_green.z * green_x_disp;
  pos_green.y += pos_green.z * green_y_disp;

  if (cylinder_type) {
    color_out = GetCylinderColor(pos_red);
    color_out.b = GetCylinderColor(pos_blue).b;
    color_out.g = GetCylinderColor(pos_green).g;
  }
  else {
    color_out = GetSphereColor(pos_red);
    color_out.b = GetSphereColor(pos_blue).b;
    color_out.g = GetSphereColor(pos_green).g;
  }

  // Координаты отображения для красного (info_red_position),
  // зелёного (info_green_position) и синего (info info_blue_position) цвета
  // Координаты поля: x = -1 - +1; y = -1 - +1; z = 1
  // Координата x возрастает слева (-1) - направо (+1);
  // у возрастает снизу (-1) вверх (+1)
  // Координаты могут выходить за границы поля или быть меньше, т.к. есть
  // коррекция глазного расстояния


  // Наложим маску информации
  vec4 infor = GetInfoColor(info_red_position);
  vec4 infog = GetInfoColor(info_green_position);
  vec4 infob = GetInfoColor(info_blue_position);

  color_out.r = color_out.r * (1.0 - infor.a) + infor.r * infor.a;
  color_out.g = color_out.g * (1.0 - infog.a) + infog.g * infog.a;
  color_out.b = color_out.b * (1.0 - infob.a) + infob.b * infob.a;
}
