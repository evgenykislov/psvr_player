#include "tile.h"

void AddTile(std::vector<uint32_t>& canvas, size_t canvas_width,
    const std::vector<uint32_t>& tile, size_t tile_width,
    size_t xpos, size_t ypos) {
  if (canvas_width == 0 || tile_width == 0) { return; }
  if ((canvas.size() % canvas_width) != 0) { return; }
  if ((tile.size() % tile_width) != 0) { return; }
  if (xpos >= canvas_width) { return; }

  uint32_t* canvas_end = canvas.data() + canvas.size();
  const uint32_t* tile_end = tile.data() + tile.size();
  size_t row_width = std::min(canvas_width - xpos, tile_width);

  uint32_t* dst = canvas.data() + ypos * canvas_width + xpos;
  const uint32_t* src = tile.data();
  while (dst < canvas_end && src < tile_end) {
    for (size_t j = 0; j < row_width; ++j) {
      struct pixel {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
      };

      auto pres = *(pixel*)(dst + j);
      auto pmsk = *(pixel*)(src + j);
      float sol = pmsk.alpha / 255.0f;
      float tra = 1.0f - sol;
      pres.red = pres.red * tra + pmsk.red * sol;
      pres.green = pres.green * tra + pmsk.green * sol;
      pres.blue = pres.blue * tra + pmsk.blue * sol;
      pres.alpha = 255 - (uint8_t)((255 - pres.alpha) * tra);
      *(pixel*)(dst + j) = pres;
    }
    dst += canvas_width;
    src += tile_width;
  }
}
