#version 330

/*
 * Created by Evgeny Kislov <dev@evgenykislov.com>
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

uniform sampler2D tex_uni;
uniform bool show_progress;
uniform float progress_value;

in vec3 position_var;
out vec4 color_out;

bool ShowProgressAndExit(vec3 pos) {
  bool bkground = false;
  bool frame = false;
  bool bar = false;

  const float bkground_t = 0.15;
  const float border_t = 0.005;
  const float border_xl = -0.4;
  const float border_xr = 0.4;
  const float border_yt = -0.2;
  const float border_yb = -0.22;

//  frame = frame || bool((pos.x >= border_x) && (pos.x <= (border_x + border_thickness)));

  bkground = pos.x >= (border_xl - bkground_t) && pos.x <= (border_xr + bkground_t) &&
      pos.y >= (border_yb - bkground_t) && pos.y <= (border_yt + bkground_t);

  if (pos.x >= border_xl && pos.x <= border_xr) {
    frame = frame || ((pos.y >= (border_yt - border_t)) && (pos.y <= border_yt));
    frame = frame || ((pos.y >= border_yb) && (pos.y <= (border_yb + border_t)));
  }

  if (pos.y <= border_yt && pos.y >= border_yb) {
    frame = frame || ((pos.x >= border_xl) && (pos.x <= (border_xl + border_t)));
    frame = frame || ((pos.x >= border_xr) && (pos.x <= (border_xr + border_t)));
  }

  float bound_x = (border_xr - border_xl) * progress_value + border_xl;
  if (pos.y <= (border_yt - border_t) && pos.y >= (border_yb + border_t)) {
    if (pos.x >= (border_xl + border_t) && pos.x <= bound_x) {
      bar = true;
    }
  }


  if (frame) {
    color_out = vec4(1.0, 1.0, 1.0, 1.0);
    return true;
  }

  if (bar) {
    color_out = vec4(0.8, 0.8, 0.8, 1.0);
    return true;
  }

  if (bkground) {
    color_out = vec4(0.0, 0.0, 0.0, 1.0);
    return true;
  }


  return false;
}

void main(void)
{
  // position_var is changed in -1.0 to 1.0 range for x, y

  // debug
  if (show_progress || true) {
    if (ShowProgressAndExit(position_var)) {
      return;
    }
  }

  // Show shot
  vec2 tex_pos;
  tex_pos.x = position_var.x / 2.0 + 0.5;
  tex_pos.y = position_var.y / 2.0 + 0.5;
  color_out.rgb = texture(tex_uni, tex_pos).rgb;
  color_out.a = 1.0;
}
