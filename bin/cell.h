#ifndef _CELL_H_
#define _CELL_H_

struct _Cell
{
   uint16_t     alter;
   unsigned int tile : 12;
   unsigned int unit_below : 7;
   unsigned int unit_above : 7;
   unsigned int orient_below : 3; /* 8 values */
   unsigned int orient_above : 3; /* 8 values */
   unsigned int player_below : 3; /* 8 values */
   unsigned int player_above : 3; /* 8 values */
   unsigned int spread_below : 2; /* 4 values */
   unsigned int spread_above : 2; /* 4 values */
   unsigned int anchor_below : 1;
   unsigned int anchor_above : 1;
};

Cell **cell_matrix_new(const unsigned int w, const unsigned int h);
void cell_matrix_free(Cell **cells);
void cell_matrix_zero(Cell **cells, size_t count);

#endif /* ! _CELL_H_ */

