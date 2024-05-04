#ifndef TILE_PSVR_PLAYER_04052024
#define TILE_PSVR_PLAYER_04052024

#include <cstdint>
#include <vector>

/*! Добавляет tile (прямоугольную картинку) на исходное полотно canvas.
Tile и Canvas должны быть строго прямоугольными.
каждый пиксель представляется 4-х байтным числом с последовательностью rgba
Для обработки прозрачности используется последний байт. Tile отрисовывается
только в границах Canvas.
\param canvas полотно, на которое добавляется tile
\param canvas_width ширина полотна в пикселях
\param tile рисуемый фрагмент
\param tile_width ширина tile-а в пикселях
\param xpos, ypos позиция размещения tile на canvas
*/
void AddTile(std::vector<uint32_t>& canvas, size_t canvas_width,
    const std::vector<uint32_t>& tile, size_t tile_width,
    size_t xpos, size_t ypos);


#endif
