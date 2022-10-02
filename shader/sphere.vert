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

in vec3 vertex_attr;

out vec3 position_var;

void main(void)
{
  position_var = vertex_attr;
  vec4 scr_pos = modelview_projection_uni * vec4(vertex_attr, 1.0);
  gl_Position = scr_pos;

  if (true) {
    // distorsion
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
}
