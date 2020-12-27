//
// Created by Jacob Menke on 6/16/17.
//

#ifndef __GETCOLORS_H
#define __GETCOLORS_H

extern int RECEIVE_BAR_COLOR[];
extern int SENT_BAR_COLOR[];
extern int BOTH_BAR_COLOR[];

extern int SCALE_BAR_COLOR[];

extern int SCALE_MARKERS_COLOR[];
extern int DL_UL_INDICATOR_COLOR[];
extern int HOST1_COLOR[];
extern int HOST2_COLOR[];

extern int TWO_SECOND_TRANSFER_COLUMN_COLOR[];
extern int TEN_SECOND_TRANSFER_COLUMN_COLOR[];
extern int FOURTY_SECOND_TRANSFER_COLUMN_COLOR[];

extern int BOTTOM_BAR_COLOR[];
extern int CUM_LABEL_COLOR[];
extern int PEAK_LABEL_COLOR[];
extern int RATES_LABEL_COLOR[];

extern int TOTAL_LABEL_COLOR[];
extern int CUM_TRANSFER_COLUMN_COLOR[];
extern int PEAK_TRANSFER_COLUMN_COLOR[];


int convertColorToInt(char *);

int convertBoldToInt(char *);

void getColors();


#endif //CTEST_GETCOLORS_H
